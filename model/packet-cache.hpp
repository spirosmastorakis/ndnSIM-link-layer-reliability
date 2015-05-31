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

#ifndef NFD_PACKET_CACHE_HPP
#define NFD_PACKET_CACHE_HPP

#include "ndn-common.hpp"
#include <queue>

namespace ns3 {
namespace ndn {

class PacketCacheEntry
{
public:
  Interest m_interest;
  Data m_data;
  bool m_isInterest;  //true if the entry contains interest. false if it contains data

  bool containsInterest()
  {
    return m_isInterest;
  }

  bool containsData()
  {
    return not m_isInterest;
  }

  Data getData()
  {
    if (not m_isInterest)
      return m_data;
    else
      return Data();
  }

  Interest getInterest()
  {
    if (m_isInterest)
      return m_interest;
    else
      return Interest();
  }

  void setInterest(const Interest interest)
  {
    m_interest = interest;
    m_isInterest = true;
  }

  void setData(const Data data)
  {
    m_data = data;
    m_isInterest = false;
  }

};


/** \brief represents cache
 */
class PacketCache
{
public:


  explicit
  PacketCache(size_t nMaxPackets = 1024);

  ~PacketCache();

  void
  insert(uint32_t seqnum, const Data& data);

  void
  insert(uint32_t seqnum, const Interest &interest);

  PacketCacheEntry *
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
  std::map<uint32_t, PacketCacheEntry*> m_packets;

};

} // namespace ndn
} // namespace ns3

#endif // NFD_PACKET_CACHE_HPP
