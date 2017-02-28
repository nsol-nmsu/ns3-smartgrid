/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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

#include <fstream>
#include <string>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/icens-subscriber.h"
#include "ns3/point-to-point-net-device.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("iCenS");

int main (int argc, char *argv[])
{

  CommandLine cmd;
  cmd.Parse (argc, argv);

  //Create network nodes
  NodeContainer nodes;
  nodes.Create(5);

  //Install internet protocol stack on nodes
  InternetStackHelper stack;
  stack.Install (nodes);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
  //pointToPoint.SetDeviceAttribute ("Mtu", StringValue ("100"));

  //Declare objects for network devices, addresses and interfaces
  NetDeviceContainer devices;
  Ipv4AddressHelper address;

  //Setup network segments with IP addresses
  devices = pointToPoint.Install (nodes.Get(0), nodes.Get(1));
  address.SetBase ("10.10.1.0", "255.255.255.252");
  Ipv4InterfaceContainer interfaces1 = address.Assign (devices);

  devices = pointToPoint.Install (nodes.Get(1), nodes.Get(2));
  address.SetBase ("10.10.2.0", "255.255.255.252");
  Ipv4InterfaceContainer interfaces2 = address.Assign (devices);

  devices = pointToPoint.Install (nodes.Get(2), nodes.Get(3));
  address.SetBase ("10.10.3.0", "255.255.255.252");
  Ipv4InterfaceContainer interfaces3 = address.Assign (devices);

  devices = pointToPoint.Install (nodes.Get(3), nodes.Get(4));
  address.SetBase ("10.10.4.0", "255.255.255.252");
  Ipv4InterfaceContainer interfaces4 = address.Assign (devices);

/*
  devices = pointToPoint.Install (nodes.Get(3), nodes.Get(5));
  address.SetBase ("10.10.5.0", "255.255.255.252");
  Ipv4InterfaceContainer interfaces5 = address.Assign (devices);

  devices = pointToPoint.Install (nodes.Get(3), nodes.Get(6));
  address.SetBase ("10.10.6.0", "255.255.255.252");
  Ipv4InterfaceContainer interfaces6 = address.Assign (devices);

  devices = pointToPoint.Install (nodes.Get(4), nodes.Get(5));
  address.SetBase ("10.10.7.0", "255.255.255.252");
  Ipv4InterfaceContainer interfaces7 = address.Assign (devices);

  devices = pointToPoint.Install (nodes.Get(4), nodes.Get(6));
  address.SetBase ("10.10.8.0", "255.255.255.252");
  Ipv4InterfaceContainer interfaces8 = address.Assign (devices);
*/

  //Populate the routing table
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  //Install producer application on compute nodes for PMU
  uint16_t pmuPort = 7000;
  ns3::AppHelper proHelper ("ns3::iCenSProducer");
  proHelper.SetAttribute ("LocalPort", UintegerValue (pmuPort));
  proHelper.SetAttribute ("Frequency", TimeValue (Seconds (0.0)));
  ApplicationContainer proApps = proHelper.Install (nodes.Get (4));

  //proApps = proHelper.Install (nodes.Get (6));

  //Install aggregator application on aggregation nodes for PMU
  ns3::AppHelper aggHelper ("ns3::iCenSAggregator");
  aggHelper.SetAttribute ("LocalPort", UintegerValue (pmuPort));
  aggHelper.SetAttribute ("Frequency", TimeValue (Seconds (2)));
  aggHelper.SetAttribute ("RemoteAddress", AddressValue (InetSocketAddress (interfaces4.GetAddress (1), pmuPort)));
  //aggHelper.SetAttribute ("Offset", UintegerValue (500));
  ApplicationContainer aggApps = aggHelper.Install (nodes.Get (2));

  //Install subscriber application on physical nodes for PMU
  ns3::AppHelper subHelper ("ns3::iCenSSubscriber");
  ApplicationContainer subApps;

  subHelper.SetAttribute ("RemoteAddress", AddressValue (InetSocketAddress (interfaces2.GetAddress (1), pmuPort)));
  subHelper.SetAttribute ("PacketSize", UintegerValue (470));
  subHelper.SetAttribute ("Subscription", UintegerValue (0));
  subHelper.SetAttribute ("Frequency", TimeValue (Seconds (1)));
  subApps = subHelper.Install (nodes.Get (0));
  //subApps.Start (Seconds (0.2));



/*
  ApplicationContainer subApps2;
  subHelper.SetAttribute ("RemoteAddress", AddressValue (InetSocketAddress (interfaces3.GetAddress (0), pmuMsgPort)));
  subHelper.SetAttribute ("PacketSize", UintegerValue (120));
  subHelper.SetAttribute ("Subscription", UintegerValue (0));
  subHelper.SetAttribute ("Frequency", TimeValue (Seconds (3)));
  subApps2 = subHelper.Install (nodes.Get (3));
  //subApps2.Start (Seconds (0.0));
*/

/*
  Ptr<Ipv4> ipv4n2 = nodes.Get(2)->GetObject<Ipv4> ();
  Simulator::Schedule (Seconds (1.0),&Ipv4::SetDown,ipv4n2, 2); //bring down the second p2p interface attached on node 3
  Simulator::Schedule (Seconds (1.5),&Ipv4::SetUp,ipv4n2, 2);

  Ptr<Ipv4> ipv4n4 = nodes.Get(4)->GetObject<Ipv4> ();
  Simulator::Schedule (Seconds (2),&Ipv4::SetDown,ipv4n4, 2); //bring down the second p2p interface attached on node 4
  Simulator::Schedule (Seconds (4),&Ipv4::SetUp,ipv4n4, 2);
*/

/*
  // Trace routing tables
  Ipv4GlobalRoutingHelper g;
  Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("dynamic-global-routing.routes", std::ios::out);
  g.PrintRoutingTableAllAt (Seconds (1), routingStream);
*/

  //Run actual simulation
  Simulator::Stop (Seconds(5.0));
  Simulator::Run ();
  Simulator::Destroy ();

}
