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
 */

#ifndef ICENS_PRODUCER_H
#define ICENS_PRODUCER_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"
#include "ns3/network-module.h"
#include "ns3/string.h"

namespace ns3 {

class Address;
class Socket;
class Packet;

/**
 * \brief A server application for iCenS simulation
 *
 * Receives packet on specified port and sends acknowledgements. Installed at the compute layer of the iCenS architecture. 
 * Also used for demand-reponse where packets are sent at specific intervals based on subscription
 * requests from clients at the physical layer
 */

class iCenSProducer : public Application
{
public:


  typedef void (* ReceivedPacketTraceCallback) (uint32_t nodeid, Ptr<Packet> packet, const Address &address, uint32_t localport, uint32_t packetSize, uint32_t subscription, Ipv4Address localip);
  typedef void (* SentPacketTraceCallback) (uint32_t nodeid, Ptr<Packet> packet, const Address &address, uint32_t localport);

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  iCenSProducer ();

  virtual ~iCenSProducer();

  bool IsNewClientConnection(Address client_address);

  /**
  * \brief Handle received packets
  */
  void HandleRead (Ptr<Socket> socket);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTransmit (Ptr<Socket> socket, Address client_address);
  void SendPacket (Ptr<Socket> socket, Address client_address);

  Ptr<Socket>     m_socket;
  Ptr<Socket>     m_socket6;
  Address         m_remote_address;
  uint16_t        m_local_port; //!< The port on which to bind server application
  uint32_t        m_packet_size;
  EventId         m_sendEvent;
  bool            m_running;
  TypeId          m_tid;
  std::string	  m_socket_type;
  Time 		  m_frequency; //!< Time interval at which packets are sent
  uint32_t	  m_subscription; //!< Subscription value of packet
  bool		  m_firstTime;
  size_t 	  m_subDataSize; //Size of subscription data, in Kbytes
  std::vector<Address> m_client_addresses;
  Ipv4Address     m_local_ip;

  TracedCallback<uint32_t, Ptr<Packet>, const Address &, uint32_t, uint32_t, uint32_t, Ipv4Address> m_receivedPacket;
  TracedCallback<uint32_t, Ptr<Packet>, const Address &, uint32_t> m_sentPacket;

};

} //namespace

#endif //ICENS_PRODUCER_H
