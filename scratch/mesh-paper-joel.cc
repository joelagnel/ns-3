/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008,2009 IITP RAS
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
 * Author: Kirill Andreev <andreev@iitp.ru>
 *
 *
 * By default this script creates m_xSize * m_ySize square grid topology with
 * IEEE802.11s stack installed at each node with peering management
 * and HWMP protocol.
 * The side of the square cell is defined by m_step parameter.
 * When topology is created, UDP ping is installed to opposite corners
 * by diagonals. packet size of the UDP ping and interval between two
 * successive packets is configurable.
 * 
 *  See also MeshTest::Configure to read more about configurable
 *  parameters.
 */

#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mesh-module.h"
#include "ns3/mobility-module.h"
#include "ns3/propagation-module.h"
#include "ns3/mesh-helper.h"
#include "ns3/visualizer-module.h"

#include <iostream>
#include <sstream>
#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TestMeshScript");
class MeshTest
{
  public:
    /// Init test
    MeshTest ();
    /// Configure test from command line arguments
    void Configure (int argc, char ** argv);
    /// Run test
    int Run ();

    void ReceivePacket (Ptr<Socket> socket);
    void InterfererReceivePacket (Ptr<Socket> socket);

    Ptr<Socket> SetupPacketReceive (Ptr<Node> node);
    Ptr<Socket> SetupInterfererPacketReceive (Ptr<Node> node);
	Ptr<MobilityModel> GetMobilityModel (int node_num);

  private:
    int       m_xSize;
    int       m_ySize;
    double    m_step;
    double    m_randomStart;
    double    m_totalTime;
    double    m_packetInterval;
    double    m_interfererStartTime;
    double    m_interfererBytesTotal;
    double    m_bytesTotal;
    uint16_t  m_packetSize;
    uint32_t  m_nIfaces;
    uint32_t  m_interfererSrc;
    uint32_t  m_interfererSink;
    bool      m_chan;
    bool      m_pcap;
    bool      m_interferer;
    std::string m_stack;
    std::string m_root;
    std::string m_interfererDataRate;
    /// List of network nodes
    NodeContainer nodes;
    /// List of all mesh point devices
    NetDeviceContainer meshDevices;
    //Addresses of interfaces:
    Ipv4InterfaceContainer interfaces;
    // MeshHelper. Report is not static methods
    MeshHelper mesh;
  private:
    /// Create nodes and setup their mobility
    void CreateNodes ();
    void ForcePath(int mp, int destination, int retransmitter);
    /// Install internet m_stack on nodes
    void InstallInternetStack ();
    /// Install applications
    void InstallApplication ();
    /// Print mesh devices diagnostics
    void Report ();  
};
MeshTest::MeshTest () :
  m_xSize (3),
  m_ySize (3),
  m_step (100.0),
  m_randomStart (0.1),
  m_totalTime (100.0),
  m_packetInterval (0.1),
  m_interfererStartTime (6.0),
  m_interfererBytesTotal (0.0),
  m_bytesTotal (0.0),
  m_packetSize (1024),
  m_nIfaces (1),
  m_interfererSrc (4),
  m_interfererSink (5),
  m_chan (false),
  m_pcap (false),
  m_interferer (false),
  m_stack ("ns3::Dot11sStack"),
  m_root ("ff:ff:ff:ff:ff:ff"),
  m_interfererDataRate ("5000000bps")
{
}
void
MeshTest::Configure (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.AddValue ("x-size", "Number of nodes in a row grid. [6]", m_xSize);
  cmd.AddValue ("y-size", "Number of rows in a grid. [6]", m_ySize);
  cmd.AddValue ("step",   "Size of edge in our grid, meters. [100 m]", m_step);
  /*
   * As soon as starting node means that it sends a beacon,
   * simultaneous start is not good.
   */
  cmd.AddValue ("start",  "Maximum random start delay, seconds. [0.1 s]", m_randomStart);
  cmd.AddValue ("time",  "Simulation time, seconds [100 s]", m_totalTime);
  cmd.AddValue ("int-start-time",  "Time at which the interferer starts transmitting", m_interfererStartTime);
  cmd.AddValue ("packet-interval",  "Interval between packets in UDP ping, seconds [0.001 s]", m_packetInterval);
  cmd.AddValue ("packet-size",  "Size of packets in UDP ping", m_packetSize);
  cmd.AddValue ("interfaces", "Number of radio interfaces used by each mesh point. [1]", m_nIfaces);
  cmd.AddValue ("channels",   "Use different frequency channels for different interfaces. [0]", m_chan);
  cmd.AddValue ("pcap",   "Enable PCAP traces on interfaces. [0]", m_pcap);
  cmd.AddValue ("stack",  "Type of protocol stack. ns3::Dot11sStack by default", m_stack);
  cmd.AddValue ("root", "Mac address of root mesh point in HWMP", m_root);
  cmd.AddValue ("int-src", "source interfer node's index", m_interfererSrc);
  cmd.AddValue ("int-sink", "sink interfer node's index", m_interfererSink);
  cmd.AddValue ("int-data-rate", "data rate of interfer node", m_interfererDataRate);
  cmd.AddValue ("interferer", "introdce an interferer node", m_interferer);
  
  cmd.Parse (argc, argv);
  NS_LOG_DEBUG ("Simulation time: " << m_totalTime << " s");
}

void
MeshTest::ReceivePacket (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  while (packet = socket->Recv ())
    {
      m_bytesTotal += packet->GetSize ();
    }
}

void
MeshTest::InterfererReceivePacket (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  while (packet = socket->Recv ())
    {
      m_interfererBytesTotal += packet->GetSize ();
    }
}

Ptr<Socket>
MeshTest::SetupPacketReceive (Ptr<Node> node)
{
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink = Socket::CreateSocket (node, tid);
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink->Bind (local);
  recvSink->SetRecvCallback (MakeCallback (&MeshTest::ReceivePacket, this));
  return recvSink;

  /*TypeId tid = TypeId::LookupByName ("ns3::PacketSocketFactory");
  Ptr<Socket> sink = Socket::CreateSocket (node, tid);
  sink->Bind ();
  sink->SetRecvCallback (MakeCallback (&MeshTest::ReceivePacket, this));
  return sink;*/
}

Ptr<Socket>
MeshTest::SetupInterfererPacketReceive (Ptr<Node> node)
{
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink = Socket::CreateSocket (node, tid);
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 90);
  recvSink->Bind (local);
  recvSink->SetRecvCallback (MakeCallback (&MeshTest::InterfererReceivePacket, this));
  return recvSink;

  /*TypeId tid = TypeId::LookupByName ("ns3::PacketSocketFactory");
  Ptr<Socket> sink = Socket::CreateSocket (node, tid);
  sink->Bind ();
  sink->SetRecvCallback (MakeCallback (&MeshTest::InterfererReceivePacket, this));
  return sink;*/
}

Ptr<MobilityModel> MeshTest::GetMobilityModel (int node_num)
{
	return nodes.Get (node_num)->GetObject<MobilityModel>();
}

void
MeshTest::ForcePath(int current, int destination, int retransmitter)
{
	Mac48Address destination_mac = Mac48Address::ConvertFrom(meshDevices.Get(destination)->GetAddress());
	Mac48Address retransmitter_mac = Mac48Address::ConvertFrom(meshDevices.Get(retransmitter)->GetAddress());
	Ptr<ns3::MeshPointDevice> mp = dynamic_cast<ns3::MeshPointDevice*>(PeekPointer(meshDevices.Get(current)));
	Ptr<ns3::dot11s::HwmpProtocol> m_proto = dynamic_cast<ns3::dot11s::HwmpProtocol*>(PeekPointer(mp->GetRoutingProtocol()));
	m_proto->ForcePath(destination_mac, retransmitter_mac);
}

void
MeshTest::CreateNodes ()
{ 
  /*
   * Create m_ySize*m_xSize stations to form a grid topology
   */
  nodes.Create (6);

  // Add names for nodes
  Names::Add ("sink", nodes.Get (3));
  Names::Add ("source", nodes.Get (0));
 
  if (m_interferer) 
  {
    Names::Add ("intSink", nodes.Get (m_interfererSink));
    Names::Add ("intSource", nodes.Get (m_interfererSrc));
  }

  // Configure YansWifiChannel
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO); 
  //wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue (-120.0) );
  //wifiPhy.Set ("RxGain", DoubleValue (2.) );
  YansWifiChannelHelper wifiChannelHelper = YansWifiChannelHelper::Default ();
  Ptr<YansWifiChannel> wifiChannel = wifiChannelHelper.Create ();
  wifiPhy.SetChannel (wifiChannel);
  /*
   * Create mesh helper and set stack installer to it
   * Stack installer creates all needed protocols and install them to
   * mesh point device
   */
  mesh = MeshHelper::Default ();
  if (!Mac48Address (m_root.c_str ()).IsBroadcast ())
    {
      mesh.SetStackInstaller (m_stack, "Root", Mac48AddressValue (Mac48Address (m_root.c_str ())));
    }
  else
    {
      //If root is not set, we do not use "Root" attribute, because it
      //is specified only for 11s
      mesh.SetStackInstaller (m_stack);
    }
  if (m_chan)
    {
      mesh.SetSpreadInterfaceChannels (MeshHelper::SPREAD_CHANNELS);
    }
  else
    {
      mesh.SetSpreadInterfaceChannels (MeshHelper::ZERO_CHANNEL);
    }

  //mesh.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("OfdmRate54Mbps"));
  mesh.SetMacType ("RandomStart", TimeValue (Seconds(m_randomStart)));
  // Set number of interfaces - default is single-interface mesh point
  mesh.SetNumberOfInterfaces (m_nIfaces);
  // Install protocols and return container if MeshPointDevices
  meshDevices = mesh.Install (wifiPhy, nodes);

  Names::Add ("sink/mesh0", meshDevices.Get (3));
  Names::Add ("source/mesh0", meshDevices.Get (0));
  
  if (m_interferer) 
  {
    Names::Add ("intSink/mesh0", meshDevices.Get (m_interfererSink));
    Names::Add ("intSource/mesh0", meshDevices.Get (m_interfererSrc));
  }

  /*
   * Force paths in Mesh network
   */

  ForcePath(0, 3, 1);
  ForcePath(0, 1, 1);

  ForcePath(1, 3, 3);
  ForcePath(1, 0, 0);

  ForcePath(3, 0, 1);
  ForcePath(3, 0, 1);

  // Setup mobility - static topology
  /*
   *         0
   *        / \
   *       2   1   4---5
   *        \ / 
   *         3
   */
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> posAlloc = CreateObject <ListPositionAllocator> ();
  posAlloc->Add (Vector (100, 0, 0)); // A (Source)
  posAlloc->Add (Vector (150, 80, 0)); // C
  posAlloc->Add (Vector (50, 80, 0)); // D
  posAlloc->Add (Vector (100, 160, 0)); // E (Sink)
  posAlloc->Add (Vector (250, 80, 0)); // B (interferer Source)
  posAlloc->Add (Vector (300, 80, 0)); // E (Interferer Sink)
  

  mobility.SetPositionAllocator (posAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);

  /* Create Propogation Loss */
  Ptr<MatrixPropagationLossModel> lossModel = CreateObject<MatrixPropagationLossModel> ();
  lossModel->SetDefaultLoss (300); // set default loss to 200 dB (no link)

  lossModel->SetLoss (GetMobilityModel(0), GetMobilityModel(1), 50);
  lossModel->SetLoss (GetMobilityModel(0), GetMobilityModel(2), 50);
  lossModel->SetLoss (GetMobilityModel(0), GetMobilityModel(3), 100);
  lossModel->SetLoss (GetMobilityModel(1), GetMobilityModel(2), 100);
  lossModel->SetLoss (GetMobilityModel(1), GetMobilityModel(3), 50);
  lossModel->SetLoss (GetMobilityModel(2), GetMobilityModel(3), 50);

  lossModel->SetLoss (GetMobilityModel(4), GetMobilityModel(5), 50);
  lossModel->SetLoss (GetMobilityModel(1), GetMobilityModel(4), 100);

   // Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue(10));

  wifiChannel->SetPropagationLossModel (lossModel);

  if (m_pcap)
    wifiPhy.EnablePcapAll (std::string ("mp-"));
}
void
MeshTest::InstallInternetStack ()
{
  InternetStackHelper internetStack;
  internetStack.Install (nodes);
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  interfaces = address.Assign (meshDevices);
}
void
MeshTest::InstallApplication ()
{
  Ptr<Socket> sink = SetupPacketReceive (nodes.Get (3));

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> source = Socket::CreateSocket (nodes.Get (0), tid);
  InetSocketAddress remote = InetSocketAddress (interfaces.GetAddress (3, 0), 80);
  if (source->Connect (remote) < 0)
    NS_LOG_DEBUG ("Error in connect");

  OnOffHelper onoff ("ns3::UdpSocketFactory", Address (remote));
  onoff.SetAttribute ("OnTime", RandomVariableValue (ConstantVariable (m_totalTime-2)));
  onoff.SetAttribute ("OffTime", RandomVariableValue (ConstantVariable (0)));
  onoff.SetAttribute ("DataRate", DataRateValue (DataRate ("5000000bps")));
  onoff.SetAttribute ("PacketSize", UintegerValue (1024));
  
  ApplicationContainer apps = onoff.Install (nodes.Get (0));
  apps.Start (Seconds (0.0));
  apps.Stop (Seconds (m_totalTime));

  
  if (m_interferer)
  {
	  cout<<"Installing interferer. start-time: " << m_interfererStartTime << endl;
	  /*PacketSocketAddress isocket;
	  isocket.SetSingleDevice(meshDevices.Get (m_interfererSrc)->GetIfIndex ());
	  isocket.SetPhysicalAddress (meshDevices.Get (m_interfererSink)->GetAddress ());
	  isocket.SetProtocol (1);*/
	  Ptr<Socket> isink = SetupInterfererPacketReceive (nodes.Get (m_interfererSink));

	  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
	  Ptr<Socket> isource = Socket::CreateSocket (nodes.Get (m_interfererSrc), tid);
	  InetSocketAddress iremote = InetSocketAddress (interfaces.GetAddress (m_interfererSink, 0), 90);
	  if (isource->Connect (iremote) < 0)
            NS_LOG_DEBUG ("Error in connect");
	  OnOffHelper ionoff ("ns3::UdpSocketFactory", iremote);
	  ionoff.SetAttribute ("OnTime", RandomVariableValue (ConstantVariable (m_totalTime-m_interfererStartTime)));
	  ionoff.SetAttribute ("OffTime", RandomVariableValue (ConstantVariable (0)));
	  ionoff.SetAttribute ("DataRate", DataRateValue (DataRate (m_interfererDataRate)));
	  ionoff.SetAttribute ("PacketSize", UintegerValue (1024));

	  ApplicationContainer iapps = ionoff.Install (nodes.Get (m_interfererSrc));
	  iapps.Start (Seconds (m_interfererStartTime));
	  iapps.Stop (Seconds (50.0));
  }

}
int
MeshTest::Run ()
{
  CreateNodes ();
  InstallInternetStack ();
  InstallApplication ();
  Simulator::Schedule (Seconds(m_totalTime), & MeshTest::Report, this);
  Simulator::Stop (Seconds (m_totalTime));
  Simulator::Run ();
  // Visualizer::Run ();
  Simulator::Destroy ();
  return 0;
}
void
MeshTest::Report ()
{
  unsigned n (0);
  for (NetDeviceContainer::Iterator i = meshDevices.Begin (); i != meshDevices.End (); ++i, ++n)
  {
    std::ostringstream os;
    os << "mp-report-" << n << ".xml";
    std::cerr << "Printing mesh point device #" << n << " diagnostics to " << os.str () << "\n";
    std::ofstream of;
    of.open (os.str().c_str());
    if (! of.is_open ())
    {
      std::cerr << "Error: Can't open file " << os.str() << "\n";
      return;
    }
    mesh.Report (*i, of);
    of.close ();

  }

  std::cout << "Throughput: " << m_bytesTotal/(m_totalTime-2) << std::endl;

  if (m_interferer)
    std::cout << "Interferer Throughput: " << m_interfererBytesTotal/(m_totalTime-m_interfererStartTime) << std::endl;
}
int
main (int argc, char *argv[])
{
  ns3::PacketMetadata::Enable();
  // LogComponentEnable("HwmpRtable", LOG_LEVEL_DEBUG);
  // LogComponentEnable("HwmpProtocol", LOG_LEVEL_DEBUG);
  // LogComponentEnable("MeshPointDevice", LOG_LEVEL_DEBUG);
  MeshTest t; 
  t.Configure (argc, argv);
  return t.Run();
}

/*posAlloc->Add (Vector (-20, 0, 0)); // A (Source)
  posAlloc->Add (Vector (70, 100, 0)); // B (interferer Source)
  posAlloc->Add (Vector (0, 100, 0)); // C
  posAlloc->Add (Vector (-70, 100, 0)); // D
  posAlloc->Add (Vector (-20, 170, 0)); // E (Sink)
  posAlloc->Add (Vector (170, 100, 0)); // E (Interferer Sink)*/
  
  /*posAlloc->Add (Vector (100, 10, 0)); // A (Source)
  posAlloc->Add (Vector (110, 35, 0)); // B
  posAlloc->Add (Vector (90, 35, 0)); // C
  //posAlloc->Add (Vector (110, 45, 0)); // D
  //posAlloc->Add (Vector (90, 45, 0)); // E
  //posAlloc->Add (Vector (100, 55, 0)); // E (Sink)
  posAlloc->Add (Vector (100, 60, 0)); // E (Sink)
  posAlloc->Add (Vector (130, 35, 0)); // B (interferer Source)
  posAlloc->Add (Vector (150, 35, 0)); // E (Interferer Sink)*/

