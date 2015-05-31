/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

/*
 * This scenario simulates a grid topology (using topology reader module)
 *
 * (consumer) -- ( ) ----- ( )
 *     |          |         |
 *    ( ) ------ ( ) ----- ( )
 *     |          |         |
 *    ( ) ------ ( ) -- (producer)
 *
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "model/ndn-net-device-face.hpp"

using namespace ns3;
using ns3::ndn::StackHelper;
using ns3::ndn::AppHelper;
using ns3::ndn::GlobalRoutingHelper;
using ns3::ndn::StrategyChoiceHelper;

namespace ns3 {

namespace ndn
{
  class Consumer;
}

void getTotalPackets (string nodeName) 
{
    auto node = Names::Find<Node>(nodeName.c_str());

    auto ndn = node->GetObject<ndn::L3Protocol>();
    int totalPackets = 0;
    for (const auto& entry : ndn->getForwarder()->getFib()) {
      
      for (auto& nextHop : entry.getNextHops()) {
        
        auto face = dynamic_pointer_cast<ndn::NetDeviceFace>(nextHop.getFace());
        if (face == nullptr)
          continue;
        totalPackets += face->getCounters().getNOutInterests() + face->getCounters().getNOutDatas();    
        }
      }
      cout << totalPackets << " " << nodeName << " Interests and Data" << endl;
      // return totalPackets;
};


int
main()
{
  // setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("100Mbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("5ms"));
  Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("100"));

  AnnotatedTopologyReader topologyReader("", 25);
  topologyReader.SetFileName("src/ndnSIM/examples/topologies/topo-grid-3x3-loss.txt");
  topologyReader.Read();

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.InstallAll();

  // Choosing forwarding strategy
  ndn::StrategyChoiceHelper::InstallAll("/prefix", "ndn:/localhost/nfd/strategy/best-route");
  
  // Getting containers for the consumer/producer
  Ptr<Node> producer = Names::Find<Node>("Node8");
  NodeContainer consumerNodes;
  consumerNodes.Add(Names::Find<Node>("Node0"));

  // Consumer
  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  // Consumer will request /prefix/0, /prefix/1, ...
  consumerHelper.SetPrefix("/prefix");
  consumerHelper.SetAttribute("Frequency", StringValue("300")); // 300 interests a second
  consumerHelper.SetAttribute("MaxSeq", IntegerValue(10000));
  consumerHelper.SetAttribute("Randomize",StringValue("uniform"));
  consumerHelper.SetAttribute("RTO",TimeValue(MilliSeconds(24)));
  consumerHelper.SetAttribute("LifeTime",TimeValue(MilliSeconds(24)));
  
  ApplicationContainer consumerApps = consumerHelper.Install(consumerNodes);     // first node
  consumerApps.Get(0)->SetStopTime(MilliSeconds(39999));

  // Producer
  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  // Producer will reply to all requests starting with /prefix
  producerHelper.SetPrefix("/prefix");
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  producerHelper.Install(producer); // last node


  GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();
  ndnGlobalRoutingHelper.AddOrigins("/prefix", producer);
  GlobalRoutingHelper::CalculateRoutes();

  // AsciiTraceHelper ascii;
  // p2p.EnableAsciiAll (ascii.CreateFileStream ("simple3Trace.tr"));
  
  Simulator::Stop(Seconds(42));
  
  Simulator::Schedule (MilliSeconds(39999), &ns3::getTotalPackets, "Node0");
  Simulator::Schedule (MilliSeconds(39999), &ns3::getTotalPackets, "Node1");
  Simulator::Schedule (MilliSeconds(39999), &ns3::getTotalPackets, "Node2");
  Simulator::Schedule (MilliSeconds(39999), &ns3::getTotalPackets, "Node3");
  Simulator::Schedule (MilliSeconds(39999), &ns3::getTotalPackets, "Node4");
  Simulator::Schedule (MilliSeconds(39999), &ns3::getTotalPackets, "Node5");
  Simulator::Schedule (MilliSeconds(39999), &ns3::getTotalPackets, "Node6");
  Simulator::Schedule (MilliSeconds(39999), &ns3::getTotalPackets, "Node7");
  Simulator::Schedule (MilliSeconds(39999), &ns3::getTotalPackets, "Node8");


  Simulator::Run();
  Simulator::Destroy();
  return 0;
}
} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main();
}
