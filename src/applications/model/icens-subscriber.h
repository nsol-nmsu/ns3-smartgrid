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

#ifndef ICENS_SUBSCRIBER_H
#define ICENS_SUBSCRIBER_H

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
 * \brief A client application for iCenS simulation
 *
 * Sends packets from physical layer nodes towards the aggregation and compute nodes
 *
 * Below is for server documentation...
 * Receives packet on specified port and sends acknowledgements. Installed at the compute layer of the iCenS architecture. 
 * Also used for demand-reponse where packets are sent at specific intervals based on subscription
 * requests from clients at the physical layer
 */

class iCenSSubscriber : public Application
{
public:


  typedef void (* ReceivedPacketTraceCallback) (uint32_t nodeid, Ptr<Packet> packet, const Address &address);
  typedef void (* SentPacketTraceCallback) (uint32_t nodeid, Ptr<Packet> packet, const Address &address, uint32_t seqNo);

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  iCenSSubscriber ();

  virtual ~iCenSSubscriber();

  /**
  * \brief Handle received packets
  */
  void HandleRead (Ptr<Socket> socket);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleNextPacket ();
  void SendPacket (void);

  Ptr<Socket>     m_socket;
  Address         m_remote_address; //!< IPv4/IPv6 address of remote node
  uint16_t        m_remote_port;
  uint32_t        m_packetSize;
  EventId         m_sendEvent;
  bool            m_running;
  TypeId          m_tid;
  std::string	  m_socket_type;
  Time 		  m_frequency; //!< Time interval at which packets are sent 
  uint32_t	  m_subscription; //!< Subscription value set in packet
  bool 		  m_firstTime;
  uint32_t 	  m_offset;
  uint32_t	  m_seq;

  TracedCallback<uint32_t, Ptr<Packet>, const Address &> m_receivedPacket;
  TracedCallback<uint32_t, Ptr<Packet>, const Address &, uint32_t> m_sentPacket;
};

} //namespace

#endif //ICENS_SUBSCRIBER_H
