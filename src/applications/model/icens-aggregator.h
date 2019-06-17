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

#ifndef ICENS_AGGREGATOR_H
#define ICENS_AGGREGATOR_H

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
 * \brief An aggregator application for iCenS simulation
 *
 * Receives packets from physical layer on specified port and keeps a sum of all the payload sizes. A new packet is re-created with its payload size equal to
 * the sum of all the payloads received during a configured time interval and sent to the compute layer.
 * Installed at the aggregation layer of the iCenS architecture.
 */

class iCenSAggregator : public Application
{
public:

  typedef void (* ReceivedPacketTraceCallback) (uint32_t nodeid, Ptr<Packet> packet, const Address &address, uint32_t localport, uint32_t recvseq);
  typedef void (* SentPacketTraceCallback) (uint32_t nodeid, Ptr<Packet> packet, const Address &address, uint32_t sendseq);

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  iCenSAggregator ();

  virtual ~iCenSAggregator();

  /**
  * \brief Handle received packets from physical layer nodes
  */
  void HandleRead (Ptr<Socket> socket);

  /**
  * \brief Handle received acknowledgements from compute nodes, after aggregated packets are sent
  */
  void HandleACK (Ptr<Socket> socket);


private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleAggPackets ();
  void SendAggPacket ();

  Ptr<Socket>     m_socket;
  Ptr<Socket>     m_socket6;
  Ptr<Socket>	  m_comp_socket;
  Address         m_remote_address; //!< The destination address of the compute node to deliver aggregated payload to
  uint16_t        m_local_port; //!< The port on which to bind server application
  uint32_t        m_packet_size;
  EventId         m_sendEvent;
  bool            m_running;
  TypeId          m_tid;
  std::string	  m_socket_type;
  Time 		  m_frequency; //!< Time interval at which packets are sent
  uint32_t	  m_subscription; //!< Subscription value of packet
  bool		  m_firstTime;
  size_t	  m_totalpayload;
  uint32_t	  m_offset;
  Address	  m_src_address; //!< The source address of the physical node that sent the payloaded packet
  uint32_t	  m_recv_seq;
  uint32_t	  m_send_seq;

  TracedCallback<uint32_t, Ptr<Packet>, const Address &, uint32_t, uint32_t> m_receivedPacket;
  TracedCallback<uint32_t, Ptr<Packet>, const Address &, uint32_t> m_sentPacket;

};

} //namespace

#endif //ICENS_AGGREGATOR_H
