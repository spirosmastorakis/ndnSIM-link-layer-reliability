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

#include "ndn-net-device-face.hpp"
#include "ndn-l3-protocol.hpp"
#include "ndn-ns3.hpp"
#include "ns3/net-device.h"

// #include "ns3/address.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/channel.h"

#include "../utils/ndn-fw-hop-count-tag.hpp"
#include "../utils/ndn-sequence-count-tag.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "model/ndn-net-device-face.hpp"

using namespace std;

NS_LOG_COMPONENT_DEFINE("ndn.NetDeviceFace");


namespace ns3 {
namespace ndn {

NetDeviceFace::NetDeviceFace(Ptr<Node> node, const Ptr<NetDevice>& netDevice)
  : Face(FaceUri("netDeviceFace://"), FaceUri("netDeviceFace://"))
  , m_node(node)
  , m_netDevice(netDevice)
  , m_rttCalculator(node, netDevice)
  , m_maxRetransmissions(2)
  , m_maxNacks(2)
  , m_ackEnabled(false)
{
  NS_LOG_FUNCTION(this << netDevice);

  setMetric(1); // default metric

  NS_ASSERT_MSG(m_netDevice != 0, "NetDeviceFace needs to be assigned a valid NetDevice");

  m_node->RegisterProtocolHandler(MakeCallback(&NetDeviceFace::receiveFromNetDevice, this),
                                  L3Protocol::ETHERNET_FRAME_TYPE, m_netDevice,
                                  true /*promiscuous mode*/);
  
  m_windowSize = 1024;
  
  //Variables representing state at sender
  m_nextPacketToSend = 0;
  
  //Variables representing state at receiver
  m_packetExpected = 0;
  m_lastPacketReceived = 0; 
  m_isPacketReceived.assign(m_windowSize, false);
  
  //members related to timer  
  if (m_ackEnabled)
  {
      m_timerEvents.assign(m_windowSize, EventId());
  }

  m_ackTimerId = EventId();
  m_dropTimerId = EventId();

  std::ostringstream s, s2, s3, s4, s5, s6;
  s << "NetDevice-" << m_node->GetId() << "-Retransmissions.txt";
  // m_retransmissionsFile.open(s.str());
  m_retransmissionsFd = open(s.str().c_str(), O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);

  s2 << "NetDevice-" << m_node->GetId() << "-Nacks.txt";
  // m_nacksFile.open(s2.str());
  m_nacksFd = open(s2.str().c_str(), O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);

  s3 << "NetDevice-" << m_node->GetId() << "-ReceivedNacks.txt";
  // m_receivedNacksFile.open(s3.str());
  m_receivedNacksFd = open(s3.str().c_str(), O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);

  s4 << "NetDevice-" << m_node->GetId() << "-ReceiverDrops.txt";
  // m_receiverDropsFile.open(s4.str());
  m_receiverDropsFd = open(s4.str().c_str(), O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);

  s5 << "NetDevice-" << m_node->GetId() << "-PureAcks.txt";
  m_acksFd = open(s5.str().c_str(), O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);

  s6 << "NetDevice-" << m_node->GetId() << "-IorD.txt";     //All interests or data that is sent
  m_iordFd = open(s6.str().c_str(), O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);

  this->setTotalBytes(2*1024);
  incBufferSize();
}

NetDeviceFace::~NetDeviceFace()
{
  NS_LOG_FUNCTION_NOARGS();
  printRetransmissions();
  close();
}

void
NetDeviceFace::close()
{
  NS_LOG_FUNCTION(this);
  
  m_node->UnregisterProtocolHandler(MakeCallback(&NetDeviceFace::receiveFromNetDevice, this));
  this->fail("Close connection");
}

Ptr<NetDevice>
NetDeviceFace::GetNetDevice() const
{
  NS_LOG_FUNCTION(this);
    
  return m_netDevice;
}


void 
NetDeviceFace::incBufferSize()
{
  NS_LOG_FUNCTION(this);
  setTotalBytes( getTotalBytes() + 1024 );
  
  if (m_netDevice->GetChannel()->GetDevice(0)->GetNode() == Names::Find<Node>("Rtr1") 
      && m_netDevice->GetChannel()->GetDevice(1)->GetNode() == Names::Find<Node>("Rtr2"))
  {
    Simulator::Schedule(MicroSeconds(1600), &NetDeviceFace::incBufferSize, this);
    // std::cout << Simulator::Now() << Names::FindName(m_netDevice->GetChannel()->GetDevice(0)->GetNode()) << "-" << Names::FindName(m_netDevice->GetChannel()->GetDevice(1)->GetNode()) << " " << getTotalBytes() << "Next time : " << Time("5ms") << endl;
  }
  else if (m_netDevice->GetChannel()->GetDevice(0)->GetNode() == Names::Find<Node>("Rtr1")
      && m_netDevice->GetChannel()->GetDevice(1)->GetNode() == Names::Find<Node>("Rtr2"))
  {
    Simulator::Schedule(MicroSeconds(1600), &NetDeviceFace::incBufferSize, this);
    // std::cout << Simulator::Now() << Names::FindName(m_netDevice->GetChannel()->GetDevice(0)->GetNode()) << "-" << Names::FindName(m_netDevice->GetChannel()->GetDevice(1)->GetNode()) << " " << getTotalBytes() << "Next time : " << Time("5ms") << endl;
  }
  else
  {
    Simulator::Schedule(NanoSeconds(100000), &NetDeviceFace::incBufferSize, this);
    // std::cout << Simulator::Now() << Names::FindName(m_netDevice->GetChannel()->GetDevice(0)->GetNode()) << "-" << Names::FindName(m_netDevice->GetChannel()->GetDevice(1)->GetNode()) << " " << getTotalBytes() << "Next time : " << Time("0.5ms") << endl;
  }
}

void
NetDeviceFace::printRetransmissions()
{
    NS_LOG_FUNCTION(this);
    
    std::map<uint32_t,uint32_t>::iterator it;
    NS_LOG_INFO("Node " << m_node->GetId() << " Retranmissions. Device : " << m_netDevice->GetAddress());
    for (it = m_retransmissions.begin(); it != m_retransmissions.end(); it++)
      NS_LOG_INFO(it->first << "  -  " << it->second);
}

uint32_t            //Returns the sequence number of the packet sent
NetDeviceFace::send(Ptr<Packet> packet)
{
  NS_LOG_FUNCTION(this);
    
  NS_ASSERT_MSG(packet->GetSize() <= m_netDevice->GetMtu(),
                "Packet size " << packet->GetSize() << " exceeds device MTU "
                               << m_netDevice->GetMtu());
  
  FwHopCountTag tag;
  packet->RemovePacketTag(tag);
  tag.Increment();
  packet->AddPacketTag(tag);
  
  SequenceCountTag seqTag;
  seqTag.Assign(m_nextPacketToSend);
  packet->AddPacketTag(seqTag);
  
  //Schedule a timer for retransmission
  Time tNext = 3 * m_rttCalculator.getMeanRtt();
  
  NS_LOG_INFO("Node " << m_node->GetId() << ":" << "Sending a packet with sequence number " << seqTag.Get() << " with timer value = " << tNext);
  
  if (m_ackEnabled)
  {
      m_timerEvents[m_nextPacketToSend % m_windowSize] = Simulator::Schedule (tNext, &NetDeviceFace::Retransmit, this, m_nextPacketToSend);
  }
  
  uint32_t seqNum = m_nextPacketToSend;
  incSeqCount(m_nextPacketToSend);
  
  ostringstream s;
  s << seqNum << endl;
  lseek(m_iordFd,0,SEEK_END);
  if (write(m_iordFd,s.str().c_str(),s.str().size()) == -1)
      std::cout << "ERROR" << std::endl;

  m_netDevice->Send(packet, m_netDevice->GetBroadcast(), L3Protocol::ETHERNET_FRAME_TYPE);
  return seqNum;
}

void
NetDeviceFace::sendInterest(const Interest& interest)
{
  NS_LOG_FUNCTION(this << &interest);

  this->onSendInterest(interest);
  NS_LOG_DEBUG("Node " << m_node->GetId() << ":" << "Sending interest(piggybackedAck) with name : " << interest.getName());
  
  shared_ptr<LinkPbAck> ack(new LinkPbAck);
  ack->setInterest(interest);
  NS_LOG_INFO("Node " << m_node->GetId() << ":" << "In Interest, acknowledging : ");
  for(std::list<uint32_t>::iterator it = m_toBeAcked.begin(); it != m_toBeAcked.end(); it++)
  {
        ack->addAckNumber(*it);
        NS_LOG_INFO( *it << " " );
  }
  
  Ptr<Packet> packet = Convert::ToPacket<LinkPbAck>(*ack);
  uint32_t seqNum = send(packet); 
  
  m_toBeAcked.clear();
  Simulator::Cancel(m_ackTimerId);
  m_packetCache.insert(seqNum, interest);
}

void
NetDeviceFace::sendData(const Data& data)
{
  NS_LOG_FUNCTION(this << &data);

  this->onSendData(data);
  NS_LOG_DEBUG("Node " << m_node->GetId() << ":" << "Sending Data(PiggybackedAck) with name : " << data.getName());
  
  shared_ptr<LinkPbAck> ack(new LinkPbAck);
  ack->setData(data);
  NS_LOG_INFO("Node " << m_node->GetId() << ":" << "In data, acknowledging : ");
  for (std::list<uint32_t>::iterator it = m_toBeAcked.begin(); it != m_toBeAcked.end(); it++)
  {
    ack->addAckNumber(*it);
    NS_LOG_INFO( *it );
  }
    
  Ptr<Packet> packet = Convert::ToPacket<LinkPbAck>(*ack);
  uint32_t seqNum = send(packet);
  
  m_toBeAcked.clear();
  Simulator::Cancel(m_ackTimerId);
  m_packetCache.insert(seqNum, data);
}

void
NetDeviceFace::processHoles()
{
    NS_LOG_FUNCTION(this);
    
    //If next 2 packets are received and repeat request is not sent already,
    // NS_LOG_INFO(m_packetExpected << " " << m_lastPacketReceived);
    for (uint32_t it = m_packetExpected; it < m_lastPacketReceived; it++)
    {
        if ( (not m_isPacketReceived[it % m_windowSize]) && m_isPacketReceived[(it+1) % m_windowSize] && m_isPacketReceived[(it+2) % m_windowSize] )
        {
            //If there is a hole
            sendNack(it); //Send's nack if it can be sent.
        }
    }
    // if (m_isRttCalculated)
    //     Simulator::Schedule( m_rttCalculator.getMeanRtt() / 4, &NetDeviceFace::processHoles, this );    
}

void
NetDeviceFace::sendNack(uint32_t seqnum)
{
    NS_LOG_FUNCTION(this << seqnum);
    if ( canSendNack(seqnum) )
    {
        //If nack can be sent, send repeat request(nack)
        shared_ptr<RepeatRequest> request(new RepeatRequest);
        request->setRepeatNumber(seqnum);
        Ptr<Packet> packet = Convert::ToPacket(*request);
        m_netDevice->Send(packet, m_netDevice->GetBroadcast(), L3Protocol::ETHERNET_FRAME_TYPE);
              
        Time tNext;
        tNext = 2 * m_rttCalculator.getMeanRtt();
              
        m_nackTimers[seqnum] = Simulator::Schedule(tNext, &NetDeviceFace::onRepeatRequestTimeout, this, seqnum);
        ostringstream s;
        s << seqnum << endl;
        // m_nacksFile << "\n " << seqnum;
        // m_nacksFile.flush();
        lseek(m_nacksFd,0,SEEK_END);
        if (write(m_nacksFd,s.str().c_str(),s.str().size()) == -1)
            std::cout << "ERROR" << std::endl;
    }
}

void
NetDeviceFace::onReceiveRepeatRequest(Ptr<Packet> &packet)
{
    NS_LOG_FUNCTION(this);
    
    shared_ptr<const RepeatRequest> request = Convert::FromPacket<RepeatRequest>(packet);
    NS_LOG_INFO("Node " << m_node->GetId() << ":" << "Received RepeatRequest and Repeating " << request->getRepeatNumber());
    
    if (m_ackEnabled)
        Simulator::Cancel(m_timerEvents[request->getRepeatNumber() % m_windowSize]);

    // m_receivedNacksFile << "\n " << request->getRepeatNumber();
    // m_receivedNacksFile.flush();
    ostringstream s;
    s << request->getRepeatNumber() << endl;
    lseek(m_receivedNacksFd,0,SEEK_END);
    if (write(m_receivedNacksFd,s.str().c_str(),s.str().size()) == -1)
        std::cout << "ERROR" << std::endl;

    Retransmit(request->getRepeatNumber());
}

void
NetDeviceFace::onRepeatRequestTimeout(uint32_t seqnum)
{
    NS_LOG_FUNCTION(this);
    
    NS_ASSERT(m_nackTimers.find(seqnum) != m_nackTimers.end());  //Repeat request has to be sent for this sequence number
    NS_ASSERT(m_nackTimers[seqnum].IsExpired());
    if (m_packetExpected <= seqnum && (not m_isPacketReceived[seqnum % m_windowSize])) //Otherwise the packet is already received
    { 
        m_nackTimers.erase(seqnum);
        sendNack(seqnum);
    }
}

void 
NetDeviceFace::processAckList(std::list<uint32_t> ackList)
{
  NS_LOG_FUNCTION(this);
    
    for (std::list<uint32_t>::iterator it = ackList.begin(); it != ackList.end(); it++)
    {
        m_packetCache.erase(*it);
        if (m_ackEnabled)
            Simulator::Cancel (m_timerEvents[*it % m_windowSize]);
    }
}

void
NetDeviceFace::processRepeatList(std::list<uint32_t> repeatList)
{
  NS_LOG_FUNCTION(this);
    
    for (std::list<uint32_t>::iterator it = repeatList.begin(); it != repeatList.end(); it++)
    {
        NS_LOG_INFO("Node " << m_node->GetId() << ":" << "Process repeat request = " << *it);
        if (m_ackEnabled)
            Simulator::Cancel(m_timerEvents[*it % m_windowSize]);
        // m_receivedNacksFile << "\n " << *it;
        // m_receivedNacksFile.flush();
        
        ostringstream s;
        s << *it << endl;
        lseek(m_receivedNacksFd,0,SEEK_END);
        if (write(m_receivedNacksFd,s.str().c_str(),s.str().size()) == -1)
            std::cout << "ERROR" << std::endl;

        Retransmit(*it);
    }
}

void
NetDeviceFace::incSeqCount(uint32_t &seqnum)
{
    NS_LOG_FUNCTION(this << seqnum);
    seqnum++;
}

void
NetDeviceFace::Retransmit(uint32_t expiredSeqnum)
{
    NS_LOG_FUNCTION(this);
    if (m_ackEnabled)
        NS_ASSERT(!m_timerEvents[expiredSeqnum % m_windowSize].IsRunning());
    //Calling a call back on retransmission
    if ( canRetransmit(expiredSeqnum) )  //If current number of retransmission is less than max no. of retransmissions, then retransmit again
    {     
      
      Ptr<Packet> packet;
      shared_ptr<LinkPbAck> linkPbAck(new LinkPbAck);
      PacketCacheEntry *e = m_packetCache.find(expiredSeqnum);
      if (e == NULL) 
      {
        m_retransmissions[expiredSeqnum]--;
        return;
      } 
      if (e->containsInterest())
      {
          linkPbAck->setInterest(e->getInterest());
          NS_LOG_DEBUG("Node " << m_node->GetId() << ":" << "Retransmitting interest with prefix " << e->getInterest().getName());
          this->onSendInterest(e->getInterest());
      }
      else
      {
          NS_LOG_DEBUG("Node " << m_node->GetId() << ":" << "Retransmitting data with prefix " << e->getData().getName());
          linkPbAck->setData(e->getData());
          this->onSendData(e->getData());
      }
      
      NS_LOG_INFO("Node " << m_node->GetId() << ":" << "Acknowledging in PbAck ");
      for(std::list<uint32_t>::iterator it = m_toBeAcked.begin(); it != m_toBeAcked.end(); it++)
      {
        linkPbAck->addAckNumber(*it);
        NS_LOG_INFO( *it );
      }
      packet = Convert::ToPacket(*linkPbAck);     
      
      FwHopCountTag tag;
      packet->RemovePacketTag(tag);
      tag.Increment();
      packet->AddPacketTag(tag);
      
      SequenceCountTag seqTag;
      seqTag.Assign(expiredSeqnum);
      packet->AddPacketTag(seqTag);
      
      m_netDevice->Send(packet, m_netDevice->GetBroadcast(), L3Protocol::ETHERNET_FRAME_TYPE);
        
      // m_retransmissionsFile << "\n " << expiredSeqnum;  
      // m_retransmissionsFile.flush();
      
      ostringstream s, s2;
      s << expiredSeqnum << endl;
      lseek(m_retransmissionsFd,0,SEEK_END);
      if (write(m_retransmissionsFd,s.str().c_str(),s.str().size()) == -1)
          std::cout << "ERROR" << std::endl;

      s2 << expiredSeqnum << endl;
      lseek(m_iordFd,0,SEEK_END);
      if (write(m_iordFd,s2.str().c_str(),s2.str().size()) == -1)
          std::cout << "ERROR" << std::endl;

      if (m_ackEnabled)
      {
          m_timerEvents[expiredSeqnum % m_windowSize] = Simulator::Schedule (3 * m_rttCalculator.getMeanRtt(), &NetDeviceFace::Retransmit, this, expiredSeqnum);
      }

    }
}


void 
NetDeviceFace::AckTimerTimeout()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(m_ackTimerId.IsExpired());
    if (m_toBeAcked.size() == 0)    //No Packet Transmitted during last RTT. This doesn't consider LinkEcho, reply which also have to be considered.
    {
        return;
    }
    
    //If a packet is transmitted during last RTT
    std::list<uint32_t>::iterator it;
    shared_ptr<LinkPbAck> ack(new LinkPbAck);
    NS_LOG_INFO("Node " << m_node->GetId() << ":" << "In Acktimertimout, acknowledging : ");
    for (it = m_toBeAcked.begin(); it != m_toBeAcked.end(); it++)
    {
        ack->addAckNumber(*it);
        NS_LOG_INFO(*it);
    }
   
    Ptr<Packet> packet = Convert::ToPacket<LinkPbAck>(*ack);
    m_netDevice->Send(packet, m_netDevice->GetBroadcast(), L3Protocol::ETHERNET_FRAME_TYPE);
    m_toBeAcked.clear();

    ostringstream s;
    s << 1 << endl;
    lseek(m_acksFd,0,SEEK_END);
    if (write(m_acksFd,s.str().c_str(),s.str().size()) == -1)
        std::cout << "ERROR" << std::endl;    
}

bool
NetDeviceFace::canRetransmit(uint32_t seqnum)
{
  NS_LOG_FUNCTION(this << seqnum);
  
  std::map<uint32_t,uint32_t>::iterator it = m_retransmissions.find(seqnum);
  if (it == m_retransmissions.end() && m_maxRetransmissions >= 1)
  {
      m_retransmissions[seqnum] = 1;
      return true;
  }
  else if ( it != m_retransmissions.end() )
  {
      if ( m_retransmissions[seqnum] < m_maxRetransmissions )
      {
          m_retransmissions[seqnum] = m_retransmissions[seqnum]+1; 
          return true;
      }
  }
  return false;
}

bool
NetDeviceFace::canSendNack(uint32_t seqnum)
{
   NS_LOG_FUNCTION(this << seqnum);
   // if (seqnum < m_packetExpected) //If packet is already received or dropped
   //   return false;

   if (m_nackTimers.find(seqnum) != m_nackTimers.end())  //If there is an outstanding nack already
      return false;

   if (m_nacksSent.find(seqnum) == m_nacksSent.end())
   {
      if (m_maxNacks > 0)
      {
         m_nacksSent[seqnum] = 1;
         return true;
      }
   }
   else if (m_nacksSent[seqnum] < m_maxNacks)
   {
      m_nacksSent[seqnum]++;
      return true;
   }

   return false;        
}

void
NetDeviceFace::processIncomingPacket(Ptr<Packet> &packet, Address from, uint16_t protocol)
{
    NS_LOG_FUNCTION(this);
    
    bool containDataOrInterest = false;
    
    SequenceCountTag seqTag;
    packet->RemovePacketTag(seqTag);
    
    FwHopCountTag tag;
    packet->RemovePacketTag(tag);
    NS_LOG_DEBUG("Node " << m_node->GetId() << ":" << "Received packet with hop count "<< tag.Get());
    packet->AddPacketTag(tag);
    
    uint32_t type = Convert::getPacketType(packet);
    if (type == ::ndn::tlv::Interest)
    {
        containDataOrInterest = true;
        shared_ptr<const Interest> i = Convert::FromPacket<Interest>(packet);
        this->onReceiveInterest(*i);  
    }
    else if (type == ::ndn::tlv::Data)
    {
        containDataOrInterest = true;
        shared_ptr<const Data> d = Convert::FromPacket<Data>(packet);
        this->onReceiveData(*d);
    }
    else if (type == ::ndn::tlv::LinkPbAck)
    {
        shared_ptr<const LinkPbAck> a = Convert::FromPacket<LinkPbAck>(packet);
        if (a->containData())
        {
            containDataOrInterest = true;             
            shared_ptr<const Data> d(new Data(a->getData()));
            this->onReceiveData(*d);
        }
        else if (a->containInterest())
        {
            containDataOrInterest = true;
            shared_ptr<const Interest> i(new Interest(a->getInterest()));
            this->onReceiveInterest(*i);  
        }
            
        /*Process acknowledgments and repeat requests*/
        processAckList(a->getAckList());
        processRepeatList(a->getRepeatList());
    }
    else
    {
        NS_LOG_ERROR("Node " << m_node->GetId() << ":" << "unsupported/unrecognized packet format");
    }
       
    if (containDataOrInterest)
    {
        ackIncomingSeqnum(seqTag.Get());
        //Calculating last packet received
        if (m_packetExpected == 0 && m_lastPacketReceived == 0)     //For the first packet that is received
        {
            m_lastPacketReceived = seqTag.Get();
        }
        else if (m_lastPacketReceived < seqTag.Get())
            m_lastPacketReceived = seqTag.Get();
        
        cancelNackTimer(seqTag.Get());  //Cancel nack timer if a nack was sent for it
        processHoles();
    }
    else
    {            
        NS_LOG_INFO("Node " << m_node->GetId() << ":" << "Received a pure ack ");
    }
    updateReceiverState();
}

void
NetDeviceFace::ackIncomingSeqnum(uint32_t seqnum)
{
    NS_LOG_FUNCTION(this << seqnum);
    
    if (m_packetExpected <= seqnum)
    {
        NS_LOG_INFO("Node " << m_node->GetId() << ":" << "Processing incoming packet with sequence number " << seqnum);
        //Only if the packet contains an interest or an ack, the sequence number in the packet makes sense and is to be acknowledged
        m_isPacketReceived[seqnum % m_windowSize] = true;
    }
    else
       NS_LOG_INFO("Node " << m_node->GetId() << ":" << "A packet with sequence number " << seqnum << " received. ");
    
    //If acks are disabled, then sender doesn't expect ack by startinga retransmission timer but acks are still sent by receiver.
    //These acks help in updating sender buffer.
    m_toBeAcked.push_back(seqnum); 
    if (! m_ackTimerId.IsRunning())
    {
        Time tNext;
        if (m_ackEnabled)   //If acks are enabled, then send acks more faster.
        {
            tNext = m_rttCalculator.getMeanRtt();
        }
        else
            tNext = (7 * m_rttCalculator.getMeanRtt() ) / 2;
        m_ackTimerId = Simulator::Schedule(tNext, &NetDeviceFace::AckTimerTimeout ,this);
    }
}

void 
NetDeviceFace::cancelNackTimer(uint32_t seqnum)
{
    NS_LOG_FUNCTION(this << seqnum);
    if (m_nackTimers.find(seqnum) != m_nackTimers.end())
        Simulator::Cancel(m_nackTimers[seqnum]);
}

void
NetDeviceFace::updateReceiverState()
{
    NS_LOG_FUNCTION(this);
    
    while(true)
    {
        if (not m_isPacketReceived[m_packetExpected % m_windowSize])
        {
            //packet with sequence no. m_packetExpected is not received
            if (m_maxNacks == 0) //If nacks are disabled, then start a timer for dropping the packet.
            {
                if (!m_dropTimerId.IsRunning()) //If drop timer is not started already
                    m_dropTimerId = Simulator::Schedule(3*m_maxRetransmissions*m_rttCalculator.getMeanRtt(), &NetDeviceFace::dropIncomingPacket, this,  m_packetExpected);
                break;  
            }

            if (m_nacksSent.find(m_packetExpected) == m_nacksSent.end() && m_maxNacks > 0) //No nack is sent upto now. The packet can't be ignored. 
                break;
            if (m_nacksSent[m_packetExpected] < m_maxNacks)    //Maximum no. of nacks are not sent. Hence can't be ignored.
                break;
            if (m_nackTimers.find(m_packetExpected) != m_nackTimers.end()) //Maximum no. of retransmissions are sent but last retransmission is outstanding. Can't be ignored.
                break;

            //m_packetExpected is ignored at receiver
            NS_LOG_INFO("Node " << m_node->GetId() << ":" << "Packet with sequence no. " << m_packetExpected << " is ignored at receiver");
            // m_receiverDropsFile << "\n " << m_packetExpected;
            // m_receiverDropsFile.flush();

            ostringstream s;
            s << m_packetExpected << endl;
            lseek(m_receiverDropsFd,0,SEEK_END);
            if (write(m_receiverDropsFd,s.str().c_str(),s.str().size()) == -1)
              std::cout << "ERROR" << std::endl;
        }
        Simulator::Cancel(m_dropTimerId);
        m_isPacketReceived[m_packetExpected % m_windowSize] = false;
        incSeqCount(m_packetExpected);
    }
    
    NS_LOG_INFO("Node " << m_node->GetId() << ":" << "Next packet expected is " << m_packetExpected);
}

void
NetDeviceFace::dropIncomingPacket(uint32_t seqnum)
{
    NS_LOG_FUNCTION(this << seqnum);
    if (seqnum < m_packetExpected) return;
    else if (seqnum == m_packetExpected)
    {
        NS_LOG_INFO("Node " << m_node->GetId() << ":" << "Packet with sequence no. " << m_packetExpected << " is ignored at receiver");
        // m_receiverDropsFile << "\n " << m_packetExpected;
        // m_receiverDropsFile.flush();

        ostringstream s;
        s << m_packetExpected << endl;
        lseek(m_receiverDropsFd,0,SEEK_END);
        if (write(m_receiverDropsFd,s.str().c_str(),s.str().size()) == -1)
            std::cout << "ERROR" << std::endl;

        incSeqCount(m_packetExpected);
    }
}


// callback
void
NetDeviceFace::receiveFromNetDevice(Ptr<NetDevice> device, Ptr<const Packet> p, uint16_t protocol,
                                    const Address& from, const Address& to,
                                    NetDevice::PacketType packetType)
{
  NS_LOG_FUNCTION(this << device << p << protocol << from << to << packetType);
  Ptr<Packet> packet = p->Copy();
  
  try {
    uint32_t type = Convert::getPacketType(packet);
    if (type == ::ndn::tlv::Interest || type == ::ndn::tlv::Data || type == ::ndn::tlv::LinkPbAck) {
      NS_LOG_INFO("Node " << m_node->GetId() << ":" << "received Interest/Data/PbAck from netdevice " << from);
      processIncomingPacket(packet,from,protocol);
    }
    else if (type == ::ndn::tlv::LinkEcho)
    {
      NS_LOG_LOGIC("Node " << m_node->GetId() << ":" << "received LinkEcho from netdevice " << from);
      m_rttCalculator.onReceiveLinkEcho(packet, from, protocol);
    }
    else if (type == ::ndn::tlv::LinkReply)
    {
      NS_LOG_LOGIC("Node " << m_node->GetId() << ":" << "received LinkReply from netdevice " << from);
      m_rttCalculator.onReceiveLinkReply(packet);
    }   
    else if (type == ::ndn::tlv::RepeatRequest)
    {
      NS_LOG_INFO("Node " << m_node->GetId() << ":" << "received RepeatRequest from netdevice " << from);
      onReceiveRepeatRequest(packet);
    }
    
    else {
      NS_LOG_ERROR("Node " << m_node->GetId() << ":" << "Unsupported TLV packet");
    }
  }
  catch (::ndn::tlv::Error&) {
    NS_LOG_ERROR("Node " << m_node->GetId() << ":" << "Unrecognized TLV packet");
  }
}

} // namespace ndn
} // namespace ns3
