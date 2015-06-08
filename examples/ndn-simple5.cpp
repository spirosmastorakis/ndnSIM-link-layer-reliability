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

void
packetDropCallback(const Ptr<const Packet> packet)
{
  std::cout << "Packet Drop" << std::endl;
}

int
main()
{
  // setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("100Mbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("5ms"));
  Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("100"));

  AnnotatedTopologyReader topologyReader("", 25);
  topologyReader.SetFileName("src/ndnSIM/examples/topologies/topo-6-node-loss.txt");
  topologyReader.Read();

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.InstallAll();

  // Choosing forwarding strategy
  ndn::StrategyChoiceHelper::InstallAll("/prefix1", "ndn:/localhost/nfd/strategy/best-route");
  ndn::StrategyChoiceHelper::InstallAll("/prefix2", "ndn:/localhost/nfd/strategy/best-route");
  
  
  // Getting containers for the consumer/producer
  Ptr<Node> producer1 = Names::Find<Node>("Dst1");
  Ptr<Node> producer2 = Names::Find<Node>("Dst2");
  
  Ptr<Node> consumer1 = Names::Find<Node>("Src1");
  Ptr<Node> consumer2 = Names::Find<Node>("Src2");


  // Consumer
  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  // Consumer will request /prefix/0, /prefix/1, ...
  consumerHelper.SetAttribute("Frequency", StringValue("500")); // 500 interests a second
  consumerHelper.SetAttribute("MaxSeq", IntegerValue(10000));
  consumerHelper.SetAttribute("Randomize",StringValue("uniform"));
  consumerHelper.SetAttribute("RTO",TimeValue(MilliSeconds(24)));
  consumerHelper.SetAttribute("LifeTime",TimeValue(MilliSeconds(24)));
  
  consumerHelper.SetPrefix("/prefix1");
  ApplicationContainer consumerApps1 = consumerHelper.Install(consumer1);     // first node
  consumerApps1.Get(0)->SetStopTime(MilliSeconds(39999));

  consumerHelper.SetPrefix("/prefix2");
  ApplicationContainer consumerApps2 = consumerHelper.Install(consumer2);     // first node
  consumerApps2.Get(0)->SetStopTime(MilliSeconds(39999));

  // Producer
  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  // Producer will reply to all requests starting with /prefix1
  producerHelper.SetAttribute("PayloadSize", StringValue("800"));
  
  producerHelper.SetPrefix("/prefix1");
  producerHelper.Install(producer1); 

  producerHelper.SetPrefix("/prefix2");
  producerHelper.Install(producer2);


  GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();
  ndnGlobalRoutingHelper.AddOrigins("/prefix1", producer1);
  ndnGlobalRoutingHelper.AddOrigins("/prefix2", producer2);
  
  GlobalRoutingHelper::CalculateRoutes();

  Config::ConnectWithoutContext("/NodeList/2/DeviceList/2/$ns3::PointToPointNetDevice/TxQueue/Drop", MakeCallback(&packetDropCallback));
  Config::ConnectWithoutContext("/NodeList/2/DeviceList/1/$ns3::PointToPointNetDevice/TxQueue/Drop", MakeCallback(&packetDropCallback));
  Config::ConnectWithoutContext("/NodeList/2/DeviceList/0/$ns3::PointToPointNetDevice/TxQueue/Drop", MakeCallback(&packetDropCallback));

  // AsciiTraceHelper ascii;
  // p2p.EnableAsciiAll (ascii.CreateFileStream ("simple3Trace.tr"));
  
  Simulator::Stop(Seconds(42));
  
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
