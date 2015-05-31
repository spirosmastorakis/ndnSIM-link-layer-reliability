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

#include "ndn-consumer.hpp"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/double.h"
#include "iostream"
#include "fstream"
#include "utils/ndn-ns3-packet-tag.hpp"
#include "model/ndn-app-face.hpp"
#include "utils/ndn-rtt-mean-deviation.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/ref.hpp>

NS_LOG_COMPONENT_DEFINE("ndn.Consumer");
using namespace std;

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(Consumer);

TypeId
Consumer::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::Consumer")
      .SetGroupName("Ndn")
      .SetParent<App>()
      .AddAttribute("StartSeq", "Initial sequence number", IntegerValue(0),
                    MakeIntegerAccessor(&Consumer::m_seq), MakeIntegerChecker<int32_t>())

      .AddAttribute("Prefix", "Name of the Interest", StringValue("/"),
                    MakeNameAccessor(&Consumer::m_interestName), MakeNameChecker())
  
      .AddAttribute("LifeTime", "LifeTime for interest packet", StringValue("40ms"),
                    MakeTimeAccessor(&Consumer::m_interestLifeTime), MakeTimeChecker())

      .AddAttribute("RetxTimer",
                    "Timeout defining how frequent retransmission timeouts should be checked",
                    StringValue("20ms"),
                    MakeTimeAccessor(&Consumer::GetRetxTimer, &Consumer::SetRetxTimer),
                    MakeTimeChecker())

      .AddTraceSource("LastRetransmittedInterestDataDelay",
                      "Delay between last retransmitted Interest and received Data",
                      MakeTraceSourceAccessor(&Consumer::m_lastRetransmittedInterestDataDelay))

      .AddTraceSource("FirstInterestDataDelay",
                      "Delay between first transmitted Interest and received Data",
                      MakeTraceSourceAccessor(&Consumer::m_firstInterestDataDelay))
                      
      .AddAttribute ("RTO",
                    "Retransmission Timeout",
                    StringValue("50ms"),
                    MakeTimeAccessor(&Consumer::m_rto),
                    MakeTimeChecker())
    ;

  return tid;
}

Consumer::Consumer()
  : m_rand(0, std::numeric_limits<uint32_t>::max())
  , m_seq(0)
  , m_seqMax(0) // don't request anything
{
  NS_LOG_FUNCTION_NOARGS();

  m_rtt = CreateObject<RttMeanDeviation>();
  m_rtt->SetMaxRto(MilliSeconds(200));
}

void
Consumer::SetRetxTimer(Time retxTimer)
{
  m_retxTimer = retxTimer;
  if (m_retxEvent.IsRunning()) {
    // m_retxEvent.Cancel (); // cancel any scheduled cleanup events
    Simulator::Remove(m_retxEvent); // slower, but better for memory
  }

  // schedule even with new timeout
  m_retxEvent = Simulator::Schedule(m_retxTimer, &Consumer::CheckRetxTimeout, this);
}

Time
Consumer::GetRetxTimer() const
{
  return m_retxTimer;
}

void
Consumer::CheckRetxTimeout()
{
  Time now = Simulator::Now();
  // Time rto = m_rtt->RetransmitTimeout();
  Time rto = m_rto;

  // NS_LOG_DEBUG ("Current RTO: " << rto.ToDouble (Time::S) << "s");
  //std::cout << "check retx timeout - RTO " << rto << std::endl;
  
  while (!m_seqTimeouts.empty()) {
    SeqTimeoutsContainer::index<i_timestamp>::type::iterator entry =
      m_seqTimeouts.get<i_timestamp>().begin();
    if (entry->time + rto <= now) // timeout expired?
    {
      uint32_t seqNo = entry->seq;
      m_seqTimeouts.get<i_timestamp>().erase(entry);
      OnTimeout(seqNo);
    }
    else
      break; // nothing else to do. All later packets need not be retransmitted
  }

  m_retxEvent = Simulator::Schedule(m_retxTimer, &Consumer::CheckRetxTimeout, this);
}

// Application Methods
void
Consumer::StartApplication() // Called at time specified by Start
{
  NS_LOG_FUNCTION_NOARGS();

  // do base stuff
  App::StartApplication();

  ScheduleNextPacket();
}

void
Consumer::StopApplication() // Called at time specified by Stop
{
  NS_LOG_FUNCTION_NOARGS();

  // cancel periodic packet generation
  Simulator::Cancel(m_sendEvent);

  // cleanup base stuff
  App::StopApplication();
  
  PrintRetransmissions();
   
}


void
Consumer::SendPacket()
{
  if (!m_active)
    return;
  
  NS_LOG_FUNCTION_NOARGS();

  uint32_t seq = std::numeric_limits<uint32_t>::max(); // invalid

  while (m_retxSeqs.size()) {
    seq = *m_retxSeqs.begin();
    m_retxSeqs.erase(m_retxSeqs.begin());
    break;
  }

  if (seq == std::numeric_limits<uint32_t>::max()) {
    if (m_seqMax != std::numeric_limits<uint32_t>::max()) {
      if (m_seq >= m_seqMax) {
        ////std::cout << "max seq reached." << std::endl;
        return; // we are totally done
      }
    }

    seq = m_seq++;
  }
  
  //
  shared_ptr<Name> nameWithSequence = make_shared<Name>(m_interestName);
  nameWithSequence->appendSequenceNumber(seq);
  //
  
  // shared_ptr<Interest> interest = make_shared<Interest> ();
  shared_ptr<Interest> interest = make_shared<Interest>();
  interest->setNonce(m_rand.GetValue());
  interest->setName(*nameWithSequence);
  time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
  interest->setInterestLifetime(interestLifeTime);

  // NS_LOG_INFO ("Requesting Interest: \n" << *interest);

  NS_LOG_INFO("> Interest for " << seq);

  WillSendOutInterest(seq);

  m_transmittedInterests(interest, this, m_face);
  m_face->onReceiveInterest(*interest);

  ScheduleNextPacket();
}

///////////////////////////////////////////////////
//          Process incoming packets             //
///////////////////////////////////////////////////

void
Consumer::OnData(shared_ptr<const Data> data)
{
  if (!m_active)
    return;

  App::OnData(data); // tracing inside

  NS_LOG_FUNCTION(this << data);

  // NS_LOG_INFO ("Received content object: " << boost::cref(*data));

  // This could be a problem......
  uint32_t seq = data->getName().at(-1).toSequenceNumber();
  NS_LOG_INFO("< DATA for " << seq);

  int hopCount = -1;
  auto ns3PacketTag = data->getTag<Ns3PacketTag>();
  if (ns3PacketTag != nullptr) {
    FwHopCountTag hopCountTag;
    if (ns3PacketTag->getPacket()->PeekPacketTag(hopCountTag)) {
      hopCount = hopCountTag.Get();
      NS_LOG_DEBUG("Hop count: " << hopCount);
    }
  }

  SeqTimeoutsContainer::iterator entry = m_seqLastDelay.find(seq);
  if (entry != m_seqLastDelay.end()) {
    m_lastRetransmittedInterestDataDelay(this, seq, Simulator::Now() - entry->time, hopCount);
  }

  entry = m_seqFullDelay.find(seq);
  if (entry != m_seqFullDelay.end()) {
    m_firstInterestDataDelay(this, seq, Simulator::Now() - entry->time, m_seqRetxCounts[seq], hopCount);
  }

  m_seqRetxCounts.erase(seq);
  m_seqFullDelay.erase(seq);
  m_seqLastDelay.erase(seq);

  m_seqTimeouts.erase(seq);
  m_retxSeqs.erase(seq);
  
  if (m_totalDelay.find(seq) != m_totalDelay.end())
  {
      m_totalDelay[seq] = Simulator::Now() - m_totalDelay[seq];
  }
  
  m_rtt->AckSeq(SequenceNumber32(seq));
}

void
Consumer::OnTimeout(uint32_t sequenceNumber)
{
  NS_LOG_FUNCTION(sequenceNumber);
  // ////std::cout << Simulator::Now () << ", TO: " << sequenceNumber << ", current RTO: " <<
  // m_rtt->RetransmitTimeout ().ToDouble (Time::S) << "s\n";
  ////std::cout << "Ontimeout" << sequenceNumber << std::endl;
  m_rtt->IncreaseMultiplier(); // Double the next RTO
  m_rtt->SentSeq(SequenceNumber32(sequenceNumber),
                 1); // make sure to disable RTT calculation for this sample
  m_retxSeqs.insert(sequenceNumber);

  IncreaseRetransmissionCount(sequenceNumber);

  ScheduleNextPacket();
}

void
Consumer::IncreaseRetransmissionCount(uint32_t seqnum)
{
  std::map<uint32_t,uint32_t>::iterator it = m_retransmissions.find(seqnum);
  if (it == m_retransmissions.end())
      m_retransmissions[seqnum] = 1;
  else
      m_retransmissions[seqnum] = m_retransmissions[seqnum]+1; 
}

void
Consumer::PrintRetransmissions()
{
  NS_LOG_FUNCTION(this);
  std::ofstream rFile;
  std::ostringstream s;
  s << "Consumer-" << GetNode()->GetId() << "-Retransmissions.txt";
  rFile.open (s.str());
 
  NS_LOG_INFO("Printing Retransmissions at consumer.");
  uint32_t totalRetransmissions = 0;
  for (std::map<uint32_t, uint32_t>::iterator it = m_retransmissions.begin(); it != m_retransmissions.end(); it++)
  {
    NS_LOG_INFO("Retransmissions : " << it->first << "  ->  " << it->second);
    rFile << it->first << "  ->  " << it->second << std::endl;
    totalRetransmissions += it->second;
  }
  rFile << "Total number of retransmissions are : " << totalRetransmissions << endl;
  rFile << "Average number of retransmissions are : " << float(totalRetransmissions)/m_seq;
  rFile.close();
  
  
  std::ofstream dFile;
  std::ostringstream s2;
  s2 << "Consumer-" << GetNode()->GetId() << "-Delay.csv";
  dFile.open (s2.str());
 
  Time totalDelay, delay;
  for (std::map<uint32_t, Time>::iterator it = m_totalDelay.begin(); it != m_totalDelay.end(); it++)
  {
//    NS_LOG_INFO("Delay : " << it->first << "  ->  " << it->second);
    if (it != m_totalDelay.begin())
      dFile << ",";
    dFile << float((it->second).GetMicroSeconds())/1000;
    
    if ( m_seqTimeouts.find(it->first) == m_seqTimeouts.end() ) //If the interest is satisfied
        delay = it->second;
    
    else
        delay = Simulator::Now() - it->second;
    
    if (it != m_totalDelay.begin())
        totalDelay += delay;
    else
        totalDelay = delay;
  }
  cout << "Total delay : " << totalDelay << endl;
  cout << "Average delay : " << (totalDelay/m_seq) << endl;
  
  
  NS_LOG_INFO("Interests not yet satisfied are :");
  while (!m_seqTimeouts.empty()) {
    
    SeqTimeoutsContainer::index<i_timestamp>::type::iterator entry = m_seqTimeouts.get<i_timestamp>().begin();
    uint32_t seqNo = entry->seq;
    m_seqTimeouts.get<i_timestamp>().erase(entry);
    dFile << seqNo << endl;
  }
}

void
Consumer::WillSendOutInterest(uint32_t sequenceNumber)
{
  NS_LOG_DEBUG("Trying to add " << sequenceNumber << " with " << Simulator::Now() << ". already "
                                << m_seqTimeouts.size() << " items");

  m_seqTimeouts.insert(SeqTimeout(sequenceNumber, Simulator::Now()));
  m_seqFullDelay.insert(SeqTimeout(sequenceNumber, Simulator::Now()));

  m_seqLastDelay.erase(sequenceNumber);
  m_seqLastDelay.insert(SeqTimeout(sequenceNumber, Simulator::Now()));

  m_seqRetxCounts[sequenceNumber]++;

  m_rtt->SentSeq(SequenceNumber32(sequenceNumber), 1);
  
  if (m_totalDelay.find(sequenceNumber) == m_totalDelay.end())
      m_totalDelay[sequenceNumber] = Simulator::Now();
}

} // namespace ndn
} // namespace ns3
