/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "adaptive-strategy.hpp"
#include "ns3/simulator.h"

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "model/ndn-net-device-face.hpp"
using namespace ns3;

namespace nfd {
namespace fw {

const Name AdaptiveStrategy::STRATEGY_NAME("ndn:/localhost/nfd/strategy/adaptive-strategy/%FD%01");

AdaptiveStrategy::AdaptiveStrategy(Forwarder& forwarder, const Name& name)
  : Strategy(forwarder, name)
{
}

AdaptiveStrategy::~AdaptiveStrategy()
{
}

static inline bool
predicate_PitEntry_canForwardTo_NextHop(shared_ptr<pit::Entry> pitEntry,
                                        const fib::NextHop& nexthop)
{
  return pitEntry->canForwardTo(*nexthop.getFace());
}

void
AdaptiveStrategy::afterReceiveInterest(const Face& inFace,
                                 const Interest& interest,
                                 shared_ptr<fib::Entry> fibEntry,
                                 shared_ptr<pit::Entry> pitEntry)
{
   if (pitEntry->hasUnexpiredOutRecords()) {
    // not a new Interest, don't forward
    return;
   }

   // const_cast<Face&>(inFace).addBytes(1024);
   // const_cast<Face&>(inFace).addUtilization(1024);
   
   const fib::NextHopList& nexthops = fibEntry->getNextHops();
   fib::NextHopList::const_iterator it = nexthops.begin();
   auto i = it;
   for (; i != nexthops.end(); i++) {
   
     shared_ptr<Face> foundFace = ((*i).getFace())->shared_from_this();
     // if (not predicate_PitEntry_canForwardTo_NextHop(pitEntry, *i))
     //    continue;
     std::cout << *foundFace << " size = " << foundFace->getBytes() << " ";
     
     auto face = dynamic_pointer_cast<ns3::ndn::NetDeviceFace>(foundFace);
     if (face != nullptr)
     {
       std::cout << face->getTotalBytes() << "  " << Names::FindName(face->GetNetDevice()->GetChannel()->GetDevice(0)->GetNode()) << "-" << Names::FindName(face->GetNetDevice()->GetChannel()->GetDevice(1)->GetNode()) << std::endl;
       if (foundFace->getBytes() <= foundFace->getTotalBytes()) {
         // foundFace->addBytes(1024);
         // foundFace->addUtilization(1024);
         this->sendInterest(pitEntry, foundFace);
         break;
       }
     }
     else  /*if not a netdevice face, don't care about congestion*/
     {
         // foundFace->addBytes(1024);
         // foundFace->addUtilization(1024);
         this->sendInterest(pitEntry, foundFace);
         break;
     }
     std::cout << "Not Forwarding through first" << std::endl;
   }
   if (i == nexthops.end())
   {
    std::cout << "Dropped packet" << std::endl;
   }
}


void
AdaptiveStrategy::beforeSatisfyInterest(shared_ptr<pit::Entry> pitEntry,
                                  const Face& inFace, const Data& data)
{
  // const_cast<Face&>(inFace).substractBytes(1024);
  // const_cast<Face&>(inFace).substractUtilization(1024); 
}

void
AdaptiveStrategy::beforeExpirePendingInterest(shared_ptr<pit::Entry> pitEntry,
                                  const Face& inFace, const Data& data)
{
  // const_cast<Face&>(inFace).substractBytes(1024);
  // const_cast<Face&>(inFace).substractUtilization(1024);
}

AdaptiveStrategy::MtInfo::MtInfo()
{
  usedFaces.clear();
}

} // namespace fw
} // namespace nfd
