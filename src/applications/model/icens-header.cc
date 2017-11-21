/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/icens-header.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ns3.iCenSHeader");

NS_OBJECT_ENSURE_REGISTERED (iCenSHeader);

TypeId
iCenSHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::iCenSHeader")
    .SetParent<Header> ()
    ;
  return tid;
}

TypeId
iCenSHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
iCenSHeader::SetSubscription (uint32_t subscription)
{
  m_subscription = subscription;
}
uint32_t
iCenSHeader::GetSubscription (void)
{
  return m_subscription;
}

uint32_t
iCenSHeader::GetSerializedSize (void) const
{
  //Four bytes of data to store
  return 4;
}
void
iCenSHeader::Serialize (Buffer::Iterator start) const
{
  start.WriteHtonU32 (m_subscription);
}
uint32_t
iCenSHeader::Deserialize (Buffer::Iterator start)
{
  m_subscription = start.ReadNtohU32 ();
  return 4;
}
void
iCenSHeader::Print (std::ostream &os) const
{
  os << m_subscription;
}

}//namespace
