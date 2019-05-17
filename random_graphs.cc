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

#include "ns3/simulator.h"
#include "ns3/nstime.h"
#include "ns3/command-line.h"
#include "ns3/random-variable-stream.h"
#include <iostream>

#include <fstream>
#include <string>
#include <cassert>


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("FirstScriptExample");

int
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);
  
  Time::SetResolution (Time::NS);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  //N<=16,because 16*15=240<255,if N>16,k will bigger than 255
  int N=20;
  // p is probability of whether two vertices have edge
  float p=0.8;

  //MAC layer
  NodeContainer nodes;
  nodes.Create (N);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  //ip layer
  InternetStackHelper stack;
  stack.Install (nodes);
  Address serverAddress;
  //k is ralated to ip address
  int k=1;
  bool isserveraddress =true;
  // n,m is ralated to the number of list
  int n=0;
  int m=0;

  std::vector<NodeContainer> nodeAdjacencyList (N*(N-1));
  std::vector<NetDeviceContainer> deviceAdjacencyList (N*(N-1));
  std::vector<Ipv4InterfaceContainer> interfaceList (N*(N-1));

  for(int i=0;i<N-1;i++)
  {
	  for(int j=0;j<N-1-i;j++)
	  {

		  Ptr<UniformRandomVariable> v = CreateObject<UniformRandomVariable> ();

		  // let source and destination not directly connect
		  if (i==0 and j==0)
		  {
			  continue;
		  }

		  // use random numbers to realize random edges
		  if (v->GetValue ()<p)
		  {
			  nodeAdjacencyList[n] = NodeContainer (nodes.Get(i), nodes.Get(N-1-j));

			  deviceAdjacencyList[n] = pointToPoint.Install (nodeAdjacencyList[n]);

			  Ipv4AddressHelper address;

			  std::ostringstream subnet;
			  subnet<<"10.1."<<k<<".0";
			  k++;
			  address.SetBase (subnet.str ().c_str (), "255.255.255.0");

			  // set server ip address
			  if(i==0 and isserveraddress)
			  {
				  isserveraddress = false;
				  Ipv4InterfaceContainer interfaces = address.Assign (deviceAdjacencyList[n]);
				  serverAddress = Address(interfaces.GetAddress (0));
			  }
			  else
			  {
				  //equal to set up a gateway,so that data packets can be forwarded
				  interfaceList[m] = address.Assign (deviceAdjacencyList[n]);
			  }

			  n+=1;
			  m+=1;


		  }
	  }
  }

  //set up global static routing
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();



  //transport layer

  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (nodes.Get (0));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient (serverAddress, 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (nodes.Get (3));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
