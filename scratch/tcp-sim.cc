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

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("FirstScriptExample");

int
main (int argc, char *argv[])
{
  Time::SetResolution (Time::NS);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  //Create network nodes
  NodeContainer nodes;
  nodes.Create (3);

  //Install internet protocol stack on nodes
  InternetStackHelper stack;
  stack.Install (nodes);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  //Declare objects for network devices, addresses and interfaces
  NetDeviceContainer devices;
  Ipv4AddressHelper address;
  Ipv4InterfaceContainer interfaces;

  //Setup first network segment
  devices = pointToPoint.Install (nodes.Get(0), nodes.Get(1));
  address.SetBase ("10.1.1.0", "255.255.255.252");
  interfaces = address.Assign (devices);

  //Setup second network segment
  devices = pointToPoint.Install (nodes.Get(1), nodes.Get(2));
  address.SetBase ("10.1.2.0", "255.255.255.252");
  interfaces = address.Assign (devices);

  //Populate the routing table
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  //Install applications
  iCenSTCPServerHelper echoServer;
  echoServer.SetAttribute ("Port", UintegerValue (9000));

  ApplicationContainer serverApps = echoServer.Install (nodes.Get (2));
  //serverApps.Start (Seconds (1.0));
  //serverApps.Stop (Seconds (10.0));

  iCenSTCPClientHelper echoClient;
  echoClient.SetAttribute ("RemoteAddress", AddressValue (InetSocketAddress ( interfaces.GetAddress (1) , 9000)));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
  echoClient.SetAttribute ("Subscription", UintegerValue (0));

  ApplicationContainer clientApps = echoClient.Install (nodes.Get (0));
  //clientApps.Start (Seconds (2.0));
  //clientApps.Stop (Seconds (10.0));

  Simulator::Stop (Seconds(3.0));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
