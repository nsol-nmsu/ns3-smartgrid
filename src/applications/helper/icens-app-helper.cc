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

#include "icens-app-helper.h"
#include "ns3/icens-producer.h"
#include "ns3/icens-aggregator.h"
#include "ns3/icens-subscriber.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"

namespace ns3 {

ProducerHelper::ProducerHelper ()
{
  m_factory.SetTypeId ("ns3::iCenSProducer");
}

void
ProducerHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
ProducerHelper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;

      m_server = m_factory.Create<iCenSProducer> ();
      node->AddApplication (m_server);
      apps.Add (m_server);

    }
  return apps;
}


//---------

AggregatorHelper::AggregatorHelper ()
{
  m_factory.SetTypeId ("ns3::iCenSAggregator");
}

void
AggregatorHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
AggregatorHelper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;

      m_server = m_factory.Create<iCenSAggregator> ();
      node->AddApplication (m_server);
      apps.Add (m_server);

    }
  return apps;
}

//---


SubscriberHelper::SubscriberHelper ()
{
  m_factory.SetTypeId ("ns3::iCenSSubscriber");
}

void
SubscriberHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
SubscriberHelper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;

      m_server = m_factory.Create<iCenSSubscriber> ();
      node->AddApplication (m_server);
      apps.Add (m_server);

    }
  return apps;
}

//---


iCenSTCPServerHelper::iCenSTCPServerHelper ()
{ 
  m_factory.SetTypeId ("ns3::iCenSTCPServer");
}

void
iCenSTCPServerHelper::SetAttribute (std::string name, const AttributeValue &value)
{ 
  m_factory.Set (name, value);
}

ApplicationContainer
iCenSTCPServerHelper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;

      m_server = m_factory.Create<iCenSTCPServer> ();
      node->AddApplication (m_server);
      apps.Add (m_server);

    }
  return apps;
}

//---


iCenSTCPClientHelper::iCenSTCPClientHelper ()
{ 
  m_factory.SetTypeId ("ns3::iCenSTCPClient");
}

void
iCenSTCPClientHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
iCenSTCPClientHelper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;

      m_server = m_factory.Create<iCenSTCPClient> ();
      node->AddApplication (m_server);
      apps.Add (m_server);

    }
  return apps;
}

} // namespace ns3
