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

#ifndef NFD_FRAGMEMT_CACHE_HPP
#define NFD_FRAGMENT_CACHE_HPP

#include "ndn-common.hpp"
#include <queue>

namespace ns3 {
namespace ndn {

class FragmentCacheEntry
{

public:
  
  FragmentCacheEntry(uint32_t segMax)
  {
    m_segMax = segMax;
  }

  bool isComplete()
  {
    return m_dataFragments.size() >= m_segMax;
  }

  std::map<uint32_t, Data> getDataFragments()
  {
    return m_dataFragments;
  }

  bool addData(uint32_t seqnum, const Data data)
  {
    if (m_dataFragments.size() < m_segMax)
    {
      m_dataFragments[seqnum] = data;
      return true;
    }
    return false;
  }

  uint32_t size() const
  {
    return m_dataFragments.size();
  }

  void print() const
  {
    for (std::map<uint32_t, Data>::const_iterator it = m_dataFragments.begin(); it != m_dataFragments.end(); it++)
      std::cout << it->first << " ";
    std::cout << std::endl;
  }

  bool contains(uint32_t seqno) const
  {
    for (std::map<uint32_t, Data>::const_iterator it = m_dataFragments.begin(); it != m_dataFragments.end(); it++)
      if (it->first == seqno)
        return true;
    return false;
  }

private:
  uint32_t m_segMax;
  std::map<uint32_t, Data> m_dataFragments;
};


/** \brief represents cache
 */
class FragmentCache
{
public:

  explicit
  FragmentCache(size_t nMaxPackets = 1024);

  ~FragmentCache();

  bool
  insert(const Data& data);

  FragmentCacheEntry *
  find(const uint32_t seqnum);

  void
  erase(const uint32_t seqnum);

  /** \brief sets maximum allowed size of cache (in packets)
   */
  void
  setLimit(size_t nMaxPackets);

  /** \brief returns maximum allowed size of Cache (in packets)
   *  \return{ number of packets that can be stored in cache }
   */
  size_t
  getLimit() const;

  /** \brief returns current size of cache measured in packets
   *  \return{ number of packets located in cache }
   */
  size_t
  size() const;

  size_t
  totalFragments() const;

  void
  print() const;

  bool 
  contains(uint32_t seqno);

protected:
  /** \brief removes one Data packet from cache based on replacement policy
   *  \return{ whether the Data was removed }
   */
  void
  evict();

private:
  /** \brief returns True if the cache is at its maximum capacity
   *  \return{ True if cache is full; otherwise False}
   */
  bool
  isFull() const;

private:
  size_t m_nMaxPackets; // user defined maximum size of the cache in packets
  std::map<uint32_t, FragmentCacheEntry*> m_packets;
  size_t m_segMax;

};

} // namespace ndn
} // namespace ns3

#endif // NFD_FRAGMENT_CACHE_HPP
