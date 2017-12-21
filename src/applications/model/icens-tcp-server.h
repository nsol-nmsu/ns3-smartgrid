/* -*- Mode:C; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2012
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

#ifndef ICENS_TCP_SERVER
#define ICENS_TCP_SERVER

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"

namespace ns3 {

class Socket;
class Packet;

/**
 * \ingroup applications
 * \defgroup TcpEcho 
 */

/**
 * \ingroup tcpecho
 * \brief A Tcp Echo server
 *
 * Every packet received is sent back to the client.
 */
class iCenSTCPServer : public Application
{
public:

  typedef void (* ReceivedPacketTraceCallback) (uint32_t nodeid, Ptr<Packet> packet, const Address &address, uint32_t localport, uint32_t packetSize, uint32_t subscription, Ipv4Address localip);
  typedef void (* SentPacketTraceCallback) (uint32_t nodeid, Ptr<Packet> packet, const Address &address, uint32_t localport);

  static TypeId GetTypeId (void);
  iCenSTCPServer ();
  virtual ~iCenSTCPServer ();

  /**
   *
   * Receive the packet from client echo on socket level (layer 4), 
   * handle the packet and return to the client.
   *
   * \param socket TCP socket.
   *
   */
  void ReceivePacket(Ptr<Socket> socket);
  
  /**
  *
  * Handle packet from accept connections.
  *
  * \parm s TCP socket.
  * \parm from Address from client echo.
  */
  void HandleAccept (Ptr<Socket> s, const Address& from);
  
  /**
  *
  * Handle successful closing connections.
  *
  * \parm s TCP socket.
  *
  */
  void HandleSuccessClose(Ptr<Socket> s);

protected:
  virtual void DoDispose (void);

private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void HandleRead (Ptr<Socket> socket);

  Ptr<Socket> m_socket;
  Ptr<Socket> m_socket6;
  uint16_t m_local_port;
  bool m_running;
  uint32_t m_subscription; //!< Subscription value of packet
  Address m_remote_address;
  Ipv4Address m_local_ip;
  uint32_t m_packet_size;


  TracedCallback<uint32_t, Ptr<Packet>, const Address &, uint32_t, uint32_t, Ipv4Address> m_receivedPacket;
  TracedCallback<uint32_t, Ptr<Packet>, const Address &, uint32_t> m_sentPacket;

};

} // namespace ns3

#endif /* ICENS_TCP_SERVER */

