/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 MIRKO BANCHI
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
 *
 * Author: Mirko Banchi <mk.banchi@gmail.com>
 */
#include "tsf-tag.h"
#include "ns3/tag.h"
#include "ns3/uinteger.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (TsfTag);

TypeId 
TsfTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TsfTag")
    .SetParent<Tag> ()
    .AddConstructor<TsfTag> ()
    ;
  return tid;
}

TypeId
TsfTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

TsfTag::TsfTag ():
  m_queueTime (Seconds (0.0)),
  m_ackTime (Seconds (0.0))
{}
  
uint32_t 
TsfTag::GetSerializedSize (void) const
{
  return 2*sizeof(double);
}

void
TsfTag::Serialize (TagBuffer i) const
{
  double qtime = Time::ToDouble (m_queueTime, Time::US);
  double acktime = Time::ToDouble (m_ackTime, Time::US);

  const uint8_t *buf = (const uint8_t*)&qtime;

  i.Write (buf, sizeof(double));
  
  const uint8_t *buf2 = (const uint8_t *)&acktime;

  i.Write (buf2, sizeof(double));
}

void
TsfTag::Deserialize (TagBuffer i)
{

  uint8_t *buf = new uint8_t[sizeof(double)];

  for (int j=0; j<(int)sizeof(double); j++)
    buf[j] = i.ReadU8 ();

  double *d = (double *)buf;
  
  m_queueTime = Time::FromDouble (*d, Time::US);
  //std::cout << "**** q time: " << m_queueTime << std::endl;
  
  for (int j=0; j<(int)sizeof(double); j++)
    buf[j] = i.ReadU8 ();

  d = (double *)buf;
  
  m_ackTime = Time::FromDouble (*d, Time::US);
  //std::cout << "*** ack time: " << m_ackTime << std::endl;

  delete buf;
}

Time
TsfTag::GetQueueTime () const
{
  return m_queueTime;
}

Time
TsfTag::GetAckTime () const
{
  return m_ackTime;
}

void
TsfTag::SetQueueTime (Time t)
{
  m_queueTime = t;
}

void
TsfTag::SetAckTime (Time t)
{
  m_ackTime = t;
}

double
TsfTag::Get () const
{
  return Time::ToDouble (m_ackTime - m_queueTime, Time::US);
}

void
TsfTag::Print (std::ostream &os) const
{
  os << "Queue Time= " << m_queueTime;
  os << "Ack Time= " << m_ackTime;
}

} //namespace ns3
