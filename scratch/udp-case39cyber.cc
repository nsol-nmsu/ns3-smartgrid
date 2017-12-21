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
#include <time.h>

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


void SentPacketCallbackPhy(uint32_t, Ptr<Packet>, const Address &, uint32_t);
void ReceivedPacketCallbackCom(uint32_t, Ptr<Packet>, const Address &, uint32_t, uint32_t subscription, Ipv4Address local_ip);

void SentPacketCallbackCom(uint32_t, Ptr<Packet>, const Address &, uint32_t);
void ReceivedPacketCallbackPhy(uint32_t, Ptr<Packet>, const Address &);

void SentPacketCallbackAgg(uint32_t, Ptr<Packet>, const Address &, uint32_t);
void ReceivedPacketCallbackAgg(uint32_t, Ptr<Packet>, const Address &, uint32_t localport, uint32_t seqNo);

// Vectors to store the various node types
std::vector<int> com_nodes, agg_nodes, phy_nodes;

// Vectors to store source and destination edges to fail
std::vector<int> srcedge, dstedge;

// Vectors to store agg<--->com and phy<--->agg p2p IPs, packets are directed to these IP addresses
std::vector<std::string> com_ips;
std::vector<std::pair<int,std::string>> phy_to_agg_map;

// Nodes and their corresponding IPs, used to output namespaces in tracefile
std::vector<std::pair<int,std::string>> node_ip_map;
std::vector<std::pair<int,std::string>> wac_ip_map;
std::vector<std::pair<int,std::string>> pdc_ip_map;
std::vector<std::pair<int,std::string>> pmu_ip_map;

// Node and interface pairs to fail, between com and agg
std::vector<std::pair<int,int>> node_link_map_fail;

// Split a line into a vector of strings
std::vector<std::string> SplitString(std::string);

//Store unique IDs to prevent repeated installation of app
bool IsPDCAppInstalled(std::string PDCID);
bool IsWACAppInstalled(std::string WACID);

//Get node IP from subnet defined in topology file
std::string GetIPFromSubnet(std::string nodeid, std::string subnet);

std::string GetPDCIP(int pdcid);
std::string GetWACIP(int wacid);

std::vector<std::string> uniquePDCs;
std::vector<std::string> uniqueWACs;

bool NodeInComm(int);
bool NodeInAgg(int);
bool NodeInPhy(int);
int GetNodeFromIP(std::string);
std::string GetAggIP(int);

//Trace file
std::ofstream tracefile;

std::ofstream flowfile;

//Gets current time to make packets unique
time_t seconds;

int main (int argc, char *argv[])
{

  CommandLine cmd;
  cmd.Parse (argc, argv);

  // Open the configuration file for reading
  std::ifstream configFile ("src/ndnSIM/examples/case39Cyber.txt", std::ios::in);

  std::string strLine;
  bool gettingNodeCount = false, buildingNetworkTopo = false, attachingWACs = false, attachingPMUs = false, attachingPDCs = false, flowPMUtoPDC = false, flowPMUtoWAC = false;
  bool failLinks = false, injectData = false;
  std::vector<std::string> netParams;

  NodeContainer nodes;
  int nodeCount = 0;

  // Setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("10"));

  PointToPointHelper p2p;
  NetDeviceContainer devices;
  Ipv4AddressHelper addresses;

  std::pair<int,std::string> node_ip_pair;

  ProducerHelper proHelper;
  ApplicationContainer proApps;
  SubscriberHelper consumerHelper;
  ApplicationContainer conApps;

  uint16_t wacPort = 5000;
  uint16_t pdcPort = 6000;
  uint16_t bgdPort = 1000;

  Ptr<Ipv4> ipv4node;

  flowfile.open("ip_all_flows.csv", std::ios::out);

  if (configFile.is_open ()) {
        while (std::getline(configFile, strLine)) {

                //Determine what operation is ongoing while reading the config file
		if(strLine.substr(0,7) == "BEG_000") { gettingNodeCount = true; continue; }
                if(strLine.substr(0,7) == "END_000") { 
			//Create nodes
			nodeCount++; //Counter increment needed because nodes start from 1 not 0
			gettingNodeCount = false;
			nodes.Create(nodeCount);
			//Install internet protocol stack on nodes
			InternetStackHelper stack1;
  			stack1.Install (nodes);

			continue; 
		}
                if(strLine.substr(0,7) == "BEG_001") { buildingNetworkTopo = true; continue; }
                if(strLine.substr(0,7) == "END_001") { buildingNetworkTopo = false; continue; }
		if(strLine.substr(0,7) == "BEG_002") { attachingWACs = true; continue; }
                if(strLine.substr(0,7) == "END_002") { attachingWACs = false; continue; }
		if(strLine.substr(0,7) == "BEG_003") { attachingPMUs = true; continue; }
                if(strLine.substr(0,7) == "END_003") { attachingPMUs = false; continue; }
		if(strLine.substr(0,7) == "BEG_004") { attachingPDCs = true; continue; }
                if(strLine.substr(0,7) == "END_004") { attachingPDCs = false; continue; }
		if(strLine.substr(0,7) == "BEG_005") { flowPMUtoPDC = true; continue; }
                if(strLine.substr(0,7) == "END_005") { flowPMUtoPDC = false; continue; }
		if(strLine.substr(0,7) == "BEG_006") { flowPMUtoWAC = true; continue; }
                if(strLine.substr(0,7) == "END_006") { flowPMUtoWAC = false; continue; }
		if(strLine.substr(0,7) == "BEG_100") { failLinks = true; continue; }
                if(strLine.substr(0,7) == "END_100") { failLinks = false; continue; }
		if(strLine.substr(0,7) == "BEG_101") { injectData = true; continue; }
                if(strLine.substr(0,7) == "END_101") { injectData = false; continue; }


		if(gettingNodeCount == true) {
                        //Getting number of nodes to create
                        netParams = SplitString(strLine);
			nodeCount = stoi(netParams[1]);
		}
		else if(buildingNetworkTopo == true) {
                        //Building network topology
                        netParams = SplitString(strLine);
                        p2p.SetDeviceAttribute("DataRate", StringValue(netParams[2]+"Mbps"));
                        p2p.SetChannelAttribute("Delay", StringValue(netParams[3]+"ms"));
          		devices = p2p.Install(nodes.Get(std::stoi(netParams[0])), nodes.Get(std::stoi(netParams[1])));
			addresses.SetBase (Ipv4Address (netParams[4].c_str()), Ipv4Mask(netParams[5].c_str()));
  			addresses.Assign (devices);
                }
		else if(attachingWACs == true) {
                        //Attaching WACs to routers
                        netParams = SplitString(strLine);
                        p2p.SetDeviceAttribute("DataRate", StringValue(netParams[2]+"Mbps"));
                        p2p.SetChannelAttribute("Delay", StringValue(netParams[3]+"ms"));
                        devices = p2p.Install(nodes.Get(std::stoi(netParams[0])), nodes.Get(std::stoi(netParams[1])));
                        addresses.SetBase (Ipv4Address (netParams[4].c_str()), Ipv4Mask(netParams[5].c_str()));
                        addresses.Assign (devices);

			//Store WAC ID and its IP
			node_ip_pair.first = std::stoi(netParams[1]);
			node_ip_pair.second = GetIPFromSubnet(netParams[1], netParams[4]);
			wac_ip_map.push_back(node_ip_pair);
		}
		else if(attachingPMUs == true) {
                        //Attaching PMUs to routers
                        netParams = SplitString(strLine);
                        p2p.SetDeviceAttribute("DataRate", StringValue(netParams[2]+"Mbps"));
                        p2p.SetChannelAttribute("Delay", StringValue(netParams[3]+"ms"));
                        devices = p2p.Install(nodes.Get(std::stoi(netParams[0])), nodes.Get(std::stoi(netParams[1])));
                        addresses.SetBase (Ipv4Address (netParams[4].c_str()), Ipv4Mask(netParams[5].c_str()));
                        addresses.Assign (devices);

			//Store PMU ID and its IP
                        node_ip_pair.first = std::stoi(netParams[1]);
                        node_ip_pair.second = GetIPFromSubnet(netParams[1], netParams[4]);
                        pmu_ip_map.push_back(node_ip_pair);
                }
		else if(attachingPDCs == true) {
                        //Attaching PDCs to routers
                        netParams = SplitString(strLine);
                        p2p.SetDeviceAttribute("DataRate", StringValue(netParams[2]+"Mbps"));
                        p2p.SetChannelAttribute("Delay", StringValue(netParams[3]+"ms"));
                        devices = p2p.Install(nodes.Get(std::stoi(netParams[0])), nodes.Get(std::stoi(netParams[1])));
                        addresses.SetBase (Ipv4Address (netParams[4].c_str()), Ipv4Mask(netParams[5].c_str()));
                        addresses.Assign (devices);

			//Store WAC ID and its IP
                        node_ip_pair.first = std::stoi(netParams[1]);
                        node_ip_pair.second = GetIPFromSubnet(netParams[1], netParams[4]);
                        pdc_ip_map.push_back(node_ip_pair);
                }
		else if(flowPMUtoPDC == true) {
			//Install apps on PDCs and PMUs for data exchange
			netParams = SplitString(strLine);

			//Install app on unique PDC IDs
			if (IsPDCAppInstalled(netParams[0]) == false) {
				proHelper.SetAttribute ("LocalPort", UintegerValue (pdcPort));
        			proHelper.SetAttribute ("Frequency", TimeValue (Seconds (0.0)));
        			proApps = proHelper.Install (nodes.Get (std::stoi(netParams[0])));
			}

			//Install flow app on PMUs to send data to PDCs
                	consumerHelper.SetAttribute ("RemoteAddress", AddressValue (InetSocketAddress (Ipv4Address(GetPDCIP(std::stoi(netParams[0])).c_str()), pdcPort)));
                	consumerHelper.SetAttribute ("Frequency", TimeValue (Seconds (0.02))); //0.016 or 0.02
                	consumerHelper.SetAttribute ("Subscription", UintegerValue (0));
                	consumerHelper.SetAttribute ("PacketSize", UintegerValue (200));
                	//int offset = (rand() % 91) + 1;
                	consumerHelper.SetAttribute ("Offset", UintegerValue (0));
                	conApps = consumerHelper.Install (nodes.Get (std::stoi(netParams[1])));

			//Write flow to file
			flowfile << netParams[0] << " " << netParams[1] << " PDC" << std::endl;
		}
		else if(flowPMUtoWAC == true) {
                        //Install apps on WACs and PMUs for data exchange
                        netParams = SplitString(strLine);

                        //Install app on unique PDC IDs
                        if (IsWACAppInstalled(netParams[0]) == false) {
                                proHelper.SetAttribute ("LocalPort", UintegerValue (wacPort));
                                proHelper.SetAttribute ("Frequency", TimeValue (Seconds (0.0)));
                                proApps = proHelper.Install (nodes.Get (std::stoi(netParams[0])));
                        }

                        //Install flow app on PMUs to send data to WACs
                        consumerHelper.SetAttribute ("RemoteAddress", AddressValue (InetSocketAddress (Ipv4Address(GetWACIP(std::stoi(netParams[0])).c_str()), wacPort)));
                        consumerHelper.SetAttribute ("Frequency", TimeValue (Seconds (0.02))); //0.016 or 0.02
                        consumerHelper.SetAttribute ("Subscription", UintegerValue (0));
                        consumerHelper.SetAttribute ("PacketSize", UintegerValue (200));
                        //int offset = (rand() % 91) + 1;
                        consumerHelper.SetAttribute ("Offset", UintegerValue (0));
                        conApps = consumerHelper.Install (nodes.Get (std::stoi(netParams[1])));

			//Write flow to file
			flowfile << netParams[0] << " " << netParams[1] << " WAC" << std::endl;

                }
		else if(failLinks == true) {
                        //Fail links specified in the config file
                        netParams = SplitString(strLine);

			ipv4node = nodes.Get(stoi(netParams[1]))->GetObject<Ipv4> ();
                        Simulator::Schedule (Seconds ( ((double)stod(netParams[2])) ),&Ipv4::SetDown,ipv4node, stoi(netParams[4]));
                        Simulator::Schedule (Seconds ( ((double)stod(netParams[3])) ),&Ipv4::SetUp,ipv4node, stoi(netParams[4]));

		}
		else if(injectData == true) {
                        //Install apps on WACs, PDCs and PMUs for background data injection
                        netParams = SplitString(strLine);

			//Install app on target node to be used for background data injection
                        proHelper.SetAttribute ("LocalPort", UintegerValue (bgdPort));
                        proHelper.SetAttribute ("Frequency", TimeValue (Seconds (0.0)));
                        proApps = proHelper.Install (nodes.Get (std::stoi(netParams[0])));

			std::string targetDOS = "";
			if (netParams[4] == "PDC") targetDOS = GetPDCIP(std::stoi(netParams[0]));
			if (netParams[4] == "WAC") targetDOS = GetWACIP(std::stoi(netParams[0]));

                        //Install flow app on PMUs to send background data to target node
                        consumerHelper.SetAttribute ("RemoteAddress", AddressValue (InetSocketAddress (Ipv4Address(targetDOS.c_str()), bgdPort)));
                        consumerHelper.SetAttribute ("Frequency", TimeValue (Seconds (0.001))); //0.001 = 1000pps
                        consumerHelper.SetAttribute ("Subscription", UintegerValue (0));
                        consumerHelper.SetAttribute ("PacketSize", UintegerValue (1024));
                        consumerHelper.SetAttribute ("Offset", UintegerValue (0));
                        ApplicationContainer bgdApps = consumerHelper.Install (nodes.Get (std::stoi(netParams[1])));
			bgdApps.Start (Seconds (std::stod(netParams[2])));
			bgdApps.Stop (Seconds (std::stod(netParams[3])));
		}
		else {
			//std::cout << "reading something else " << strLine << std::endl;
		}

	}

  }
  else {
	std::cout << "Cannot open configuration file!!!" << std::endl;
	exit(1);
  }

  configFile.close();

  //Populate the routing table
  Ipv4GlobalRoutingHelper::PopulateRoutingTables();


  //Open trace file for writing
  tracefile.open("udp-case39cyber-trace.csv", std::ios::out);
  tracefile << "nodeid, event, name, payloadsize, time" << std::endl;

  std::string strcallback;

  //Trace transmitted packet from [PMU nodes]
  for (int i=0; i<(int)pmu_ip_map.size(); i++) {
	strcallback = "/NodeList/" + std::to_string(pmu_ip_map[i].first) + "/ApplicationList/" + "*/SentPacket";
	Config::ConnectWithoutContext(strcallback, MakeCallback(&SentPacketCallbackPhy));
  }

  //Trace received packet at [PDC nodes]
  for (int i=0; i<(int)pdc_ip_map.size(); i++) {
	strcallback = "/NodeList/" + std::to_string(pdc_ip_map[i].first) + "/ApplicationList/" + "*/ReceivedPacket";
        Config::ConnectWithoutContext(strcallback, MakeCallback(&ReceivedPacketCallbackCom));
  }

  //Trace received packet at [WAC nodes]
  for (int i=0; i<(int)wac_ip_map.size(); i++) {
        strcallback = "/NodeList/" + std::to_string(wac_ip_map[i].first) + "/ApplicationList/" + "*/ReceivedPacket";
        Config::ConnectWithoutContext(strcallback, MakeCallback(&ReceivedPacketCallbackCom));
  }

  //Trace sent packet from [compute nodes]
//  for (int i=0; i<(int)com_nodes.size(); i++) {
//        strcallback = "/NodeList/" + std::to_string(com_nodes[i]) + "/ApplicationList/" + "*/SentPacket";
//        Config::ConnectWithoutContext(strcallback, MakeCallback(&SentPacketCallbackCom));
//  }

  //Trace received packet at [physical nodes]
//  for (int i=0; i<(int)phy_nodes.size(); i++) {
//       	strcallback = "/NodeList/" + std::to_string(phy_nodes[i]) + "/ApplicationList/" + "*/ReceivedPacket";
//      	Config::ConnectWithoutContext(strcallback, MakeCallback(&ReceivedPacketCallbackPhy));
//  }

  //Trace received packet at [aggregation nodes]
//  for (int i=0; i<(int)agg_nodes.size(); i++) {
//        strcallback = "/NodeList/" + std::to_string(agg_nodes[i]) + "/ApplicationList/" + "*/ReceivedPacket";
//        Config::ConnectWithoutContext(strcallback, MakeCallback(&ReceivedPacketCallbackAgg));
//  }

  //Trace sent packet at [aggregation nodes]
//  for (int i=0; i<(int)agg_nodes.size(); i++) {
//        strcallback = "/NodeList/" + std::to_string(agg_nodes[i]) + "/ApplicationList/" + "*/SentPacket";
//        Config::ConnectWithoutContext(strcallback, MakeCallback(&SentPacketCallbackAgg));
//  }


  //Run actual simulation
  Simulator::Stop (Seconds(5.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

//Split a string delimited by space
std::vector<std::string> SplitString(std::string strLine) {
        std::string str = strLine;
        std::vector<std::string> result;
        std::istringstream isstr(str);
        for(std::string str; isstr >> str; )
                result.push_back(str);

        return result;
}

//Store unique PDC IDs to prevent repeated installation of app
bool IsPDCAppInstalled(std::string PDCID) {
	for (int i=0; i<(int)uniquePDCs.size(); i++) {
		if(PDCID.compare(uniquePDCs[i]) == 0) {
			return true;
		}
	}

	uniquePDCs.push_back(PDCID);
	return false;
}

//Store unique WAC IDs to prevent repeated installation of app
bool IsWACAppInstalled(std::string WACID) {
        for (int i=0; i<(int)uniqueWACs.size(); i++) {
                if(WACID.compare(uniqueWACs[i]) == 0) {
                        return true;
                }
        }

        uniqueWACs.push_back(WACID);
        return false;
}

//Get node IP from subnet defined in topology file
std::string GetIPFromSubnet(std::string nodeid, std::string subnet) {

	std::string node_p2p_ip = "";
	std::pair<int,std::string> all_nodes_ip_pair;

 	std::size_t dotposition = subnet.find(".",0) + 1;
        node_p2p_ip = node_p2p_ip + subnet.substr(0,dotposition);

        std::string ipsecondtofourthoctet = subnet.substr( dotposition, std::string::npos );
        dotposition = ipsecondtofourthoctet.find(".",0) + 1;
        node_p2p_ip = node_p2p_ip + ipsecondtofourthoctet.substr(0,dotposition);

        std::string ipthirdtofourthoctet = ipsecondtofourthoctet.substr( dotposition, std::string::npos );
        dotposition = ipthirdtofourthoctet.find(".",0) + 1;
        node_p2p_ip = node_p2p_ip + ipthirdtofourthoctet.substr(0,dotposition);

        std::string ipfourthoctet = ipthirdtofourthoctet.substr( (ipthirdtofourthoctet.find(".",0))+1, std::string::npos );
        node_p2p_ip = node_p2p_ip + std::to_string(std::stoi(ipfourthoctet) + 2);

	//Store all node and their IPs, used for trace file
	all_nodes_ip_pair.first = std::stoi(nodeid);
	all_nodes_ip_pair.second = node_p2p_ip;
	node_ip_map.push_back(all_nodes_ip_pair);

	return node_p2p_ip;
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
		std::stringstream ss;   std::string str_ip_address;
                        ss << InetSocketAddress::ConvertFrom (address).GetIpv4();
                        ss >> str_ip_address;

		if (InetSocketAddress::ConvertFrom (address).GetPort () == 5000) {
			tracefile << nodeid << ", sent, " << "/power/wac/phy" << nodeid << "/" << InetSocketAddress::ConvertFrom (address).GetIpv4 () << "/" << GetNodeFromIP(str_ip_address) << "/" <<
				seqNo << ", " << packetSize << ", " << std::fixed << std::setprecision(9) << (Simulator::Now().GetNanoSeconds())/1000000000.0 << std::endl;
		}
                if (InetSocketAddress::ConvertFrom (address).GetPort () == 6000) {
                        tracefile << nodeid << ", sent, " << "/power/pdc/phy" << nodeid << "/" << InetSocketAddress::ConvertFrom (address).GetIpv4 () << "/" << GetNodeFromIP(str_ip_address) << "/" << 
				seqNo << ", " << packetSize << ", " << std::fixed << std::setprecision(9) << (Simulator::Now().GetNanoSeconds())/1000000000.0 << std::endl;
                }
		if (InetSocketAddress::ConvertFrom (address).GetPort () == 1000) {
                        tracefile << nodeid << ", sent, " << "/power/bgd/phy" << nodeid << "/" << InetSocketAddress::ConvertFrom (address).GetIpv4 () << "/" << GetNodeFromIP(str_ip_address) << "/" <<
                                seqNo << ", " << packetSize << ", " << std::fixed << std::setprecision(9) << (Simulator::Now().GetNanoSeconds())/1000000000.0 << std::endl;
                }
                if (InetSocketAddress::ConvertFrom (address).GetPort () == 7000) {
                        tracefile << nodeid << ", sent, " << "/direct/agg/ami/phy" << nodeid << "/" << seqNo << ", " << packetSize << ", " << std::fixed
                                << std::setprecision(9) << (Simulator::Now().GetNanoSeconds())/1000000000.0 << std::endl;
                }

	}
}

void ReceivedPacketCallbackCom(uint32_t nodeid, Ptr<Packet> packet, const Address &address, uint32_t localport, uint32_t subscription, Ipv4Address local_ip) {

	int packetSize = packet->GetSize();

	//Extra 4 bytes already removed at COM app layer while checking if sequence number is greater than 100

	if (subscription == 1 || subscription == 2) {
		//Do not log subscription packet received at com nodes
	}
	else {
		//At this point "subscription" parameter contains sequence number of packet
		if (localport == 5000) {
                	std::stringstream ss;   std::string str_ip_address;
                	ss << InetSocketAddress::ConvertFrom (address).GetIpv4();
                	ss >> str_ip_address;

		    tracefile << nodeid << ", recv, " << "/power/wac/phy"  << GetNodeFromIP(str_ip_address) << "/" << local_ip << "/" << nodeid << "/" << subscription << ", " << packetSize << ", " 
			<< std::fixed << std::setprecision(9) << (Simulator::Now().GetNanoSeconds())/1000000000.0 << std::endl;
		}

                if (localport == 6000) {
                        std::stringstream ss;   std::string str_ip_address;
                        ss << InetSocketAddress::ConvertFrom (address).GetIpv4();
                        ss >> str_ip_address;

                    tracefile << nodeid << ", recv, " << "/power/pdc/phy"  << GetNodeFromIP(str_ip_address) << "/" << local_ip << "/" << nodeid << "/" << subscription << ", " << packetSize << ", " 
			<< std::fixed << std::setprecision(9) << (Simulator::Now().GetNanoSeconds())/1000000000.0 << std::endl;
                }

		if (localport == 1000) {
                        std::stringstream ss;   std::string str_ip_address;
                        ss << InetSocketAddress::ConvertFrom (address).GetIpv4();
                        ss >> str_ip_address;

                    tracefile << nodeid << ", recv, " << "/power/bgd/phy"  << GetNodeFromIP(str_ip_address) << "/" << local_ip << "/" << nodeid << "/" << subscription << ", " << packetSize << ", "
                        << std::fixed << std::setprecision(9) << (Simulator::Now().GetNanoSeconds())/1000000000.0 << std::endl;
                }

                if (localport == 7000) {
                        std::stringstream ss;   std::string str_ip_address;
                        ss << InetSocketAddress::ConvertFrom (address).GetIpv4();
                        ss >> str_ip_address;

                    tracefile << nodeid << ", recv, " << "/direct/com/ami/agg"  << GetNodeFromIP(str_ip_address) << "/" << subscription << ", " << packetSize << ", " << std::fixed
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

std::string GetPDCIP(int pdcid) {
        for (int i=0; i<(int)pdc_ip_map.size(); i++) {
                if (pdcid == pdc_ip_map[i].first) {
                        return pdc_ip_map[i].second;
                }
        }
        std::cout << "PDC " << pdcid << ", has no corresponding IP!!!!!" << std::endl;
        
	exit(1);
}

std::string GetWACIP(int wacid) {
        for (int i=0; i<(int)wac_ip_map.size(); i++) {
                if (wacid == wac_ip_map[i].first) {
                        return wac_ip_map[i].second;
                }
        }
        std::cout << "WAC " << wacid << ", has no corresponding IP!!!!!" << std::endl;
        
	exit(1);
}


} //namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}
