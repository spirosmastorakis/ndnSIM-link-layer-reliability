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

#include "fragment-cache.hpp"
#include "ndn-common.hpp"

using namespace std;
namespace ns3 {
namespace ndn {


FragmentCache::FragmentCache(size_t nMaxPackets)
{
  m_nMaxPackets = nMaxPackets;
  m_segMax = 10;
}

FragmentCache::~FragmentCache()
{
  while(m_packets.size() > 0)
    evict();
}

bool    //Returns if the packet gets formed completely on inserting this fragment
FragmentCache::insert(const Data& data)
{
  uint32_t seqno = data.getName().at(1).toSequenceNumber();
  
  if (m_packets.find(seqno / m_segMax) == m_packets.end())
  {
    FragmentCacheEntry *p = new FragmentCacheEntry(m_segMax);
    p->addData(seqno, data);
    m_packets[seqno / m_segMax] = p;
    return p->isComplete();
  }
  else
  {
    m_packets[seqno / m_segMax]->addData(seqno, data);
    return m_packets[seqno / m_segMax]->isComplete();
  }
}

FragmentCacheEntry *FragmentCache::find(const uint32_t seqnum)
{
  if (m_packets.find(seqnum / m_segMax) == m_packets.end())
    return NULL;
  else 
    return m_packets[seqnum / m_segMax];
}

void
FragmentCache::erase(const uint32_t seqnum)
{
  if (m_packets.find(seqnum /  m_segMax) != m_packets.end())
  {
    delete m_packets[seqnum / m_segMax];
    m_packets.erase(seqnum / m_segMax);
  }
}

void
FragmentCache::evict()
{
  if (m_packets.size() > 0)
  {
    delete m_packets.begin()->second;
    m_packets.erase(m_packets.begin());
  }
}

void
FragmentCache::setLimit(size_t nMaxPackets)
{
  m_nMaxPackets = nMaxPackets;
  while(m_packets.size() > m_nMaxPackets)
    evict();
}

size_t
FragmentCache::getLimit() const
{
  return m_nMaxPackets;
}

size_t
FragmentCache::size() const
{
  return m_packets.size();
}

size_t
FragmentCache::totalFragments() const
{
  uint32_t totalSize = 0;
  for (std::map<uint32_t, FragmentCacheEntry*>::const_iterator it = m_packets.begin(); it != m_packets.end(); it++)
  {
    totalSize += (it->second)->size();
  }
  return totalSize;
}

bool
FragmentCache::isFull() const
{
  return m_packets.size() >= m_nMaxPackets;
}

void
FragmentCache::print() const
{
  for (std::map<uint32_t, FragmentCacheEntry*>::const_iterator it = m_packets.begin(); it != m_packets.end(); it++)
  {
    it->second->print();
  }
}

bool
FragmentCache::contains(uint32_t seqno)
{
  if (m_packets.find(seqno / m_segMax) != m_packets.end())
  {
    return m_packets[seqno / m_segMax]->contains(seqno);
  }
  return false;
}

} //namespace ndn
} //namespace ns3