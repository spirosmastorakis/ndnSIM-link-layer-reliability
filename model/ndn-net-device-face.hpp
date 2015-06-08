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

#ifndef NDN_NET_DEVICE_FACE_H
#define NDN_NET_DEVICE_FACE_H

#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/ndnSIM/model/ndn-face.hpp"
#include "ns3/net-device.h"
#include "fstream"
#include "ns3/ndnSIM/model/packet-cache.hpp"
#include "ns3/ndnSIM/model/ndn-link-rtt-calculator.hpp"

namespace ns3 {
namespace ndn {

class LinkRttCalculator;
class NetDeviceFace;

/**
 * \ingroup ndn-face
 * \brief Implementation of layer-2 (Ethernet) Ndn face
 *
 * This class defines basic functionality of Ndn face. Face is core
 * component responsible for actual delivery of data packet to and
 * from Ndn stack
 *
 * NdnNetDevice face is permanently associated with one NetDevice
 * object and this object cannot be changed for the lifetime of the
 * face
 *
 * \see NdnAppFace, NdnNetDeviceFace, NdnIpv4Face, NdnUdpFace
 */
class NetDeviceFace : public Face {
public:
  /**
   * \brief Constructor
   *
   * @param node Node associated with the face
   * @param netDevice a smart pointer to NetDevice object to which
   * this face will be associate
   */
  NetDeviceFace(Ptr<Node> node, const Ptr<NetDevice>& netDevice);

  virtual ~NetDeviceFace();

public: // from nfd::Face
  virtual void
  sendInterest(const Interest& interest);

  virtual void
  sendData(const Data& data);
  
  void
  printRetransmissions();
  
  virtual void
  close();

public:
  /**
   * \brief Get NetDevice associated with the face
   *
   * \returns smart pointer to NetDevice associated with the face
   */
  Ptr<NetDevice>
  GetNetDevice() const;
  

private:
  uint32_t
  send(Ptr<Packet> packet);
  
  void
  processHoles();
  
  void
  onReceiveRepeatRequest(Ptr<Packet> &packet);
  
  void
  onRepeatRequestTimeout(uint32_t seqnum);
  
  void 
  processAckList(std::list<uint32_t> ackList);
  
  void
  processRepeatList(std::list<uint32_t> repeatList);
  
  void 
  Retransmit(uint32_t seqnum);
  
  void
  sendNack(uint32_t seqnum);
  
  bool
  IsBetween(uint32_t start, uint32_t givenNum, uint32_t end);
  
  void 
  incSeqCount(uint32_t &seqnum);
  
  bool
  canRetransmit(uint32_t seqnum);

  bool
  canSendNack(uint32_t seqnum);

  void
  processIncomingPacket(Ptr<Packet> &packet, Address destAddress ,uint16_t protocol);
  
  void
  updateReceiverState();

  void
  updateSenderState();
  
  void
  cancelNackTimer(uint32_t seqnum);

  void
  ackIncomingSeqnum(uint32_t seqnum);

  void
  dropIncomingPacket(uint32_t seqnum);

  void
  AckTimerTimeout();
  
  void
  incBufferSize();
  
  /// \brief callback from lower layers
  void
  receiveFromNetDevice(Ptr<NetDevice> device, Ptr<const Packet> p, uint16_t protocol,
                       const Address& from, const Address& to, NetDevice::PacketType packetType);

  enum IOrD
  {
      INTEREST,
      DATA
  };
  
private:
  Ptr<Node> m_node;
  Ptr<NetDevice> m_netDevice; ///< \brief Smart pointer to NetDevice
  uint32_t m_nextPacketToSend;
  uint32_t m_windowSize;      //Sequence number range is 0 to m_windowSize - 1
  
  uint32_t m_packetExpected;
  uint32_t m_lastPacketReceived;
  
  std::vector<bool> m_isPacketReceived;
  
  std::vector<EventId> m_timerEvents;
  
  PacketCache m_packetCache;
  
  LinkRttCalculator m_rttCalculator;

  const uint32_t m_maxRetransmissions;
  const uint32_t m_maxNacks;

  std::list<uint32_t> m_toBeAcked;
   
  EventId m_ackTimerId;
  EventId m_dropTimerId;

  const bool m_ackEnabled;
  
  std::map<uint32_t, uint32_t> m_retransmissions; //(sequence no.  -  no. of retransmissions) pairs
  
  std::map<uint32_t, uint32_t> m_nacksSent;  //(sequence no.  -  no. of nacks sent) pairs
  std::map<uint32_t, EventId> m_nackTimers;  //(sequence no.  -  nack timer id ) pairs

  ofstream m_retransmissionsFile;
  ofstream m_nacksFile;
  ofstream m_receivedNacksFile;
  ofstream m_receiverDropsFile;

  int m_retransmissionsFd;
  int m_nacksFd;
  int m_receivedNacksFd;
  int m_receiverDropsFd;
  int m_acksFd;  
  int m_iordFd;
};

} // namespace ndn
} // namespace ns3

#endif // NDN_NET_DEVICE_FACE_H
