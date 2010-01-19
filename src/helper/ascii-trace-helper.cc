/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 University of Washington
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

#include <stdint.h>
#include <fstream>
#include <string>

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/ptr.h"
#include "ns3/packet.h"
#include "ns3/node.h"
#include "ns3/names.h"
#include "ns3/net-device.h"
#include "ns3/simulator.h"

#include "ascii-trace-helper.h"

NS_LOG_COMPONENT_DEFINE("AsciiTraceHelper");

namespace ns3 {

AsciiTraceHelper::AsciiTraceHelper ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

AsciiTraceHelper::~AsciiTraceHelper ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

Ptr<OutputStreamObject>
AsciiTraceHelper::CreateFileStream (std::string filename, std::string filemode)
{
  NS_LOG_FUNCTION (filename << filemode);

  std::ofstream *ofstream = new std::ofstream;

  ofstream->open (filename.c_str ());
  NS_ABORT_MSG_UNLESS (ofstream->is_open (), "Unable to Open " << filename << " for mode " << filemode);
  
  Ptr<OutputStreamObject> streamObject = CreateObject<OutputStreamObject> ();
  streamObject->SetStream (ofstream);

  //
  // Note that the ascii trace helper promptly forgets all about the trace file.
  // We rely on the reference count of the file object which will soon be owned
  // by the caller to keep the object alive.  If the caller uses the stream 
  // object to hook a trace source, ownership of the stream object will be
  // implicitly transferred to the callback which keeps the object alive.
  // When the callback is destroyed (when either the trace is disconnected or
  // the object with the trace source is deleted) the callback will be destroyed
  // and the stream object will be destroyed, releasing the pointer and closing
  // the underlying file.
  //
  return streamObject;
}

std::string
AsciiTraceHelper::GetFilename (std::string prefix, Ptr<NetDevice> device, bool useObjectNames)
{
  NS_LOG_FUNCTION (prefix << device << useObjectNames);
  NS_ABORT_MSG_UNLESS (prefix.size (), "Empty prefix string");

  std::ostringstream oss;
  oss << prefix << "-";

  std::string nodename;
  std::string devicename;

  Ptr<Node> node = device->GetNode ();

  if (useObjectNames)
    {
      nodename = Names::FindName (node);
      devicename = Names::FindName (device);
    }

  if (nodename.size ())
    {
      oss << nodename;
    }
  else
    {
      oss << node->GetId ();
    }

  oss << "-";

  if (devicename.size ())
    {
      oss << devicename;
    }
  else
    {
      oss << device->GetIfIndex ();
    }

  oss << ".tr";

  return oss.str ();
}

//
// The basic default trace sinks.  Ascii traces are collected for four 
// operations:
//
// Enqueue:
//
//   When a packet has been sent to a device for transmission, the device is
//   expected to place the packet onto a transmit queue even if it does not
//   have to delay the packet at all, if only to trigger this event.  This 
//   event will eventually translate into a '+' operation in the trace file.
//
//   This is typically implemented by hooking the "TxQueue/Enqueue" trace hook
//   in the device (actually the Queue in the device).
//
// Drop:
//
//   When a packet has been sent to a device for transmission, the device is
//   expected to place the packet onto a transmit queue.  If this queue is 
//   full the packet will be dropped.  The device is expected to trigger an
//   event to indicate that an outbound packet is being dropped.  This event
//   will eventually translate into a 'd' operation in the trace file.
//
//   This is typically implemented by hooking the "TxQueue/Drop" trace hook
//   in the device (actually the Queue in the device).
//
// Dequeue:
//
//   When a packet has been sent to a device for transmission, the device is
//   expected to place the packet onto a transmit queue even if it does not
//   have to delay the packet at all.  The device removes the packet from the
//   transmit queue when the packet is ready to send, and this dequeue will 
//   fire a corresponding event.  This event will eventually translate into a 
//   '-' operation in the trace file.
//
//   This is typically implemented by hooking the "TxQueue/Dequeue" trace hook
//   in the device (actually the Queue in the device).
//
// Receive:
//
//   When a packet is received by a device for transmission, the device is
//   expected to trigger this event to indicate the reception has occurred.
//   This event will eventually translate into an 'r' operation in the trace 
//   file.
//
//   This is typically implemented by hooking the "MacRx" trace hook in the
//   device.
//

void
AsciiTraceHelper::DefaultEnqueueSink (Ptr<OutputStreamObject> file, Ptr<const Packet> p)
{
  NS_LOG_FUNCTION (file << p);
  *file->GetStream () << "+ " << Simulator::Now ().GetSeconds () << " " << *p << std::endl;
}

void
AsciiTraceHelper::DefaultDropSink (Ptr<OutputStreamObject> file, Ptr<const Packet> p)
{
  NS_LOG_FUNCTION (file << p);
  *file->GetStream () << "d " << Simulator::Now ().GetSeconds () << " " << *p << std::endl;
}

void
AsciiTraceHelper::DefaultDequeueSink (Ptr<OutputStreamObject> file, Ptr<const Packet> p)
{
  NS_LOG_FUNCTION (file << p);
  *file->GetStream () << "- " << Simulator::Now ().GetSeconds () << " " << *p << std::endl;
}

void
AsciiTraceHelper::DefaultReceiveSink (Ptr<OutputStreamObject> file, Ptr<const Packet> p)
{
  NS_LOG_FUNCTION (file << p);
  *file->GetStream () << "r " << Simulator::Now ().GetSeconds () << " " << *p << std::endl;
}

} // namespace ns3