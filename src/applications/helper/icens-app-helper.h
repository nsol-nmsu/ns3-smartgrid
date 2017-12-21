/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 * Author: Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 */
#ifndef ICENS_APP_HELPER_H
#define ICENS_APP_HELPER_H

#include <stdint.h>
#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/icens-producer.h"
#include "ns3/icens-aggregator.h"
#include "ns3/icens-subscriber.h"
#include "ns3/icens-tcp-server.h"
#include "ns3/icens-tcp-client.h"

namespace ns3 {
/**
 * \ingroup iCenS Applications
 * \brief Create a server application which waits for input UDP packets
 *        and uses the information carried into their payload to compute
 *        delay and to determine if some packets are lost.
 */

class ProducerHelper
{
public:
  /**
   * Create iCenSProducer which will make life easier for people trying
   * to set up simulations with icens-producer application.
   *
   */
  ProducerHelper ();

  /**
   * Record an attribute to be set in each Application after it is is created.
   *
   * \param name the name of the attribute to set
   * \param value the value of the attribute to set
   */
  void SetAttribute (std::string name, const AttributeValue &value);

  /**
   * Create one UDP server application on each of the Nodes in the
   * NodeContainer.
   *
   * \param c The nodes on which to create the Applications.  The nodes
   *          are specified by a NodeContainer.
   * \returns The applications created, one Application per Node in the
   *          NodeContainer.
   */
  ApplicationContainer Install (NodeContainer c);

private:
  ObjectFactory m_factory; //!< Object factory.
  Ptr<iCenSProducer> m_server; //!< The last created server application
};


class AggregatorHelper
{
public:
  /**
   * Create iCenSAggregator which will make life easier for people trying
   * to set up simulations with icens-aggregator application.
   *
   */
  AggregatorHelper ();

  /**
   * Record an attribute to be set in each Application after it is is created.
   *
   * \param name the name of the attribute to set
   * \param value the value of the attribute to set
   */
  void SetAttribute (std::string name, const AttributeValue &value);

  /**
   * Create one UDP server application on each of the Nodes in the
   * NodeContainer.
   *
   * \param c The nodes on which to create the Applications.  The nodes
   *          are specified by a NodeContainer.
   * \returns The applications created, one Application per Node in the
   *          NodeContainer.
   */
  ApplicationContainer Install (NodeContainer c);

private:
  ObjectFactory m_factory; //!< Object factory.
  Ptr<iCenSAggregator> m_server; //!< The last created server application
};


class SubscriberHelper
{
public:
  /**
   * Create iCenSSubscriber which will make life easier for people trying
   * to set up simulations with icens-subscriber application.
   *
   */
  SubscriberHelper ();

  /**
   * Record an attribute to be set in each Application after it is is created.
   *
   * \param name the name of the attribute to set
   * \param value the value of the attribute to set
   */
  void SetAttribute (std::string name, const AttributeValue &value);

  /**
   * Create one UDP server application on each of the Nodes in the
   * NodeContainer.
   *
   * \param c The nodes on which to create the Applications.  The nodes
   *          are specified by a NodeContainer.
   * \returns The applications created, one Application per Node in the
   *          NodeContainer.
   */
  ApplicationContainer Install (NodeContainer c);

private:
  ObjectFactory m_factory; //!< Object factory.
  Ptr<iCenSSubscriber> m_server; //!< The last created server application
};


class iCenSTCPServerHelper
{
public:
  /**
   * Create iCenSTCPServer which will make life easier for people trying
   * to set up simulations with icens-subscriber application.
   *
   */
  iCenSTCPServerHelper ();

  /**
   * Record an attribute to be set in each Application after it is is created.
   *
   * \param name the name of the attribute to set
   * \param value the value of the attribute to set
   */
  void SetAttribute (std::string name, const AttributeValue &value);

  /**
   * Create one UDP server application on each of the Nodes in the
   * NodeContainer.
   *
   * \param c The nodes on which to create the Applications.  The nodes
   *          are specified by a NodeContainer.
   * \returns The applications created, one Application per Node in the
   *          NodeContainer.
   */
  ApplicationContainer Install (NodeContainer c);

private:
  ObjectFactory m_factory; //!< Object factory.
  Ptr<iCenSTCPServer> m_server; //!< The last created server application
};

class iCenSTCPClientHelper
{ 
public:
  /**
   * Create iCenSTCPClient which will make life easier for people trying
   * to set up simulations with icens-subscriber application.
   *
   */
  iCenSTCPClientHelper ();

  /**
   * Record an attribute to be set in each Application after it is is created.
   *
   * \param name the name of the attribute to set
   * \param value the value of the attribute to set
   */
  void SetAttribute (std::string name, const AttributeValue &value);

  /**
   * Create one UDP server application on each of the Nodes in the
   * NodeContainer.
   *
   * \param c The nodes on which to create the Applications.  The nodes
   *          are specified by a NodeContainer.
   * \returns The applications created, one Application per Node in the
   *          NodeContainer.
   */
  ApplicationContainer Install (NodeContainer c);

private:
  ObjectFactory m_factory; //!< Object factory.
  Ptr<iCenSTCPClient> m_server; //!< The last created server application
};


} // namespace ns3

#endif /* ICENS_APP_HELPER_H */
