/* -*- Mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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
 */
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/address-utils.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/ipv4.h"
#include "ns3/icens-header.h"
#include "icens-tcp-client.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ns3.iCenSTCPClient");
NS_OBJECT_ENSURE_REGISTERED (iCenSTCPClient);

TypeId
iCenSTCPClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::iCenSTCPClient")
    .SetParent<Application> ()
    .AddConstructor<iCenSTCPClient> ()
    /*.AddAttribute ("MaxPackets",
                   "The maximum number of packets the application will send",
                   UintegerValue (100),
                   MakeUintegerAccessor (&iCenSTCPClient::m_count),
                   MakeUintegerChecker<uint32_t> ())
*/
    .AddAttribute ("Subscription",
                   "The subscription value of the packet. 0-normal packet, 1-soft subscribe, 2-hard subscriber, 3-unsubsribe ",
                   UintegerValue (0),
                   MakeUintegerAccessor (&iCenSTCPClient::m_subscription),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Interval",
                   "The time to wait between packets",
                   TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&iCenSTCPClient::m_interval),
                   MakeTimeChecker ())
    .AddAttribute ("RemoteAddress",
                   "The destination address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&iCenSTCPClient::m_peerAddress),
                   MakeAddressChecker ())
    .AddAttribute ("RemotePort",
                   "The destination port of the outbound packets",
                   UintegerValue (0),
                   MakeUintegerAccessor (&iCenSTCPClient::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketSize", "Size of echo data in outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&iCenSTCPClient::SetDataSize,
                                         &iCenSTCPClient::GetDataSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddTraceSource ("ReceivedPacket",
                     "A packet has been received",
                     MakeTraceSourceAccessor (&iCenSTCPClient::m_receivedPacket),
                     "ns3::iCenSTCPClient::ReceivedPacketTraceCallback")
    .AddTraceSource ("SentPacket",
                     "A packet has been sent",
                     MakeTraceSourceAccessor (&iCenSTCPClient::m_sentPacket),
                     "ns3::iCenSTCPClient::SentPacketTraceCallback")
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&iCenSTCPClient::m_txTrace))
  ;
  return tid;
}

iCenSTCPClient::iCenSTCPClient ()
{
  NS_LOG_FUNCTION_NOARGS ();
  //m_sent = 0;
  m_bytesSent = 0;
  m_recvBack = 0;
  m_bytesRecvBack = 0;
  m_socket = 0;
  m_sendEvent = EventId ();
  m_data = 0;
  m_dataSize = 0;
  m_seq = 100;
}

iCenSTCPClient::~iCenSTCPClient()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_socket = 0;

  delete [] m_data;
  m_data = 0;
  m_dataSize = 0;
}

void
iCenSTCPClient::SetRemote (Address ip, uint16_t port)
{
  m_peerAddress = ip;
  m_peerPort = port;
}

void
iCenSTCPClient::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  Application::DoDispose ();
}

void
iCenSTCPClient::StartApplication (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      if (Ipv4Address::IsMatchingType(m_peerAddress) == true)
        {
          m_socket->Bind();
          m_socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort));
        }
      else if (Ipv6Address::IsMatchingType(m_peerAddress) == true)
        {
          m_socket->Bind6();
          m_socket->Connect (Inet6SocketAddress (Ipv6Address::ConvertFrom(m_peerAddress), m_peerPort));
        }
	else if (InetSocketAddress::IsMatchingType (m_peerAddress) == true)
        {
            m_socket->Bind ();
            m_socket->Connect (m_peerAddress);
        }
        else if (Inet6SocketAddress::IsMatchingType (m_peerAddress) == true)
        {
            m_socket->Bind6 ();
            m_socket->Connect (m_peerAddress);
        }
        else
        {
            NS_ASSERT_MSG (false, "Incompatible address type: " << m_peerAddress);
        }
    }

  m_socket->SetRecvCallback (MakeCallback (&iCenSTCPClient::ReceivePacket, this));

  ScheduleTransmit (Seconds (0.));
}

void
iCenSTCPClient::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_socket != 0)
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_socket = 0;
    }

  Simulator::Cancel (m_sendEvent);
}

void
iCenSTCPClient::SetDataSize (uint32_t dataSize)
{
  NS_LOG_FUNCTION (dataSize);

  //
  // If the client is setting the echo packet data size this way, we infer
  // that she doesn't care about the contents of the packet at all, so
  // neither will we.
  //
  delete [] m_data;
  m_data = 0;
  m_dataSize = 0;
  m_size = dataSize;
}

uint32_t
iCenSTCPClient::GetDataSize (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_size;
}

void
iCenSTCPClient::SetFill (std::string fill)
{
  NS_LOG_FUNCTION (fill);

  uint32_t dataSize = fill.size () + 1;

  if (dataSize != m_dataSize)
    {
      delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  memcpy (m_data, fill.c_str (), dataSize);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}

void
iCenSTCPClient::SetFill (uint8_t fill, uint32_t dataSize)
{
  if (dataSize != m_dataSize)
    {
      delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  memset (m_data, fill, dataSize);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}

void
iCenSTCPClient::SetFill (uint8_t *fill, uint32_t fillSize, uint32_t dataSize)
{
  if (dataSize != m_dataSize)
    {
      delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  if (fillSize >= dataSize)
    {
      memcpy (m_data, fill, dataSize);
      return;
    }

  //
  // Do all but the final fill.
  //
  uint32_t filled = 0;
  while (filled + fillSize < dataSize)
    {
      memcpy (&m_data[filled], fill, fillSize);
      filled = fillSize;
    }

  //
  // Last fill may be partial
  //
  memcpy (&m_data[filled], fill, dataSize - filled);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}

void
iCenSTCPClient::ScheduleTransmit (Time dt)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_sendEvent = Simulator::Schedule (dt, &iCenSTCPClient::Send, this);
}

void
iCenSTCPClient::Send (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  NS_ASSERT (m_sendEvent.IsExpired ());

  Ptr<Packet> p;
  if (m_dataSize)
    {
      //
      // If m_dataSize is non-zero, we have a data buffer of the same size that we
      // are expected to copy and send.  This state of affairs is created if one of
      // the Fill functions is called.  In this case, m_size must have been set
      // to agree with m_dataSize
      //
      NS_ASSERT_MSG (m_dataSize == m_size, "iCenSTCPClient::Send(): m_size and m_dataSize inconsistent");
      NS_ASSERT_MSG (m_data, "iCenSTCPClient::Send(): m_dataSize but no m_data");
      p = Create<Packet> (m_data, m_dataSize);
      m_bytesSent = m_dataSize;
    }
  else
    {
      //
      // If m_dataSize is zero, the client has indicated that she doesn't care
      // about the data itself either by specifying the data size by setting
      // the corresponding atribute or by not calling a SetFill function.  In
      // this case, we don't worry about it either.  But we do allow m_size
      // to have a value different from the (zero) m_dataSize.
      //
      p = Create<Packet> (m_size);
      m_bytesSent = m_size;
    }
  // call to the trace sinks before the packet is actually sent,
  // so that tags added to the packet can be sent as well
  m_txTrace (p);

  //Add subscription information (extra 4-bytes) to packet header and send
  iCenSHeader packetHeader;
  if (m_subscription == 0) {
	//Use sequence number in subscription field to identify non-subscription packets when received at destination (m_subscription=0)
  	packetHeader.SetSubscription(m_seq);
  }
  else {
	//Leave subscription value in this field
	packetHeader.SetSubscription(m_subscription);
  }
  p->AddHeader(packetHeader);
  
  m_socket->Send (p);

//  ++m_sent;

  NS_LOG_INFO ("Sent " << m_size << " bytes to " << InetSocketAddress::ConvertFrom (m_peerAddress).GetIpv4 () 
	<< ":" << InetSocketAddress::ConvertFrom (m_peerAddress).GetPort ());


  // Callback for sent packet
  m_sentPacket (GetNode()->GetId(), p, m_peerAddress, m_seq, m_subscription);

  //Increase sequence number for next packet
  m_seq = m_seq + 1;

  ScheduleTransmit (m_interval);

}

void
iCenSTCPClient::ReceivePacket (Ptr<Socket> socket)
{

  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  while (packet = socket->RecvFrom (from))
    {
      NS_LOG_INFO ("Received " << packet->GetSize () << " bytes from " << InetSocketAddress::ConvertFrom (from).GetIpv4 ()
	<< ":" << InetSocketAddress::ConvertFrom (from).GetPort ());

      // dont check if data returned is the same data sent earlier
      m_recvBack++;
      m_bytesRecvBack = packet->GetSize ();
    }

  if (m_count == m_recvBack)
    {
	  socket->Close();
	  m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
	  socket = 0;
    }
}

} // Namespace ns3

