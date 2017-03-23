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

//
// This is an illustration of how one could use virtualization techniques to
// allow running applications on virtual machines talking over simulated
// networks.
//
// The actual steps required to configure the virtual machines can be rather
// involved, so we don't go into that here.  Please have a look at one of
// our HOWTOs on the nsnam wiki for more details about how to get the 
// system confgured.  For an example, have a look at "HOWTO Use Linux 
// Containers to set up virtual networks" which uses this code as an 
// example.
//
// The configuration you are after is explained in great detail in the 
// HOWTO, but looks like the following:
//
//  +----------+                           +----------+
//  | virtual  |                           | virtual  |
//  |  Linux   |                           |  Linux   |
//  |   Host   |                           |   Host   |
//  |          |                           |          |
//  |   eth0   |                           |   eth0   |
//  +----------+                           +----------+
//       |                                      |
//  +----------+                           +----------+
//  |  Linux   |                           |  Linux   |
//  |  Bridge  |                           |  Bridge  |
//  +----------+                           +----------+
//       |                                      |
//  +------------+                       +-------------+
//  | "tap-left" |                       | "tap-right" |
//  +------------+                       +-------------+
//       |           n0            n1           |
//       |       +--------+    +--------+       |
//       +-------|  tap   |    |  tap   |-------+
//               | bridge |    | bridge |
//               +--------+    +--------+
//               |  CSMA  |    |  CSMA  |
//               +--------+    +--------+
//                   |             |
//                   |             |
//                   |             |
//                   ===============
//                      CSMA LAN
//
#include <iostream>
#include <fstream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/tap-bridge-module.h"
#include "ns3/internet-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TapCsmaVirtualMachineExample");

int 
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  //
  // We are interacting with the outside, real, world.  This means we have to 
  // interact in real-time and therefore means we have to use the real-time
  // simulator and take the time to calculate checksums.
  //
  GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  //
  // Create two ghost nodes.  The first will represent the virtual machine host
  // on the left side of the network; and the second will represent the VM on 
  // the right side.
  //
  NodeContainer nodes;
  nodes.Create (4);

  InternetStackHelper stack;
  stack.Install (nodes);

  //
  // Use a CsmaHelper to get a CSMA channel created, and the needed net 
  // devices installed on both of the nodes.  The data rate and delay for the
  // channel can be set through the command-line parser.  For example,
  //
  // ./waf --run "tap=csma-virtual-machine --ns3::CsmaChannel::DataRate=10000000"
  //
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", DataRateValue (5000000));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));

  Ipv4AddressHelper addresses;

  NodeContainer nodepair1;
  nodepair1.Add (nodes.Get(0));
  nodepair1.Add (nodes.Get(1));
  NetDeviceContainer devices1 = csma.Install (nodepair1);
  addresses.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces1 = addresses.Assign (devices1);

  NodeContainer nodepair2;
  nodepair2.Add (nodes.Get(1));
  nodepair2.Add (nodes.Get(2));
  NetDeviceContainer devices2 = csma.Install (nodepair2);
  addresses.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces2 = addresses.Assign (devices2);

  NodeContainer nodepair3;
  nodepair3.Add (nodes.Get(2));
  nodepair3.Add (nodes.Get(3));
  NetDeviceContainer devices3 = csma.Install (nodepair3);
  addresses.SetBase ("10.2.2.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces3 = addresses.Assign (devices3);


  //
  // Use the TapBridgeHelper to connect to the pre-configured tap devices for 
  // the left side.  We go with "UseBridge" mode since the CSMA devices support
  // promiscuous mode and can therefore make it appear that the bridge is 
  // extended into ns-3.  The install method essentially bridges the specified
  // tap to the specified CSMA device.
  //
  TapBridgeHelper tapBridge;
  tapBridge.SetAttribute ("Mode", StringValue ("UseBridge"));
  tapBridge.SetAttribute ("DeviceName", StringValue ("tap-client"));
  tapBridge.Install (nodes.Get (0), devices1.Get (0));

  //
  // Connect the right side tap to the right side CSMA device on the right-side
  // ghost node.
  //
  tapBridge.SetAttribute ("DeviceName", StringValue ("tap-server"));
  tapBridge.Install (nodes.Get (3), devices3.Get (1));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  //
  // Run the simulation for ten minutes to give the user time to play around
  //
  Simulator::Stop (Seconds (600.));
  Simulator::Run ();
  Simulator::Destroy ();
}
