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

#ifndef NDN_LINK_RTT_CALCULATOR_HPP
#define NDN_LINK_RTT_CALCULATOR_HPP 

#include "ndn-common.hpp"
#include "ndn-ns3.hpp"
#include "ns3/net-device.h"
#include "ndn-l3-protocol.hpp"
// #include "ndn-net-device-face.hpp"
#include "ns3/point-to-point-net-device.h"
#include "ns3/channel.h"

namespace ns3 {
namespace ndn {

// class NetDeviceFace;
class LinkCalculator;

class LinkRttCalculator
{
public:
  LinkRttCalculator(Ptr<Node> node, Ptr<NetDevice> netDevice);

  void
  onReceiveLinkReply(Ptr<Packet> packet);
  
  void
  onReceiveLinkEcho(Ptr<Packet> &pkt, Address destAddress, uint16_t protocol);
          
  void
  CalculateRtt();

  Time
  getMeanRtt();

  Time
  getVarRtt();

private:
	void
  sendLinkEcho ();

  void
  LinkEchoTimeout();
  
  uint32_t m_nonce;
  EventId m_rttTimerId;
	Time m_rttTimerValue;
	Time m_meanRtt;
	Time m_varRtt;
  bool m_isRttCalculated;
	Ptr<Node> m_node;
  Ptr<NetDevice> m_netDevice;  
};

}
}

#endif //NDN_LINK_RTT_CALCULATOR_HPP