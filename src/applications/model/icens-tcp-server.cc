/* -*- Mode:C  ; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
 /*
  * Copyright 2012
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
 
 #include "ns3/log.h"
 #include "ns3/ipv4-address.h"
 #include "ns3/ipv6-address.h"
 #include "ns3/address-utils.h"
 #include "ns3/nstime.h"
 #include "ns3/inet-socket-address.h"
 #include "ns3/inet6-socket-address.h"
 #include "ns3/socket.h"
 #include "ns3/simulator.h"
 #include "ns3/socket-factory.h"
 #include "ns3/packet.h"
 #include "ns3/uinteger.h"
 #include "ns3/icens-header.h" 
 #include "ns3/trace-source-accessor.h"
 #include "ns3/ipv4.h"
 #include "icens-tcp-server.h"
 
 namespace ns3 {
 
 NS_LOG_COMPONENT_DEFINE ("ns3.iCenSTCPServer");
 NS_OBJECT_ENSURE_REGISTERED (iCenSTCPServer);
 
 TypeId
 iCenSTCPServer::GetTypeId (void)
 {
   static TypeId tid = TypeId ("ns3::iCenSTCPServer")
     .SetParent<Application> ()
     .AddConstructor<iCenSTCPServer> ()
     .AddAttribute ("Port", "Port on which we listen for incoming connections.",
                    UintegerValue (7),
                    MakeUintegerAccessor (&iCenSTCPServer::m_local_port),
                    MakeUintegerChecker<uint16_t> ())
     .AddAttribute ("RemoteAddress",
                   "The destination Address of the outbound packet",
                   AddressValue (),
                   MakeAddressAccessor (&iCenSTCPServer::m_remote_address),
                   MakeAddressChecker ())
     .AddAttribute ("PacketSize",
                   "The size of outbound packet, typically acknowledgement packets from server application. 536 not fragmented on 1500 MTU",
                   UintegerValue (536),
                   MakeUintegerAccessor (&iCenSTCPServer::m_packet_size),
		   MakeUintegerChecker<uint32_t> ())
     .AddTraceSource ("ReceivedPacket",
                     "A packet has been received",
                     MakeTraceSourceAccessor (&iCenSTCPServer::m_receivedPacket),
                     "ns3::iCenSTCPServer::ReceivedPacketTraceCallback")
     .AddTraceSource ("SentPacket",
                     "A packet has been sent",
                     MakeTraceSourceAccessor (&iCenSTCPServer::m_sentPacket),
                     "ns3::iCenSTCPServer::SentPacketTraceCallback")
   ;
   return tid;
 }
 
iCenSTCPServer::iCenSTCPServer ()
 {
   NS_LOG_FUNCTION_NOARGS ();
   m_socket = 0;
   m_socket6 = 0;
   m_running = false;
   m_subscription = 0;
   //m_remote_address;
 }
 
iCenSTCPServer::~iCenSTCPServer()
 {
   NS_LOG_FUNCTION_NOARGS ();
   m_socket = 0;
   m_socket6 = 0;
 }
 
void
iCenSTCPServer::DoDispose (void)
 {
   NS_LOG_FUNCTION_NOARGS ();
   Application::DoDispose ();
 }
 
void
iCenSTCPServer::StartApplication (void)
 {
   NS_LOG_FUNCTION_NOARGS ();
 
   m_running = true;
 
   if (m_socket == 0)
     {
       TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
       m_socket = Socket::CreateSocket (GetNode (), tid);
       InetSocketAddress listenAddress = InetSocketAddress (Ipv4Address::GetAny (), m_local_port);
       m_socket->Bind (listenAddress);
       m_socket->Listen();
     }
   if (m_socket6 == 0)
     {
       TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
       m_socket6 = Socket::CreateSocket (GetNode (), tid);
       Inet6SocketAddress listenAddress = Inet6SocketAddress (Ipv6Address::GetAny (), m_local_port);
       m_socket6->Bind (listenAddress);
       m_socket6->Listen();
     }
 
   m_socket->SetAcceptCallback (
         MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
         MakeCallback (&iCenSTCPServer::HandleAccept, this));
   m_socket6->SetAcceptCallback (
         MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
         MakeCallback (&iCenSTCPServer::HandleAccept, this));
 }
 
void
iCenSTCPServer::StopApplication ()
 {
   NS_LOG_FUNCTION_NOARGS ();
 
   m_running = false;
 
   if (m_socket != 0)
     {
       m_socket->Close ();
       m_socket->SetAcceptCallback (
             MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
             MakeNullCallback<void, Ptr<Socket>, const Address &> () );
     }
   if (m_socket6 != 0)
     {
       m_socket6->Close ();
       m_socket6->SetAcceptCallback (
             MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
             MakeNullCallback<void, Ptr<Socket>, const Address &> () );
     }
 }
 
void
iCenSTCPServer::ReceivePacket (Ptr<Socket> s)
 {

   NS_LOG_FUNCTION (this << s);
 
   Ptr<Packet> packet;
   //Address from;

   while (packet = s->RecvFrom (m_remote_address))
     {
       if (packet->GetSize () > 0)
         {

	//Get the first interface IP attached to this node (this is where socket is bound, true nodes that have only 1 IP)
    	//Ptr<NetDevice> PtrNetDevice = PtrNode->GetDevice(0);
    	Ptr <Node> PtrNode = this->GetNode();
    	Ptr<Ipv4> ipv4 = PtrNode->GetObject<Ipv4> (); 
    	Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1,0);  
    	m_local_ip = iaddr.GetLocal (); 

     	  NS_LOG_INFO ("Server Received " << packet->GetSize () << " bytes from " << InetSocketAddress::ConvertFrom (m_remote_address).GetIpv4 ()
		<< ":" << InetSocketAddress::ConvertFrom (m_remote_address).GetPort ());
	 
     	  packet->RemoveAllPacketTags ();
     	  packet->RemoveAllByteTags ();

	//Get subscription value set in packet's payload
    	iCenSHeader packetHeader;
    	packet->RemoveHeader(packetHeader);
    	m_subscription = packetHeader.GetSubscription();
    	NS_LOG_INFO("SUBSCRIPTION value = " << m_subscription);
    
    	//Send packet through the socket
    	if (m_subscription >= 100 ) {
		//1-byte byte acknowledgement, for packets with sequence numbers (m_subscription >=100)
		m_packet_size = 1;
   	}
    	else if (m_subscription == 1 || m_subscription == 2) {
/*
        //Soft or Hard subsription set
	if (m_frequency != 0) {
		//Application instance is set for demand-response flow, send separate packet to each client that subscribes
		ScheduleTransmit(socket, m_remote_address);
	}
  */
	} 
	  
     	  NS_LOG_LOGIC ("Sending reply packet " << packet->GetSize ());
	  //Keep original packet to send to trace file and get correct size
          Ptr<Packet> packet_copy = Create<Packet> (m_packet_size);
     	  s->Send (packet_copy);

          // Callback for received packet
    	  m_receivedPacket (GetNode()->GetId(), packet, m_remote_address, m_local_port, m_subscription, m_local_ip);
         }
     }
 }
 
void iCenSTCPServer::HandleAccept (Ptr<Socket> s, const Address& from)
 {
   NS_LOG_FUNCTION (this << s << from);
   s->SetRecvCallback (MakeCallback (&iCenSTCPServer::ReceivePacket, this));
   s->SetCloseCallbacks(MakeCallback (&iCenSTCPServer::HandleSuccessClose, this),
     MakeNullCallback<void, Ptr<Socket> > () );
 }
 
void iCenSTCPServer::HandleSuccessClose(Ptr<Socket> s)
 {
   NS_LOG_FUNCTION (this << s);
   NS_LOG_LOGIC ("Client close received");
   s->Close();
   s->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > () );
   s->SetCloseCallbacks(MakeNullCallback<void, Ptr<Socket> > (),
       MakeNullCallback<void, Ptr<Socket> > () );
 }
 
 } // Namespace ns3

