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

#include "icens-aggregator.h"
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

NS_LOG_COMPONENT_DEFINE ("ns3.iCenSAggregator");

NS_OBJECT_ENSURE_REGISTERED (iCenSAggregator);

TypeId
iCenSAggregator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::iCenSAggregator")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<iCenSAggregator> ()
    .AddAttribute ("RemoteAddress",
                   "The destination Address of the outbound packet",
                   AddressValue (),
                   MakeAddressAccessor (&iCenSAggregator::m_remote_address),
                   MakeAddressChecker ())
    .AddAttribute ("LocalPort",
   		   "The local port on which server listens for requests",
                   UintegerValue (0),
                   MakeUintegerAccessor (&iCenSAggregator::m_local_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketSize",
                   "The size of outbound packet, typically acknowledgement packets from server application",
                   UintegerValue (1),
                   MakeUintegerAccessor (&iCenSAggregator::m_packet_size),
		   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("SocketType",
                   "The type of socket to create.By default, uses a UDP socket",
                   StringValue ("ns3::UdpSocketFactory"),
                   MakeStringAccessor (&iCenSAggregator::m_socket_type),
                   MakeStringChecker ())
    .AddAttribute ("Frequency",
                   "The frequency at which packets are aggregated and forwarded upstream",
                   TimeValue (Seconds (5.0)),
                   MakeTimeAccessor (&iCenSAggregator::m_frequency),
                   MakeTimeChecker ())
    .AddAttribute ("Offset",
                   "Random offset from when to start aggregation of packets",
                   UintegerValue (0),
                   MakeUintegerAccessor (&iCenSAggregator::m_offset),
                   MakeUintegerChecker<uint32_t> ())
    .AddTraceSource ("ReceivedPacket",
                     "A packet has been received",
                     MakeTraceSourceAccessor (&iCenSAggregator::m_receivedPacket),
                     "ns3::iCenSAggregator::ReceivedPacketTraceCallback")
    .AddTraceSource ("SentPacket",
                     "A packet has been sent",
                     MakeTraceSourceAccessor (&iCenSAggregator::m_sentPacket),
                     "ns3::iCenSAggregator::SentPacketTraceCallback")

  ;
  return tid;
}

iCenSAggregator::iCenSAggregator ()
  : m_socket (0),
    m_comp_socket (0),
    m_remote_address (),
    m_packet_size (0),
    m_sendEvent (),
    m_running (false),
    m_subscription (0),
    m_firstTime (true),
    m_totalpayload (0)
{
}

iCenSAggregator::~iCenSAggregator()
{
  m_socket = 0;
  m_socket6 = 0;
}

void
iCenSAggregator::StartApplication (void)
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

  m_socket->SetRecvCallback (MakeCallback (&iCenSAggregator::HandleRead, this));
  m_socket6->SetRecvCallback (MakeCallback (&iCenSAggregator::HandleRead, this));

  // Create socket (only once) to compute node that this aggregator sends packets to
  if (m_comp_socket == 0) {
        m_comp_socket = Socket::CreateSocket (GetNode(), TypeId::LookupByName (m_socket_type));

        if (InetSocketAddress::IsMatchingType (m_remote_address) == true)
        {
            m_comp_socket->Bind ();
            m_comp_socket->Connect (m_remote_address);
        }
        else if (Inet6SocketAddress::IsMatchingType (m_remote_address) == true)
        {
            m_comp_socket->Bind6 ();
            m_comp_socket->Connect (m_remote_address);
        }
        else
        {
            NS_ASSERT_MSG (false, "Incompatible address type: " << m_remote_address);
        }
  }

  //Handle acknowledgement responses back from compute nodes on the new socket
  m_comp_socket->SetRecvCallback (MakeCallback (&iCenSAggregator::HandleACK, this));

  //Send aggregated packets at scheduled rate
  ScheduleAggPackets();
}

void
iCenSAggregator::StopApplication (void)
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
iCenSAggregator::ScheduleAggPackets ()
{
    if(m_firstTime) {
        m_firstTime = false;
	m_sendEvent = Simulator::Schedule(Seconds(double(m_offset)/1000), &iCenSAggregator::ScheduleAggPackets, this);
    }
    else {
	if (!m_sendEvent.IsRunning())
        	m_sendEvent = Simulator::Schedule (m_frequency, &iCenSAggregator::SendAggPacket, this);
    }
}

void
iCenSAggregator::SendAggPacket ()
{

  //If no payloaded interest received, do not send an interest with zero payload
  if (m_totalpayload > 0)
  {
	//Create a new packet the size of the aggregated payload and forward to compute node
	Ptr<Packet> aggpacket = Create<Packet> (m_totalpayload);
    	//m_comp_socket->SendTo (aggpacket, 0, m_remote_address);
	m_comp_socket->Send (aggpacket);

	if (InetSocketAddress::IsMatchingType (m_remote_address))
	{
      		NS_LOG_INFO ("Node(" << GetNode()->GetId() << ") SENT AGGREGATED packet of size " << m_totalpayload << " bytes to " <<
         	InetSocketAddress::ConvertFrom (m_remote_address).GetIpv4 () << ":" << InetSocketAddress::ConvertFrom (m_remote_address).GetPort ());
    	}
    	else if (Inet6SocketAddress::IsMatchingType (m_remote_address))
    	{
      		NS_LOG_INFO ("Node(" << GetNode()->GetId() << ") SENT AGGREGATED packet of size " << m_totalpayload << " bytes to " <<
         	Inet6SocketAddress::ConvertFrom (m_remote_address).GetIpv6 () << ":" << Inet6SocketAddress::ConvertFrom (m_remote_address).GetPort ());
    	}

	//Reset total payload size to start accumulating subsequent ones
    	m_totalpayload = 0;

        // Callback for sent packet
        m_sentPacket (GetNode()->GetId(), aggpacket, m_remote_address);

  }

  //Schedule next aggregation
  ScheduleAggPackets();
}

void
iCenSAggregator::HandleRead (Ptr<Socket> socket)
{

  Ptr<Packet> packet;

  while ((packet = socket->RecvFrom (m_src_address)))
  {

    if (InetSocketAddress::IsMatchingType (m_src_address))
    {
      NS_LOG_INFO ("Node(" << GetNode()->GetId() << ") RECEIVED packet of size " << packet->GetSize () << " bytes from " <<
         InetSocketAddress::ConvertFrom (m_src_address).GetIpv4 () << ":" << InetSocketAddress::ConvertFrom (m_src_address).GetPort ());
    }
    else if (Inet6SocketAddress::IsMatchingType (m_src_address))
    {
      NS_LOG_INFO ("Node(" << GetNode()->GetId() << ") RECEIVED packet of size " << packet->GetSize () << " bytes from " <<
         Inet6SocketAddress::ConvertFrom (m_src_address).GetIpv6 () << ":" << Inet6SocketAddress::ConvertFrom (m_src_address).GetPort ());
    }

    //Aggregate the payload size
    m_totalpayload += packet->GetSize ();

    // Callback for received packet
    m_receivedPacket (GetNode()->GetId(), packet, m_src_address, m_local_port);


  }

}

void
iCenSAggregator::HandleACK (Ptr<Socket> socket)
{

  Ptr<Packet> packet;

  while ((packet = socket->RecvFrom (m_remote_address)))
  {

    if (InetSocketAddress::IsMatchingType (m_remote_address))
    {
      NS_LOG_INFO ("Node(" << GetNode()->GetId() << ") RECEIVED ACK of size " << packet->GetSize () << " bytes from " <<
         InetSocketAddress::ConvertFrom (m_remote_address).GetIpv4 () << ":" << InetSocketAddress::ConvertFrom (m_remote_address).GetPort ());
    }
    else if (Inet6SocketAddress::IsMatchingType (m_remote_address))
    {
      NS_LOG_INFO ("Node(" << GetNode()->GetId() << ") RECEIVED ACK of size " << packet->GetSize () << " bytes from " <<
         Inet6SocketAddress::ConvertFrom (m_remote_address).GetIpv6 () << ":" << Inet6SocketAddress::ConvertFrom (m_remote_address).GetPort ());
    }
  }

}

}//namespace
