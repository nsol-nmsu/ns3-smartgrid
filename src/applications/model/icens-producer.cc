/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author:  Tom Henderson (tomhend@u.washington.edu)
 */

#include "icens-producer.h"
#include "ns3/address.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/ipv4.h"
#include "ns3/icens-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ns3.iCenSProducer");

NS_OBJECT_ENSURE_REGISTERED (iCenSProducer);

TypeId
iCenSProducer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::iCenSProducer")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<iCenSProducer> ()
    .AddAttribute ("RemoteAddress",
                   "The destination Address of the outbound packet",
                   AddressValue (),
                   MakeAddressAccessor (&iCenSProducer::m_remote_address),
                   MakeAddressChecker ())
    .AddAttribute ("LocalPort",
   		   "The local port on which server listens for requests",
                   UintegerValue (0),
                   MakeUintegerAccessor (&iCenSProducer::m_local_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketSize",
                   "The size of outbound packet, typically acknowledgement packets from server application",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&iCenSProducer::m_packet_size),
		   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("SocketType",
                   "The type of socket to create.By default, uses a UDP socket",
                   StringValue ("ns3::UdpSocketFactory"),
                   MakeStringAccessor (&iCenSProducer::m_socket_type),
                   MakeStringChecker ())
    .AddAttribute ("Frequency",
                   "The frequency at which packets are sent for demand-response flows",
                   TimeValue (Seconds (5.0)),
                   MakeTimeAccessor (&iCenSProducer::m_frequency),
                   MakeTimeChecker ())
    .AddTraceSource ("ReceivedPacket",
                     "A packet has been received",
                     MakeTraceSourceAccessor (&iCenSProducer::m_receivedPacket),
                     "ns3::iCenSProducer::ReceivedPacketTraceCallback")
    .AddTraceSource ("SentPacket",
                     "A packet has been sent",
                     MakeTraceSourceAccessor (&iCenSProducer::m_sentPacket),
                     "ns3::iCenSProducer::SentPacketTraceCallback")

  ;
  return tid;
}

iCenSProducer::iCenSProducer ()
  : m_socket (0),
    m_socket6 (0),
    m_remote_address (),
    m_packet_size (0),
    m_sendEvent (),
    m_running (false),
    m_subscription (0),
    m_firstTime (true),
    m_subDataSize (10)
{
}

iCenSProducer::~iCenSProducer()
{
  m_socket = 0;
  m_socket6 = 0;
}

void
iCenSProducer::StartApplication (void)
{

  m_running = true;

  //Listen on IPv4 socket
  if (m_socket == 0) {
	m_socket = Socket::CreateSocket (GetNode(), TypeId::LookupByName (m_socket_type));
  	m_socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), m_local_port));
  }

  //Listen on IPv6 socket
  if (m_socket6 == 0) {
        m_socket6 = Socket::CreateSocket (GetNode(), TypeId::LookupByName (m_socket_type));
        m_socket6->Bind (Inet6SocketAddress (Ipv6Address::GetAny (), m_local_port));
  }

  m_socket->SetRecvCallback (MakeCallback (&iCenSProducer::HandleRead, this));
  m_socket6->SetRecvCallback (MakeCallback (&iCenSProducer::HandleRead, this));
}

void
iCenSProducer::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }

  if (m_socket6)
    {
      m_socket6->Close ();
    }
}

void
iCenSProducer::SendPacket (Ptr<Socket> socket, Address client_address)
{

    //Create a packet of specified size and send it back to the source, will be ACK for non-subscription flows
    Ptr<Packet> packet = Create<Packet> (m_packet_size);
    socket->SendTo (packet, 0, client_address);

    if (InetSocketAddress::IsMatchingType (client_address))
    {
      NS_LOG_INFO ("Node(" << GetNode()->GetId() << ") SENT REPLY packet of size " << m_packet_size << " bytes to " <<
         InetSocketAddress::ConvertFrom (client_address).GetIpv4 () << ":" << InetSocketAddress::ConvertFrom (client_address).GetPort ());
    }
    else if (Inet6SocketAddress::IsMatchingType (client_address))
    {
      NS_LOG_INFO ("Node(" << GetNode()->GetId() << ") SENT REPLY packet of size " << m_packet_size << " bytes to " <<
         Inet6SocketAddress::ConvertFrom (client_address).GetIpv6 () << ":" << Inet6SocketAddress::ConvertFrom (client_address).GetPort ());
    }

    // Callback for sent packet
    m_sentPacket (GetNode()->GetId(), packet, client_address, m_local_port);
}

void
iCenSProducer::ScheduleTransmit (Ptr<Socket> socket, Address client_address)
{
	double send_delay = 0.0; 

	//Do not send initial data before scheduling with the input frequency
	if(IsNewClientConnection(client_address)) {
		m_sendEvent = Simulator::Schedule (m_frequency, &iCenSProducer::ScheduleTransmit, this, socket, client_address);
	}
	else {
		//Sent as multiple chunks, sice data is bigger than MTU of 1500
		for (int i=0; i<(int)m_subDataSize; i++) {
    			//SendPacket(socket, client_address);
			Simulator::Schedule (Seconds(send_delay), &iCenSProducer::SendPacket, this, socket, client_address);
			send_delay = send_delay + 0.03;
		}
    		m_sendEvent = Simulator::Schedule (m_frequency, &iCenSProducer::ScheduleTransmit, this, socket, client_address);	
	}

}

bool
iCenSProducer::IsNewClientConnection(Address client_address) {

	//Check if client is already connected
	for (int i=0; i<(int)m_client_addresses.size(); i++) {
		if ( operator==(client_address, m_client_addresses[i]) ) {
		//if ( str_client_address.compare(m_client_addresses[i]) == 0) {
			return false;
		}
	}

	//Add new client to vector
	m_client_addresses.push_back( client_address );

	return true;
} 

void
iCenSProducer::HandleRead (Ptr<Socket> socket)
{

  Ptr<Packet> packet;

  while ((packet = socket->RecvFrom (m_remote_address)))
  {

    //Get the first interface IP attached to this node (this is where socket is bound, true nodes that have only 1 IP)
    //Ptr<NetDevice> PtrNetDevice = PtrNode->GetDevice(0);
    Ptr <Node> PtrNode = this->GetNode();
    Ptr<Ipv4> ipv4 = PtrNode->GetObject<Ipv4> (); 
    Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1,0);  
    m_local_ip = iaddr.GetLocal (); 

    if (InetSocketAddress::IsMatchingType (m_remote_address))
    {
      NS_LOG_INFO ("Node(" << GetNode()->GetId() << ") RECEIVED packet of size " << packet->GetSize () - 4 << " bytes from " <<
         InetSocketAddress::ConvertFrom (m_remote_address).GetIpv4 () << ":" << InetSocketAddress::ConvertFrom (m_remote_address).GetPort ());
    }
    else if (Inet6SocketAddress::IsMatchingType (m_remote_address))
    {
      NS_LOG_INFO ("Node(" << GetNode()->GetId() << ") RECEIVED packet of size " << packet->GetSize () - 4 << " bytes from " <<
         Inet6SocketAddress::ConvertFrom (m_remote_address).GetIpv6 () << ":" << Inet6SocketAddress::ConvertFrom (m_remote_address).GetPort ());
    }

    //packet->RemoveAllPacketTags ();
    //packet->RemoveAllByteTags ();

    //Get subscription value set in packet's payload
    iCenSHeader packetHeader;
    packet->RemoveHeader(packetHeader);
    m_subscription = packetHeader.GetSubscription();
    NS_LOG_INFO("SUBSCRIPTION value = " << m_subscription);
    
    //Send packet through the socket
    if (m_subscription >= 100 ) {
	//1-byte acknowledgement, for packets with sequence numbers (m_subscription >=100)
	m_packet_size = 1;
    	SendPacket(socket,m_remote_address);
    }
    else if (m_subscription == 1 || m_subscription == 2) {

        //Soft or Hard subsription set
	if (m_frequency != 0) {
		//Application instance is set for demand-response flow, send separate packet to each client that subscribes
		ScheduleTransmit(socket, m_remote_address);
	}
    }

    // Callback for received packet
    m_receivedPacket (GetNode()->GetId(), packet, m_remote_address, m_local_port, m_packet_size, m_subscription, m_local_ip);
  }

}

}//namespace
