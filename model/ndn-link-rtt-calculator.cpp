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

#include "ns3/ndnSIM/model/ndn-link-rtt-calculator.hpp"
NS_LOG_COMPONENT_DEFINE("ndn.LinkRttCalculator");

namespace ns3 {
namespace ndn {

LinkRttCalculator::LinkRttCalculator(Ptr<Node> node, Ptr<NetDevice> netDevice)
{
    NS_LOG_FUNCTION(this);
	m_node = node;
	m_netDevice = netDevice;
    m_isRttCalculated = false;
    m_meanRtt = Time::Min();
    m_varRtt = Time::Min();
    CalculateRtt();
}

void
LinkRttCalculator::sendLinkEcho()
{
  NS_LOG_FUNCTION(this);
  
  shared_ptr<LinkEcho> echo(new LinkEcho);
  m_nonce = echo->getNonce();
  Ptr<Packet> packet = Convert::ToPacket(*echo);
  NS_LOG_INFO("Node " << m_node->GetId() << ":" << "sending LinkEcho with nonce = " << m_nonce);
  m_netDevice->Send(packet, m_netDevice->GetBroadcast(), L3Protocol::ETHERNET_FRAME_TYPE);
}

void
LinkRttCalculator::onReceiveLinkReply(Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(this);
    
    shared_ptr<const LinkReply> reply = Convert::FromPacket<LinkReply>(packet);
    uint32_t nonce = reply->getNonce();
    NS_LOG_INFO( "Node " << m_node->GetId() << ":" << "Processing Link reply with nonce = " << nonce);
    
    if (m_nonce == nonce)
    {
        Time remainingTime = Simulator::GetDelayLeft(m_rttTimerId);
        Time Rtt = m_rttTimerValue - remainingTime;
        NS_LOG_INFO( "Node " << m_node->GetId() << "Rtt value = " << Rtt);
        Simulator::Cancel(m_rttTimerId);

        if (m_isRttCalculated == false)
        {
            m_isRttCalculated = true;
            m_meanRtt = Rtt;
            m_varRtt = Rtt/2;
        }
        else
        {
            m_varRtt = (7 * m_varRtt) / 8 + (Abs(m_meanRtt - Rtt)) / 8;
            m_meanRtt = (7 * m_meanRtt) / 8 + Rtt / 8;
        }
        NS_LOG_INFO( "Node " << m_node->GetId() << ":" << " mean Rtt = " << m_meanRtt << "  variance of Rtt = " << m_varRtt);
        Simulator::Schedule(m_meanRtt, &LinkRttCalculator::CalculateRtt, this);
    }
}

void
LinkRttCalculator::onReceiveLinkEcho(Ptr<Packet> &pkt, Address destAddress, uint16_t protocol)
{
    NS_LOG_FUNCTION(this);
    
    shared_ptr<const LinkEcho> echo = Convert::FromPacket<LinkEcho>(pkt);
    uint32_t nonce = echo->getNonce();
    
    NS_LOG_INFO( "Node " << m_node->GetId() << ":" << "Processing Link Echo with nonce = " << nonce << ". Sending Link Reply.");
    
    shared_ptr<LinkReply> reply(new LinkReply);
    reply->setNonce(nonce);
    Ptr<Packet> packet = Convert::ToPacket(*reply);
    
    m_netDevice->Send(packet, destAddress, protocol);
}

void 
LinkRttCalculator::LinkEchoTimeout()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(m_rttTimerId.IsExpired());
    NS_LOG_INFO( "Node " << m_node->GetId() << ":" << "Timeout during Rtt Calculation");
    m_rttTimerValue = m_rttTimerValue * 2;
    sendLinkEcho();
    m_rttTimerId = Simulator::Schedule (m_rttTimerValue, &LinkRttCalculator::LinkEchoTimeout, this);
}

void
LinkRttCalculator::CalculateRtt ()
{
    NS_LOG_FUNCTION(this);
    m_rttTimerValue = MilliSeconds(32);
    NS_LOG_INFO( "Node " << m_node->GetId() << ":" << "Sending Echo for Rtt calculation ");
    sendLinkEcho();
    m_rttTimerId = Simulator::Schedule (m_rttTimerValue, &LinkRttCalculator::LinkEchoTimeout, this);
}

Time
LinkRttCalculator::getMeanRtt()
{
    if (m_isRttCalculated)
        return m_meanRtt;
    else
        return m_rttTimerValue;
}

Time
LinkRttCalculator::getVarRtt()
{
    if (m_isRttCalculated)
        return m_varRtt;
    else
        return m_rttTimerValue;
}

}
}