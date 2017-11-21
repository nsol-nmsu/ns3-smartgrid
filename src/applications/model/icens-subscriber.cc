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

#include "icens-subscriber.h"
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

NS_LOG_COMPONENT_DEFINE ("ns3.iCenSSubscriber");

NS_OBJECT_ENSURE_REGISTERED (iCenSSubscriber);

TypeId
iCenSSubscriber::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::iCenSSubscriber")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<iCenSSubscriber> ()
    .AddAttribute ("RemoteAddress",
                   "The destination Address of the outbound packet",
                   AddressValue (),
                   MakeAddressAccessor (&iCenSSubscriber::m_remote_address),
                   MakeAddressChecker ())
    .AddAttribute ("RemotePort",
   		   "The destination port of the outbound packets",
                   UintegerValue (0),
                   MakeUintegerAccessor (&iCenSSubscriber::m_remote_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketSize",
                   "The size of outbound packet",
                   UintegerValue (1000),
                   MakeUintegerAccessor (&iCenSSubscriber::m_packetSize),
		   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("SocketType",
                   "The type of socket to create.By default, uses a UDP socket",
                   StringValue ("ns3::UdpSocketFactory"),
                   MakeStringAccessor (&iCenSSubscriber::m_socket_type),
                   MakeStringChecker ())
    .AddAttribute ("Frequency",
                   "The frequency at which packets are sent",
                   TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&iCenSSubscriber::m_frequency),
                   MakeTimeChecker ())
    .AddAttribute ("Subscription",
                   "The subscription value of the packet. 0-normal packet, 1-soft subscribe, 2-hard subscriber, 3-unsubsribe ",
                   UintegerValue (0),
                   MakeUintegerAccessor (&iCenSSubscriber::m_subscription),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Offset",
                   "Random offset to start sending packets",
                   UintegerValue (0),
                   MakeUintegerAccessor (&iCenSSubscriber::m_offset),
                   MakeUintegerChecker<uint32_t> ())
    .AddTraceSource ("ReceivedPacket",
                     "A packet has been received",
                     MakeTraceSourceAccessor (&iCenSSubscriber::m_receivedPacket),
                     "ns3::iCenSSubscriber::ReceivedPacketTraceCallback")
    .AddTraceSource ("SentPacket",
                     "A packet has been sent",
                     MakeTraceSourceAccessor (&iCenSSubscriber::m_sentPacket),
                     "ns3::iCenSSubscriber::SentPacketTraceCallback")

  ;
  return tid;
}

iCenSSubscriber::iCenSSubscriber ()
  : m_socket (0),
    m_remote_address (),
    m_remote_port (0),
    m_packetSize (0),
    m_sendEvent (),
    m_running (false),
    m_firstTime (true),
    m_seq (100)
{
}

iCenSSubscriber::~iCenSSubscriber()
{
  m_socket = 0;
}

void
iCenSSubscriber::StartApplication (void)
{

  m_running = true;

  if (m_socket == 0) {   

	//Create socket
	m_socket = Socket::CreateSocket (GetNode(), TypeId::LookupByName (m_socket_type));

  	if (Ipv4Address::IsMatchingType(m_remote_address) == true)
        {
            m_socket->Bind();
            m_socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_remote_address), m_remote_port));
        }
        else if (Ipv6Address::IsMatchingType(m_remote_address) == true)
        {
            m_socket->Bind6();
            m_socket->Connect (Inet6SocketAddress (Ipv6Address::ConvertFrom(m_remote_address), m_remote_port));
        }
        else if (InetSocketAddress::IsMatchingType (m_remote_address) == true)
        {
            m_socket->Bind ();
            m_socket->Connect (m_remote_address);
        }
        else if (Inet6SocketAddress::IsMatchingType (m_remote_address) == true)
        {
            m_socket->Bind6 ();
            m_socket->Connect (m_remote_address);
        }
        else
        {
            NS_ASSERT_MSG (false, "Incompatible address type: " << m_remote_address);
        }
  }

  m_socket->SetRecvCallback (MakeCallback (&iCenSSubscriber::HandleRead, this));

  ScheduleNextPacket();

}

void
iCenSSubscriber::StopApplication (void)
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
}

void
iCenSSubscriber::SendPacket (void)
{

  NS_ASSERT (m_sendEvent.IsExpired ());

  //Create a packet of specified size and send through the socket
  Ptr<Packet> packet = Create<Packet> (m_packetSize);

  //Add subscription information (extra 4-bytes) to packet header and send
  iCenSHeader packetHeader;
  if (m_subscription == 0) {
	//Use sequence number in subscription field to identify non-subscription packets when received at destination (m_subscription=0)
  	packetHeader.SetSubscription(m_seq);
  }
  else {
	//Leave subscription value in this field
	packetHeader.SetSubscription(m_subscription);
/*
if(m_subscription == 1 || m_subscription == 2) {
std::cout << "about to sent packet with SUB not seq " << m_subscription << std::endl;
}
*/

  }
  packet->AddHeader(packetHeader);

  m_socket->Send (packet);

  if (Ipv4Address::IsMatchingType (m_remote_address))
  {
     NS_LOG_INFO ("Node(" << GetNode()->GetId() << ") SENT packet of size " << packet->GetSize () << " bytes to - " <<
     	Ipv4Address::ConvertFrom (m_remote_address) << ":" << m_remote_port);
  }
  else if (Ipv6Address::IsMatchingType (m_remote_address))
  {
     NS_LOG_INFO ("Node(" << GetNode()->GetId() << ") SENT packet of size " << packet->GetSize () << " bytes to - " <<
     	Ipv6Address::ConvertFrom (m_remote_address) << ":" << m_remote_port);
  }
  else if (InetSocketAddress::IsMatchingType (m_remote_address))
  {
     NS_LOG_INFO ("Node(" << GetNode()->GetId() << ") SENT packet of size " << packet->GetSize () << " bytes to - " <<
	InetSocketAddress::ConvertFrom (m_remote_address).GetIpv4 () << ":" << InetSocketAddress::ConvertFrom (m_remote_address).GetPort ());
  }
  else if (Inet6SocketAddress::IsMatchingType (m_remote_address))
  {
     NS_LOG_INFO ("Node(" << GetNode()->GetId() << ") SENT packet of size " << packet->GetSize () << " bytes to - " <<
        Inet6SocketAddress::ConvertFrom (m_remote_address).GetIpv6 () << " port " << Inet6SocketAddress::ConvertFrom (m_remote_address).GetPort ());
  }

  // Callback for sent packet
  m_sentPacket (GetNode()->GetId(), packet, m_remote_address, m_seq);

  //Increase sequence number for next packet
  m_seq = m_seq + 1;

  //Schedule next packet to be transmitted
  ScheduleNextPacket ();

}

void
iCenSSubscriber::ScheduleNextPacket ()
{
  if (m_firstTime) {
	m_sendEvent = Simulator::Schedule(Seconds(double(m_offset)), &iCenSSubscriber::SendPacket, this);
	m_firstTime = false;
  }
  else if (!m_sendEvent.IsRunning()) {
  	m_sendEvent = Simulator::Schedule (m_frequency, &iCenSSubscriber::SendPacket, this);
  }
}

void
iCenSSubscriber::HandleRead (Ptr<Socket> socket)
{

   Ptr<Packet> packet;
   Address from;

   while ((packet = socket->RecvFrom (from)))
   {
       if (InetSocketAddress::IsMatchingType (from))
       {
          NS_LOG_INFO ("Node(" << GetNode()->GetId() << ") RECEIVED REPLY packet of size " << packet->GetSize () << " bytes from - " <<
             InetSocketAddress::ConvertFrom (from).GetIpv4 () << ":" << InetSocketAddress::ConvertFrom (from).GetPort ());
       }
       else if (Inet6SocketAddress::IsMatchingType (from))
       {
          NS_LOG_INFO ("Node(" << GetNode()->GetId() << ") RECEIVED REPLY packet of size " << packet->GetSize () << " bytes from - " <<
             Inet6SocketAddress::ConvertFrom (from).GetIpv6 () << ":" << Inet6SocketAddress::ConvertFrom (from).GetPort ());
       }

       // Callback for received packet
       m_receivedPacket (GetNode()->GetId(), packet, from);
   }
}

}//namespace
