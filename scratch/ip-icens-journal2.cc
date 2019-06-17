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
#include <stdlib.h>
#include <iomanip>
#include <iostream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/icens-app-helper.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/netanim-module.h"

namespace ns3 {

//NS_LOG_COMPONENT_DEFINE ("iCenS");


void SentPacketCallbackPhy(uint32_t, Ptr<Packet>, const Address &, uint32_t seqNo);
void ReceivedPacketCallbackCom(uint32_t, Ptr<Packet>, const Address &, uint32_t, uint32_t packetSize, uint32_t subscription, Ipv4Address localip);

void SentPacketCallbackCom(uint32_t, Ptr<Packet>, const Address &, uint32_t);
void ReceivedPacketCallbackPhy(uint32_t, Ptr<Packet>, const Address &);

void ReceivedPacketCallbackAgg(uint32_t, Ptr<Packet>, const Address &, uint32_t localport, uint32_t seqNo);
void SentPacketCallbackAgg(uint32_t, Ptr<Packet>, const Address &, uint32_t seqNo);

// Vectors to store the various node types
std::vector<int> com_nodes, agg_nodes, phy_nodes;

// Vectors to store source and destination edges to fail
std::vector<int> srcedge, dstedge;

// Vectors to store agg<--->com and phy<--->agg p2p IPs, packets are directed to these IP addresses
std::vector<std::string> com_ips;
std::vector<std::pair<int,std::string>> phy_to_agg_map;

// Nodes and their corresponding IPs, used to output namespaces in tracefile
std::vector<std::pair<int,std::string>> node_ip_map;

// Node and interface pairs to fail, between com and agg
std::vector<std::pair<int,int>> node_link_map_fail;

bool NodeInComm(int);
bool NodeInAgg(int);
bool NodeInPhy(int);
int GetNodeFromIP(std::string);
std::string GetAggIP(int);

//Trace file
std::ofstream tracefile;

int main (int argc, char *argv[])
{

  CommandLine cmd;
  cmd.Parse (argc, argv);

  //--- Count the number of nodes to create
  std::ifstream nfile ("src/ndnSIM/examples/icens-nodes5.txt", std::ios::in);

  std::string nodeid, nodename, nodetype;
  int nodecount = 0; //number of nodes in topology
  int numOfPMUs = 20; //number of PMUs = 20

  if (nfile.is_open ()) {
  	while (nfile >> nodeid >> nodename >> nodetype) {
		nodecount += 1;

		if (nodename.substr(0,4) == "com_") {
			com_nodes.push_back(std::stoi(nodeid));
		}
		else if (nodename.substr(0,4) == "agg_") {
			agg_nodes.push_back(std::stoi(nodeid));
		}
		else if (nodename.substr(0,4) == "phy_") {
			phy_nodes.push_back(std::stoi(nodeid));
		}
		else {
			std::cout << "Error::Node not classified" << std::endl;
		}
  	}
  }
  else {
	std::cout << "Error::Cannot open nodes file!!!" << std::endl;
	return 1;
  }
  nfile.close();

  //Create network nodes
  NodeContainer nodes;
  nodes.Create(nodecount);

  //Install internet protocol stack on nodes
  InternetStackHelper stack;
  stack.Install (nodes);

  // Connect nodes using point to point links
  PointToPointHelper p2p;

  // Setting default parameters for PointToPoint links and channels
  //p2p.SetDeviceAttribute ("Mtu", StringValue ("18000"));

  // Setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("10"));

  //--- Get the edges of the graph from file and connect them
  std::ifstream efile ("src/ndnSIM/examples/icens-edges5.txt", std::ios::in);

  std::string srcnode, dstnode, bw, delay, edgetype;

  //Declare objects for network devices, addresses and interfaces
  NetDeviceContainer devices;
  Ipv4AddressHelper address;

  // Use class A 10.0.0.0/x for point to point links, set the base
  int octet1 = 10, octet2 = 0, octet3 = 0, octet4 = 0;
  std::string subnet, mask = "255.255.255.252";
  std::pair<int,std::string> phy_aggip_pair;
  std::pair<int,std::string> node_ip_pair;
  std::pair<int,int> node_link_pair;

  if (efile.is_open ()) {
        while (efile >> srcnode >> dstnode >> bw >> delay >> edgetype) {
                //Set delay and bandwidth parameters for point-to-point links
                p2p.SetDeviceAttribute("DataRate", StringValue(bw+"Mbps"));
                p2p.SetChannelAttribute("Delay", StringValue(delay+"ms"));
                devices = p2p.Install(nodes.Get(std::stoi(srcnode)), nodes.Get(std::stoi(dstnode)));

		if (octet4 < 256) {
			subnet = std::to_string(octet1) + "." + std::to_string(octet2) + "." + std::to_string(octet3) + "." + std::to_string(octet4);
			address.SetBase (Ipv4Address(subnet.c_str()), Ipv4Mask(mask.c_str()));
	                address.Assign (devices);
			// Source node <--> Source IP map
			node_ip_pair.first = stoi(srcnode);
			node_ip_pair.second = std::to_string(octet1) + "." + std::to_string(octet2) + "." + std::to_string(octet3) + "." + std::to_string(octet4 + 1);
			node_ip_map.push_back (node_ip_pair);
                        // Source node <--> Source IP map
                        node_ip_pair.first = stoi(dstnode);
                        node_ip_pair.second = std::to_string(octet1) + "." + std::to_string(octet2) + "." + std::to_string(octet3) + "." + std::to_string(octet4 + 2);
                        node_ip_map.push_back (node_ip_pair);
			octet4 += 4;
		}
		else {
			// 4th octet subnets exhausted
			if (octet3 < 255) {
				octet3 += 1;
				octet4 = 0;
				subnet = std::to_string(octet1) + "." + std::to_string(octet2) + "." + std::to_string(octet3) + "." + std::to_string(octet4);
				address.SetBase (Ipv4Address(subnet.c_str()), Ipv4Mask(mask.c_str()));
                        	address.Assign (devices);
                        	// Source node <--> Source IP map
                        	node_ip_pair.first = stoi(srcnode);
                        	node_ip_pair.second = std::to_string(octet1) + "." + std::to_string(octet2) + "." + std::to_string(octet3) + "." + std::to_string(octet4 + 1);
                        	node_ip_map.push_back (node_ip_pair);
                        	// Source node <--> Source IP map
                       		node_ip_pair.first = stoi(dstnode);
                        	node_ip_pair.second = std::to_string(octet1) + "." + std::to_string(octet2) + "." + std::to_string(octet3) + "." + std::to_string(octet4 + 2);
                        	node_ip_map.push_back (node_ip_pair);
                        	octet4 += 4;
			}
			else {
				// 3rd octet exhausted
				if (octet2 < 255) {
					octet2 += 1;
					octet3 += 0;
					octet4 += 0;
					subnet = std::to_string(octet1) + "." + std::to_string(octet2) + "." + std::to_string(octet3) + "." + std::to_string(octet4);
                                	address.SetBase (Ipv4Address(subnet.c_str()), Ipv4Mask(mask.c_str()));
                                	address.Assign (devices);
                        		// Source node <--> Source IP map
                        		node_ip_pair.first = stoi(srcnode);
                        		node_ip_pair.second = std::to_string(octet1) + "." + std::to_string(octet2) + "." + std::to_string(octet3) + "." + std::to_string(octet4 + 1);
                        		node_ip_map.push_back (node_ip_pair);
                        		// Source node <--> Source IP map
                        		node_ip_pair.first = stoi(dstnode);
                        		node_ip_pair.second = std::to_string(octet1) + "." + std::to_string(octet2) + "." + std::to_string(octet3) + "." + std::to_string(octet4 + 2);
                        		node_ip_map.push_back (node_ip_pair);
					octet4 += 4;
				}
				else {
					// 2nd octet exhausted
					// Graph size for simulation can accommodate this, no need to move 1st octet to 11.x.x.x
				}
			}
		}

                // Determine if an edge is between compute and aggregation node - some of these edges will be failed during simulation
                if (NodeInComm(stoi(srcnode)) && NodeInAgg(stoi(dstnode))) {
                        srcedge.push_back(stoi(srcnode));
                        dstedge.push_back(stoi(dstnode));
			// Store source IP as com
			com_ips.push_back(std::to_string(octet1) + "." + std::to_string(octet2) + "." + std::to_string(octet3) + "." + std::to_string(octet4 - 4 + 1));
			// Store the node and its corresponding link to fail
			node_link_pair.first = std::stoi(srcnode);
			node_link_pair.second = nodes.Get(std::stoi(srcnode))->GetNDevices() - 1;
			node_link_map_fail.push_back(node_link_pair);
                }
                if (NodeInAgg(stoi(srcnode)) && NodeInComm(stoi(dstnode))) {
                        srcedge.push_back(stoi(srcnode));
                        dstedge.push_back(stoi(dstnode));
			// Store source IP as com
                        com_ips.push_back(std::to_string(octet1) + "." + std::to_string(octet2) + "." + std::to_string(octet3) + "." + std::to_string(octet4 -4 + 2));
                        // Store the node and its corresponding link to fail
                        node_link_pair.first = std::stoi(dstnode);
                        node_link_pair.second = nodes.Get(std::stoi(dstnode))->GetNDevices() - 1;
                        node_link_map_fail.push_back(node_link_pair);
                }

		// Determine if an edge is between aggregation and physical node - maps physical nodes to the aggregation node it connects to
                if (NodeInPhy(stoi(srcnode)) && NodeInAgg(stoi(dstnode))) {
			phy_aggip_pair.first = stoi(srcnode);
			phy_aggip_pair.second = std::to_string(octet1) + "." + std::to_string(octet2) + "." + std::to_string(octet3) + "." + std::to_string(octet4 - 4 + 2);
			phy_to_agg_map.push_back(phy_aggip_pair);
                }

                if (NodeInAgg(stoi(srcnode)) && NodeInPhy(stoi(dstnode))) {
			phy_aggip_pair.first = stoi(dstnode);
			phy_aggip_pair.second = std::to_string(octet1) + "." + std::to_string(octet2) + "." + std::to_string(octet3) + "." + std::to_string(octet4 - 4 + 1);
			phy_to_agg_map.push_back(phy_aggip_pair);
                }
        }
  }
  else {
        std::cout << "Error::Cannot open edges file!!!" << std::endl;
        return 1;
  }
  efile.close();

  //Populate the routing table
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  //Set seed for AMI offset. This seed produces best unique values
  srand(1);

  //Install server applications on compute nodes
  uint16_t urgPort = 5000;
  uint16_t pmuPort = 6000;
  uint16_t amiPort = 7000;
  uint16_t subPort = 8000;

  ProducerHelper proHelper;
  ApplicationContainer proApps;

  for (int i=0; i<(int)com_nodes.size(); i++) {
	// Urgent messages
  	proHelper.SetAttribute ("LocalPort", UintegerValue (urgPort));
  	proHelper.SetAttribute ("Frequency", TimeValue (Seconds (0.0)));
  	proApps = proHelper.Install (nodes.Get (com_nodes[i]));
	// PMU messages
	proHelper.SetAttribute ("LocalPort", UintegerValue (pmuPort));
        proHelper.SetAttribute ("Frequency", TimeValue (Seconds (0.0)));
        proApps = proHelper.Install (nodes.Get (com_nodes[i]));
	// AMI messages
        proHelper.SetAttribute ("LocalPort", UintegerValue (amiPort));
        proHelper.SetAttribute ("Frequency", TimeValue (Seconds (0.0)));
        proApps = proHelper.Install (nodes.Get (com_nodes[i]));
	// Subscription messages
        proHelper.SetAttribute ("LocalPort", UintegerValue (subPort));
        proHelper.SetAttribute ("Frequency", TimeValue (Seconds (600.0))); //600.0
        proApps = proHelper.Install (nodes.Get (com_nodes[i]));

  }

  // Install aggregator application on aggregation nodes
  AggregatorHelper aggHelper;
  ApplicationContainer aggApps;

  for (int i=0; i<(int)agg_nodes.size(); i++) {

	int offset = (rand() % 900) + 100;

	int random_com = offset % (int)com_ips.size();

        // AMI messages
        aggHelper.SetAttribute ("LocalPort", UintegerValue (amiPort));
        aggHelper.SetAttribute ("Frequency", TimeValue (Seconds (6)));
        aggHelper.SetAttribute ("RemoteAddress", AddressValue (InetSocketAddress (Ipv4Address(com_ips[random_com].c_str()), amiPort)));
        aggHelper.SetAttribute ("Offset", UintegerValue (offset));
        aggApps = aggHelper.Install (nodes.Get (agg_nodes[i]));


  	// PMU messages
	//int random_com = (rand() % (int)com_ips.size());
	aggHelper.SetAttribute ("LocalPort", UintegerValue (pmuPort));
	aggHelper.SetAttribute ("Frequency", TimeValue (Seconds (0.016)));
	aggHelper.SetAttribute ("RemoteAddress", AddressValue (InetSocketAddress (Ipv4Address(com_ips[random_com].c_str()), pmuPort)));
	aggHelper.SetAttribute ("Offset", UintegerValue (0));
	aggApps = aggHelper.Install (nodes.Get (agg_nodes[i]));

  }


  //Install subscriber application on physical nodes for PMU
  SubscriberHelper consumerHelper;
  ApplicationContainer conApps;

  //Random seed for PMU offset. This seed produces best unique values
  srand(10);

  // Urgent messages are sent by PMUs to compute nodes for error reporting using
  for (int i=0; i<(int)phy_nodes.size(); i++) {
	if (i < numOfPMUs) {
 		consumerHelper.SetAttribute ("RemoteAddress", AddressValue (InetSocketAddress (Ipv4Address(com_ips[0].c_str()), urgPort)));
  		consumerHelper.SetAttribute ("Frequency", TimeValue (Seconds (360))); //360
		consumerHelper.SetAttribute ("Subscription", UintegerValue (0));
		consumerHelper.SetAttribute ("PacketSize", UintegerValue (60));
		int offset = (rand() % 91) + 1;
		consumerHelper.SetAttribute ("Offset", UintegerValue (offset));
		conApps = consumerHelper.Install (nodes.Get (phy_nodes[i]));
	}
  }

  // PMUs sends payload interests to aggregation layer
  for (int i=0; i<(int)phy_nodes.size(); i++) {
	if (i < numOfPMUs) {
		consumerHelper.SetAttribute ("RemoteAddress", AddressValue (InetSocketAddress (Ipv4Address(GetAggIP(phy_nodes[i]).c_str()) , pmuPort)  ));
		consumerHelper.SetAttribute ("Frequency", TimeValue (Seconds (0.016)));
		consumerHelper.SetAttribute ("Subscription", UintegerValue (0));
                consumerHelper.SetAttribute ("PacketSize", UintegerValue (90));
		consumerHelper.SetAttribute ("Offset", UintegerValue (0));
                conApps = consumerHelper.Install (nodes.Get (phy_nodes[i]));
	}
  }

  // AMIs sends payload interests to aggregation layer
  for (int i=0; i<(int)phy_nodes.size(); i++) {
        if (i >= numOfPMUs) {
                consumerHelper.SetAttribute ("RemoteAddress", AddressValue (InetSocketAddress (Ipv4Address(GetAggIP(phy_nodes[i]).c_str()), amiPort)));
                consumerHelper.SetAttribute ("Frequency", TimeValue (Seconds (6)));
                consumerHelper.SetAttribute ("Subscription", UintegerValue (0));
                consumerHelper.SetAttribute ("PacketSize", UintegerValue (60)); //60
		consumerHelper.SetAttribute ("Offset", UintegerValue (0));
                conApps = consumerHelper.Install (nodes.Get ( phy_nodes[i] ));
        }
  }

  //Use a counter to iterate through the the com IPs and subscribe
  int comip_count = (int)com_ips.size();
  int comip_index = 0;

  // Each physical layer node except PMUs, subscribe to compute prefix - "/overlay/com/subscription"
  for (int i=0; i<(int)phy_nodes.size(); i++) {
        if (i >= numOfPMUs) {
		if (comip_index >= comip_count) {
			comip_index = 0;
		}

                consumerHelper.SetAttribute ("RemoteAddress", AddressValue (InetSocketAddress (Ipv4Address(com_ips[ comip_index ].c_str()), subPort)));
                consumerHelper.SetAttribute ("Frequency", TimeValue (Seconds (900000)));
                consumerHelper.SetAttribute ("Subscription", UintegerValue (1));
		consumerHelper.SetAttribute ("PacketSize", UintegerValue (0));
		consumerHelper.SetAttribute ("Offset", UintegerValue (0));
                conApps = consumerHelper.Install (nodes.Get (phy_nodes[i]));

		comip_index = comip_index + 1;
	}
  }


  //Disable and enable half of the links between com to agg nodes, for a number of times during simulation
  bool DisableLink = true;
  Ptr<Ipv4> ipv4node;
  for (int i=0; i<(int)node_link_map_fail.size(); i++) {
	if (DisableLink) {

                for (int j=0; j<360; j++) {
                        int offset = (rand() % 3599) + 1;
                        ipv4node = nodes.Get(node_link_map_fail[i].first)->GetObject<Ipv4> ();
                        Simulator::Schedule (Seconds ( ((double)offset) ),&Ipv4::SetDown,ipv4node, node_link_map_fail[i].second);
                        Simulator::Schedule (Seconds ( ((double)offset + 0.1) ),&Ipv4::SetUp,ipv4node, node_link_map_fail[i].second);
                }

		DisableLink = false;
	}
	else {
		DisableLink = true;
	}
  }


/*

  //Setting up animation
  //Adding com nodes
  for(int itr = 0; itr < (int)com_nodes.size(); itr++)
  {
        int yaxis = (rand() % 100) + 1;
        AnimationInterface::SetConstantPosition(nodes.Get(com_nodes[itr]), itr*100+100, yaxis);
  }


  //Adding agg nodes
  for(int itr = 0; itr < (int)agg_nodes.size(); itr++)
  {
        int yaxis = (rand() % 270) + 250;
        AnimationInterface::SetConstantPosition(nodes.Get(agg_nodes[itr]), itr*10+10, yaxis);
  }

  //Adding phy nodes
 for(int itr = 0; itr < (int)phy_nodes.size(); itr++)
  {
        int yaxis = (rand() % 1000) + 650;
        if (yaxis > 1000) yaxis = 1000;
        AnimationInterface::SetConstantPosition(nodes.Get(phy_nodes[itr]), itr+1, yaxis);
  }

  AnimationInterface anim("anim-ip-scenario2.xml");
*/

  //Open trace file for writing
  tracefile.open("ip-icens-trace.csv", std::ios::out);
  tracefile << "nodeid, event, name, payloadsize, time" << std::endl;

  std::string strcallback;

  //Trace transmitted packet from [physical nodes]
  for (int i=0; i<(int)phy_nodes.size(); i++) {
	strcallback = "/NodeList/" + std::to_string(phy_nodes[i]) + "/ApplicationList/" + "*/SentPacket";
	Config::ConnectWithoutContext(strcallback, MakeCallback(&SentPacketCallbackPhy));
  }

  //Trace received packet at [compute nodes]
  for (int i=0; i<(int)com_nodes.size(); i++) {
	strcallback = "/NodeList/" + std::to_string(com_nodes[i]) + "/ApplicationList/" + "*/ReceivedPacket";
        Config::ConnectWithoutContext(strcallback, MakeCallback(&ReceivedPacketCallbackCom));
  }

  //Trace sent packet from [compute nodes]
  for (int i=0; i<(int)com_nodes.size(); i++) {
        strcallback = "/NodeList/" + std::to_string(com_nodes[i]) + "/ApplicationList/" + "*/SentPacket";
        Config::ConnectWithoutContext(strcallback, MakeCallback(&SentPacketCallbackCom));
  }

  //Trace received packet at [physical nodes]
  for (int i=0; i<(int)phy_nodes.size(); i++) {
       	strcallback = "/NodeList/" + std::to_string(phy_nodes[i]) + "/ApplicationList/" + "*/ReceivedPacket";
      	Config::ConnectWithoutContext(strcallback, MakeCallback(&ReceivedPacketCallbackPhy));
  }

  //Trace received packet at [aggregation nodes]
  for (int i=0; i<(int)agg_nodes.size(); i++) {
        strcallback = "/NodeList/" + std::to_string(agg_nodes[i]) + "/ApplicationList/" + "*/ReceivedPacket";
        Config::ConnectWithoutContext(strcallback, MakeCallback(&ReceivedPacketCallbackAgg));
  }

  //Trace sent packet at [aggregation nodes]
  for (int i=0; i<(int)agg_nodes.size(); i++) {
        strcallback = "/NodeList/" + std::to_string(agg_nodes[i]) + "/ApplicationList/" + "*/SentPacket";
        Config::ConnectWithoutContext(strcallback, MakeCallback(&SentPacketCallbackAgg));
  }

  //Run actual simulation
  Simulator::Stop (Seconds(3600.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}


//Define callbacks for writing to tracefile
void SentPacketCallbackPhy(uint32_t nodeid, Ptr<Packet> packet, const Address &address, uint32_t seqNo) {

	int packetSize = packet->GetSize ();

	//Remove 4 extra bytes used for subscription
        packetSize = packetSize - 4;

	packet->RemoveAllPacketTags ();
    	packet->RemoveAllByteTags ();

    	//Get subscription value set in packet's header
    	iCenSHeader packetHeader;
    	packet->RemoveHeader(packetHeader);

	if (packetHeader.GetSubscription() == 1 || packetHeader.GetSubscription() == 2) {
		//Do not log subscription packets from phy nodes
	}
	else {
		if (InetSocketAddress::ConvertFrom (address).GetPort () == 5000) {
			tracefile << nodeid << ", sent, " << "/urgent/com/error/phy" << nodeid << "/" << seqNo << ", " << packetSize << ", " << std::fixed
				<< std::setprecision(9) << (Simulator::Now().GetNanoSeconds())/1000000000.0 << std::endl;
		}
                if (InetSocketAddress::ConvertFrom (address).GetPort () == 6000) {
                        tracefile << nodeid << ", sent, " << "/direct/agg/pmu/phy" << nodeid << "/" << seqNo << ", " << packetSize << ", " << std::fixed
                                << std::setprecision(9) << (Simulator::Now().GetNanoSeconds())/1000000000.0 << std::endl;
                }
                if (InetSocketAddress::ConvertFrom (address).GetPort () == 7000) {
                        tracefile << nodeid << ", sent, " << "/direct/agg/ami/phy" << nodeid << "/" << seqNo << ", " << packetSize << ", " << std::fixed
                                << std::setprecision(9) << (Simulator::Now().GetNanoSeconds())/1000000000.0 << std::endl;
                }

	}
}

void ReceivedPacketCallbackCom(uint32_t nodeid, Ptr<Packet> packet, const Address &address, uint32_t localport, uint32_t packetSize, uint32_t subscription, Ipv4Address localip) {

	packet->RemoveAllPacketTags ();
        packet->RemoveAllByteTags ();

        //Get subscription value set in packet's header
        iCenSHeader packetHeader;
        packet->RemoveHeader(packetHeader);

        if (packetHeader.GetSubscription() == 1 || packetHeader.GetSubscription() == 2) {
                //Do not log subscription packet received at com nodes
        }
        else {
                if (localport == 5000) {
                        std::stringstream ss;   std::string str_ip_address;
                        ss << InetSocketAddress::ConvertFrom (address).GetIpv4();
                        ss >> str_ip_address;

                    tracefile << nodeid << ", recv, " << "/urgent/com/error/phy"  << GetNodeFromIP(str_ip_address) << "/" << packet->GetUid () << ", " << packetSize - 2 << ", " << std::fixed
                                << std::setprecision(9) << (Simulator::Now().GetNanoSeconds())/1000000000.0 << std::endl;
                }

                if (localport == 6000) {
                        std::stringstream ss;   std::string str_ip_address;
                        ss << InetSocketAddress::ConvertFrom (address).GetIpv4();
                        ss >> str_ip_address;

                    tracefile << nodeid << ", recv, " << "/direct/com/pmu/agg"  << GetNodeFromIP(str_ip_address) << "/" << packet->GetUid () << ", " << packetSize << ", " << std::fixed
                                << std::setprecision(9) << (Simulator::Now().GetNanoSeconds())/1000000000.0 << std::endl;
                }

                if (localport == 7000) {
                        std::stringstream ss;   std::string str_ip_address;
                        ss << InetSocketAddress::ConvertFrom (address).GetIpv4();
                        ss >> str_ip_address;

                    tracefile << nodeid << ", recv, " << "/direct/com/ami/agg"  << GetNodeFromIP(str_ip_address) << "/" << packet->GetUid () << ", " << packetSize << ", " << std::fixed
                                << std::setprecision(9) << (Simulator::Now().GetNanoSeconds())/1000000000.0 << std::endl;
                }
        }

}

void SentPacketCallbackCom(uint32_t nodeid, Ptr<Packet> packet, const Address &address, uint32_t localport) {

	if (localport !=8000) {
		//Do not log data for error, PMU ACK, AMI ACK from compute
	}
	else {
		tracefile << nodeid << ", sent, " << "/overlay/com/subscription/" << "0" << ", " << packet->GetSize () << ", " << std::fixed
        		<< std::setprecision(9) << (Simulator::Now().GetNanoSeconds())/1000000000.0 << std::endl;
	}
}

void ReceivedPacketCallbackPhy(uint32_t nodeid, Ptr<Packet> packet, const Address &address) {
	if (InetSocketAddress::ConvertFrom (address).GetPort () == 5000) {
		//Do not log packets received in response to urgent payload interest
	}
	else {
		if (InetSocketAddress::ConvertFrom (address).GetPort () == 8000) {
			tracefile << nodeid << ", recv, " << "/overlay/com/subscription/" << "0" << ", " << packet->GetSize () << ", " << std::fixed
				<< std::setprecision(9) << (Simulator::Now().GetNanoSeconds())/1000000000.0 << std::endl;
		}
	}
}

void ReceivedPacketCallbackAgg(uint32_t nodeid, Ptr<Packet> packet, const Address &address,  uint32_t localport, uint32_t seqNo) {

	int packetSize = packet->GetSize ();

        //Extra 4 bytes already removed at AGG app layer while checking if sequence number is greater than 100
        
	if (localport == 6000) {
     	        std::stringstream ss;   std::string str_ip_address;
                ss << InetSocketAddress::ConvertFrom (address).GetIpv4();
                ss >> str_ip_address;

        	tracefile << nodeid << ", recv, " << "/direct/agg/pmu/phy" << GetNodeFromIP(str_ip_address) << "/" << seqNo << ", " << packetSize << ", " << std::fixed 
			<< std::setprecision(9) << (Simulator::Now().GetNanoSeconds())/1000000000.0 << std::endl;
	}

        if (localport == 7000) {
                std::stringstream ss;   std::string str_ip_address;
                ss << InetSocketAddress::ConvertFrom (address).GetIpv4();
                ss >> str_ip_address;

                tracefile << nodeid << ", recv, " << "/direct/agg/ami/phy" << GetNodeFromIP(str_ip_address) << "/" << seqNo << ", " << packetSize << ", " << std::fixed
                        << std::setprecision(9) << (Simulator::Now().GetNanoSeconds())/1000000000.0 << std::endl;
        }

}

void SentPacketCallbackAgg(uint32_t nodeid, Ptr<Packet> packet, const Address &address, uint32_t seqNo) {

	int packetSize = packet->GetSize ();

        //Remove 4 extra bytes used for sequence number in subscription field
        packetSize = packetSize - 4;

	if (InetSocketAddress::ConvertFrom (address).GetPort () == 6000) {
        	tracefile << nodeid << ", sent, " << "/direct/com/pmu/agg" << nodeid << "/" << seqNo << ", " << packetSize << ", " << std::fixed
			<< std::setprecision(9) << (Simulator::Now().GetNanoSeconds())/1000000000.0 << std::endl;
	}

        if (InetSocketAddress::ConvertFrom (address).GetPort () == 7000) {
                tracefile << nodeid << ", sent, " << "/direct/com/ami/agg" << nodeid << "/" << seqNo << ", " << packetSize << ", " << std::fixed
                        << std::setprecision(9) << (Simulator::Now().GetNanoSeconds())/1000000000.0 << std::endl;
        }
}


bool NodeInComm(int nodeid) {
        for (int i=0; i<(int)com_nodes.size(); i++) {
                if (com_nodes[i] == nodeid) {
                        return true;
                }
        }
        return false;
}

bool NodeInAgg(int nodeid) {
        for (int i=0; i<(int)agg_nodes.size(); i++) {
                if (agg_nodes[i] == nodeid) {
                        return true;
                }
        }
        return false;
}

bool NodeInPhy(int nodeid) {
        for (int i=0; i<(int)phy_nodes.size(); i++) {
                if (phy_nodes[i] == nodeid) {
                        return true;
                }
        }
        return false;
}

int GetNodeFromIP(std::string str_ip) {
	for (int i=0; i<(int)node_ip_map.size(); i++) {
		if (node_ip_map[i].second == str_ip) {
			return node_ip_map[i].first;
		}
	}
	std::cout << "IP address " << str_ip << " not mapped to any node!!! Cannot write correct entry in tracefile" << std::endl;
	exit(1);
}

std::string GetAggIP(int nodeid) {
	for (int i=0; i<(int)phy_to_agg_map.size(); i++) {
		if (nodeid == phy_to_agg_map[i].first) {
			return phy_to_agg_map[i].second;
		}
	}
	std::cout << "Node " << nodeid << ", not connected to any aggregation IP!!!!!" << std::endl;
	exit(1);
}

} //namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}
