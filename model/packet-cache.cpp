/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
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
 *
 * 
 */

#include "packet-cache.hpp"
#include "ndn-common.hpp"

using namespace std;
namespace ns3 {
namespace ndn {


PacketCache::PacketCache(size_t nMaxPackets)
{
  m_nMaxPackets = nMaxPackets;
}

PacketCache::~PacketCache()
{
  while(m_packets.size() > 0)
    evict();
}

void
PacketCache::insert(uint32_t seqnum, const Data& data)
{
  if (isFull())
    evict();

  PacketCacheEntry *p = new PacketCacheEntry;
  p->setData(data);
  m_packets[seqnum] = p;
}

void
PacketCache::insert(uint32_t seqnum, const Interest &interest)
{
  if (isFull())
    evict();

  PacketCacheEntry *p = new PacketCacheEntry;
  p->setInterest(interest);
  m_packets[seqnum] = p;
}

PacketCacheEntry *PacketCache::find(const uint32_t seqnum)
{
  if (m_packets.find(seqnum) == m_packets.end())
    return NULL;
  else 
    return m_packets[seqnum];
}

void
PacketCache::erase(const uint32_t seqnum)
{
  if (m_packets.find(seqnum) != m_packets.end())
    delete m_packets[seqnum];
  m_packets.erase(seqnum);
}

void
PacketCache::evict()
{
  if (m_packets.size() > 0)
  {
    delete m_packets.begin()->second;
    m_packets.erase(m_packets.begin());
  }
}

void
PacketCache::setLimit(size_t nMaxPackets)
{
  m_nMaxPackets = nMaxPackets;
  while(m_packets.size() > m_nMaxPackets)
    evict();
}

size_t
PacketCache::getLimit() const
{
  return m_nMaxPackets;
}

size_t
PacketCache::size() const
{
  return m_packets.size();
}

bool
PacketCache::isFull() const
{
  return m_packets.size() >= m_nMaxPackets;
}

} //namespace ndn
} //namespace ns3