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
 * This scenario simulates ndn-testbed-topology 
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


int
main()
{
  // setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("100Mbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("5ms"));
  Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("100"));

  AnnotatedTopologyReader topologyReader("", 25);
  topologyReader.SetFileName("src/ndnSIM/examples/topologies/top-ndn-testbed.txt");
  topologyReader.Read();

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.InstallAll();

  // Choosing forwarding strategy
  ndn::StrategyChoiceHelper::InstallAll("/prefix1", "ndn:/localhost/nfd/strategy/best-route");
  ndn::StrategyChoiceHelper::InstallAll("/prefix2", "ndn:/localhost/nfd/strategy/best-route");
  ndn::StrategyChoiceHelper::InstallAll("/prefix3", "ndn:/localhost/nfd/strategy/best-route");
  ndn::StrategyChoiceHelper::InstallAll("/prefix4", "ndn:/localhost/nfd/strategy/best-route");
  ndn::StrategyChoiceHelper::InstallAll("/prefix5", "ndn:/localhost/nfd/strategy/best-route");
  ndn::StrategyChoiceHelper::InstallAll("/prefix6", "ndn:/localhost/nfd/strategy/best-route");
  ndn::StrategyChoiceHelper::InstallAll("/prefix7", "ndn:/localhost/nfd/strategy/best-route");
  
  
  // Getting containers for the consumer/producer
  Ptr<Node> producer1 = Names::Find<Node>("MICH"); //2
  Ptr<Node> producer2 = Names::Find<Node>("PADUA"); //4
  Ptr<Node> producer3 = Names::Find<Node>("REMAP"); //4
  Ptr<Node> producer4 = Names::Find<Node>("VASEDA"); //2
  Ptr<Node> producer5 = Names::Find<Node>("NTNU"); //2
  Ptr<Node> producer6 = Names::Find<Node>("URJC"); //4
  Ptr<Node> producer7 = Names::Find<Node>("CSU");

  Ptr<Node> consumer1 = Names::Find<Node>("ORANGE");
  Ptr<Node> consumer2 = Names::Find<Node>("CAIDA");
  Ptr<Node> consumer3 = Names::Find<Node>("LIP6");
  Ptr<Node> consumer4 = Names::Find<Node>("BYU");
  Ptr<Node> consumer5 = Names::Find<Node>("BUPT");
  Ptr<Node> consumer6 = Names::Find<Node>("UCLA");
  Ptr<Node> consumer7 = Names::Find<Node>("NTNU");


  // auto printFib = [](Ptr<Node> node) {
  //   auto ndn = node->GetObject<ndn::L3Protocol>();
  //   for (const auto& entry : ndn->getForwarder()->getFib()) {
  //     cout << entry.getPrefix() << " (";

  //     bool isFirst = true;
  //     for (auto& nextHop : entry.getNextHops()) {
  //       if (!isFirst)
  //         cout << ", ";
        
  //       cout << *nextHop.getFace();
  //       auto face = dynamic_pointer_cast<ndn::NetDeviceFace>(nextHop.getFace());
  //       if (face == nullptr)
  //         continue;

  //       cout << " towards ";

  //       if ( face->GetNetDevice()->GetChannel()->GetDevice(1)->GetNode() == node )
  //         cout << Names::FindName(face->GetNetDevice()->GetChannel()->GetDevice(0)->GetNode());
  //       else
  //         cout << Names::FindName(face->GetNetDevice()->GetChannel()->GetDevice(1)->GetNode());
  //       isFirst = false;
  //     }
  //     cout << ")" << endl;
  //   }
  // };

  // Consumer
  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  // Consumer will request /prefix/0, /prefix/1, ...
  consumerHelper.SetAttribute("Frequency", StringValue("500")); // 500 interests a second
  consumerHelper.SetAttribute("MaxSeq", IntegerValue(10000));
  consumerHelper.SetAttribute("Randomize",StringValue("uniform"));
  consumerHelper.SetAttribute("RTO",TimeValue(MilliSeconds(24)));
  consumerHelper.SetAttribute("LifeTime",TimeValue(MilliSeconds(24)));
  
  consumerHelper.SetPrefix("/prefix1");
  ApplicationContainer consumerApps1 = consumerHelper.Install(consumer1);     
  consumerApps1.Get(0)->SetStopTime(MilliSeconds(49999));

  consumerHelper.SetPrefix("/prefix2");
  ApplicationContainer consumerApps2 = consumerHelper.Install(consumer2);     
  consumerApps2.Get(0)->SetStopTime(MilliSeconds(49999));

  consumerHelper.SetPrefix("/prefix3");
  ApplicationContainer consumerApps3 = consumerHelper.Install(consumer3);     
  consumerApps3.Get(0)->SetStopTime(MilliSeconds(49999));

  consumerHelper.SetPrefix("/prefix4");
  ApplicationContainer consumerApps4 = consumerHelper.Install(consumer4);    
  consumerApps4.Get(0)->SetStopTime(MilliSeconds(49999));

  consumerHelper.SetPrefix("/prefix5");
  ApplicationContainer consumerApps5 = consumerHelper.Install(consumer5);     
  consumerApps5.Get(0)->SetStopTime(MilliSeconds(49999));

  consumerHelper.SetPrefix("/prefix6");
  ApplicationContainer consumerApps6 = consumerHelper.Install(consumer6);    
  consumerApps6.Get(0)->SetStopTime(MilliSeconds(49999));

  consumerHelper.SetPrefix("/prefix7");
  ApplicationContainer consumerApps7 = consumerHelper.Install(consumer7);    
  consumerApps7.Get(0)->SetStopTime(MilliSeconds(49999));

  // Producer
  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  // Producer will reply to all requests starting with /prefix1
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  
  producerHelper.SetPrefix("/prefix1");
  producerHelper.Install(producer1); 

  producerHelper.SetPrefix("/prefix2");
  producerHelper.Install(producer2);

  producerHelper.SetPrefix("/prefix3");
  producerHelper.Install(producer3); 

  producerHelper.SetPrefix("/prefix4");
  producerHelper.Install(producer4);

  producerHelper.SetPrefix("/prefix5");
  producerHelper.Install(producer5); 

  producerHelper.SetPrefix("/prefix6");
  producerHelper.Install(producer6);

  producerHelper.SetPrefix("/prefix7");
  producerHelper.Install(producer7);

  GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();
  ndnGlobalRoutingHelper.AddOrigins("/prefix1", producer1);
  ndnGlobalRoutingHelper.AddOrigins("/prefix2", producer2);
  ndnGlobalRoutingHelper.AddOrigins("/prefix3", producer3);
  ndnGlobalRoutingHelper.AddOrigins("/prefix4", producer4);
  ndnGlobalRoutingHelper.AddOrigins("/prefix5", producer5);
  ndnGlobalRoutingHelper.AddOrigins("/prefix6", producer6);
  ndnGlobalRoutingHelper.AddOrigins("/prefix7", producer7);
  
  GlobalRoutingHelper::CalculateRoutes();



// Ptr<Node> producer1 = Names::Find<Node>("MICH"); //2
//   Ptr<Node> producer2 = Names::Find<Node>("PADUA"); //3
//   Ptr<Node> producer3 = Names::Find<Node>("REMAP"); //3
//   Ptr<Node> producer4 = Names::Find<Node>("VASEDA"); //2
//   Ptr<Node> producer5 = Names::Find<Node>("NTNU"); //2
//   Ptr<Node> producer6 = Names::Find<Node>("URJC"); //3
  
//   Ptr<Node> consumer1 = Names::Find<Node>("ORANGE");
//   Ptr<Node> consumer2 = Names::Find<Node>("CAIDA");
//   Ptr<Node> consumer3 = Names::Find<Node>("LIP6");
//   Ptr<Node> consumer4 = Names::Find<Node>("BYU");
//   Ptr<Node> consumer5 = Names::Find<Node>("BUPT");
//   Ptr<Node> consumer6 = Names::Find<Node>("UCLA");


  // cout << "FIB content on node ORANGE" << endl;
  // printFib(consumer1);

  // cout << "FIB content on node CAIDA" << endl;
  // printFib(consumer2);
  
  // cout << "FIB content on node LIP6" << endl;
  // printFib(consumer3);
  
  // cout << "FIB content on node BYU" << endl;
  // printFib(consumer4);
  
  // cout << "FIB content on node BUPT" << endl;
  // printFib(consumer5);
  
  // cout << "FIB content on node UCLA" << endl;
  // printFib(consumer6);
  
  // cout << "FIB content on node TONGJI" << endl;
  // printFib(Names::Find<Node>("TONGJI"));
  
  // cout << "FIB content on node PKU" << endl;
  // printFib(Names::Find<Node>("PKU"));
  
  // cout << "FIB content on node PADUA" << endl;
  // printFib(Names::Find<Node>("PADUA"));
  
  // cout << "FIB content on node URJC" << endl;
  // printFib(Names::Find<Node>("URJC"));
  
  // cout << "FIB content on node REMAP" << endl;
  // printFib(Names::Find<Node>("REMAP"));
  
  // cout << "FIB content on node UA" << endl;
  // printFib(Names::Find<Node>("UA"));
  
  // cout << "FIB content on node BASEL" << endl;
  // printFib(Names::Find<Node>("BASEL"));
  
  // cout << "FIB content on node URJC" << endl;
  // printFib(Names::Find<Node>("URJC"));
  

  // AsciiTraceHelper ascii;
  // p2p.EnableAsciiAll (ascii.CreateFileStream ("simple3Trace.tr"));
  
  Simulator::Stop(Seconds(50));
  
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
