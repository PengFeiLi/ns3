/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Jaume Nin <jaume.nin@cttc.cat>
 */

#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
//#include "ns3/gtk-config-store.h"

using namespace ns3;

/**
 * Sample simulation script for LTE+EPC. It instantiates several eNodeB,
 * attaches one UE per eNodeB starts a flow for each UE to  and from a remote host.
 * It also  starts yet another flow between each UE pair.
 */

NS_LOG_COMPONENT_DEFINE ("EpcFirstExample");

Vector
RandomUEPosition(uint32_t enbRadius)
{
  double radius_d = static_cast<double>(enbRadius);//六边形内切圆范围内随机部署
  Ptr<UniformRandomVariable> randomVar = CreateObject<UniformRandomVariable> ();

  randomVar->SetAttribute ("Min", DoubleValue (-radius_d));
  randomVar->SetAttribute ("Max", DoubleValue (radius_d-30));//避免基站接近公共区域
  double val_x = randomVar->GetValue();

  double val_temp = std::sqrt((enbRadius)*(enbRadius)-val_x*val_x);//避免热点范围超出宏站
  randomVar->SetAttribute ("Min", DoubleValue (-val_temp));
  randomVar->SetAttribute ("Max", DoubleValue (val_temp));
  double val_y = randomVar->GetValue();

  double val_height = 0;//1.5;

  Vector pos(val_x, val_y, val_height);

  return pos;
}

Ptr<ListPositionAllocator>
PicoPositions(NodeContainer c, uint32_t enbRadius, double enbYDis)
{
  //再配置最后四个微站位置,左上为第一个，右下为最后一个
  double offset_x = 0;
  double offset_y = 0;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector(-100.0 + offset_x, 100.0 + offset_y, 0.0));
  positionAlloc->Add (Vector(100.0 + offset_x, 100.0 + offset_y, 0.0));
  positionAlloc->Add (Vector(-100.0 + offset_x, -100.0 + offset_y, 0.0));
  positionAlloc->Add (Vector(100.0 + offset_x, -100.0 + offset_y, 0.0));

//  std::cout << "weizhi:" << positionAlloc << std::endl;
  return positionAlloc;
}

Ptr<ListPositionAllocator>
UEPositions(NodeContainer c, Ptr<ListPositionAllocator> posallo, uint32_t numberOfRandUes)
{
  Ptr<ListPositionAllocator> positionAlloc = posallo;//CreateObject<ListPositionAllocator> ();
  double offset_x=-100, offset_y=-100;

  for (uint16_t i = 0; i < numberOfRandUes; i++)
   {
    Vector pos = RandomUEPosition(100);
    positionAlloc->Add (Vector(pos.x + offset_x, pos.y + offset_y, pos.z));
   }
  positionAlloc->Add (Vector(-20, -100, 0));

//  std::cout << "weizhi:" << positionAlloc << std::endl;
  return positionAlloc;
}


int
main (int argc, char *argv[])
{

  double simTime = 1.1;
  double interPacketInterval = 100;

  uint32_t macroEnbs = 1;
  uint32_t picoEnbs = macroEnbs * 4;
  uint32_t numberOfNodes = picoEnbs;
  uint32_t enbInterDistance = 500;
  uint32_t enbRadius = enbInterDistance/2;
  double enbYDis = std::sqrt(enbInterDistance*enbInterDistance-enbRadius*enbRadius);


  uint32_t numberOfRandUes = 8;
  // Command line arguments
  CommandLine cmd;
  cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
  cmd.AddValue("interPacketInterval", "Inter packet interval [ms])", interPacketInterval);
  cmd.AddValue("numberOfRandUes", "number of random UEs around the third small cell", numberOfRandUes);
  cmd.Parse(argc, argv);

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  lteHelper->SetSchedulerType ("ns3::PfFfMacScheduler");
  Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults();

  // parse again so you can override default values from the command line
  cmd.Parse(argc, argv);

  Ptr<Node> pgw = epcHelper->GetPgwNode ();

   // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  NodeContainer picoNodes;

  enbNodes.Create(macroEnbs);// * enbSectors);
  picoNodes.Create(numberOfNodes);
  ueNodes.Create(numberOfNodes+numberOfRandUes+1);//ue实际比pico多8+1个

  //set mobility of macro cells
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector( 0, 0, 0));

  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator(positionAlloc);
  mobility.Install(enbNodes);

  //Set mobility of small cells and ues
  Ptr<ListPositionAllocator> positionAllocatortem = PicoPositions(picoNodes, enbRadius, enbYDis);
  mobility.SetPositionAllocator (positionAllocatortem);
  mobility.Install (picoNodes);
  mobility.SetPositionAllocator (UEPositions(picoNodes, positionAllocatortem, numberOfRandUes));
  mobility.Install (ueNodes);

  //Install macro cells
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (46));
  // lteHelper->SetEnbAntennaModelType ("ns3::ParabolicAntennaModel");//抛物面天线模型
  // lteHelper->SetEnbAntennaModelAttribute ("Beamwidth", DoubleValue (70));//DoubleValue (70)
  lteHelper->SetEnbDeviceAttribute ("DlEarfcn", UintegerValue (100));
  lteHelper->SetEnbDeviceAttribute ("UlEarfcn", UintegerValue (100+18000));
  lteHelper->SetEnbDeviceAttribute ("DlBandwidth", UintegerValue (50));
  lteHelper->SetEnbDeviceAttribute ("UlBandwidth", UintegerValue (50));
  NetDeviceContainer enbLteDevs = lteHelper->InstallMacroEnbDevices (enbNodes);//lteHexGridEnbTopologyHelper->SetPositionAndInstallEnbDevice (enbNodes);

  //Install small cells
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (30));
  lteHelper->SetEnbAntennaModelType ("ns3::IsotropicAntennaModel");
  lteHelper->SetEnbDeviceAttribute ("DlEarfcn", UintegerValue (200));//原先注释
  lteHelper->SetEnbDeviceAttribute ("UlEarfcn", UintegerValue (200+18000));//原先注释
  lteHelper->SetEnbDeviceAttribute ("DlBandwidth", UintegerValue (50));
  lteHelper->SetEnbDeviceAttribute ("UlBandwidth", UintegerValue (50));
  NetDeviceContainer picoLteDevs = lteHelper->InstallSmallEnbDevices (picoNodes, 4);

  //Install ues
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);

  lteHelper->AddX2Interface (NodeContainer(enbNodes, picoNodes));

  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
  // Assign IP address to UEs, and install applications
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  // Attach one UE per eNodeB
//  for (uint16_t i = 0; i < numberOfNodes; i++)
//      {
//        lteHelper->Attach (ueLteDevs.Get(i), enbLteDevs.Get(i));
//        // side effect: the default EPS bearer will be activated
//      }
  uint32_t ueIndex;
  for (ueIndex=0; ueIndex < ueLteDevs.GetN ()-1; ++ueIndex)
    lteHelper->Attach (ueLteDevs.Get (ueIndex));

  Simulator::Schedule (MilliSeconds (600), &LteHelper::AttachDelay, lteHelper, ueLteDevs.Get (ueIndex));

  // Install and start applications on UEs and remote host
  uint16_t dlPort = 1234;
  uint16_t ulPort = 2000;
  uint16_t otherPort = 3000;
  ApplicationContainer clientApps;
  ApplicationContainer serverApps;
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      ++ulPort;
      ++otherPort;
      PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
      PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
      // PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), otherPort));
      serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get(u)));
      serverApps.Add (ulPacketSinkHelper.Install (remoteHost));
      // serverApps.Add (packetSinkHelper.Install (ueNodes.Get(u)));

      UdpClientHelper dlClient (ueIpIface.GetAddress (u), dlPort);
      dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
      dlClient.SetAttribute ("MaxPackets", UintegerValue(1000000));

      UdpClientHelper ulClient (remoteHostAddr, ulPort);
      ulClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
      ulClient.SetAttribute ("MaxPackets", UintegerValue(1000000));

      // UdpClientHelper client (ueIpIface.GetAddress (u), otherPort);
      // client.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
      // client.SetAttribute ("MaxPackets", UintegerValue(1000000));

      // clientApps.Add (dlClient.Install (remoteHost));
      clientApps.Add (ulClient.Install (ueNodes.Get(u)));
      // if (u+1 < ueNodes.GetN ())
      //   {
      //     clientApps.Add (client.Install (ueNodes.Get(u+1)));
      //   }
      // else
      //   {
      //     clientApps.Add (client.Install (ueNodes.Get(0)));
      //   }
    }
  serverApps.Start (Seconds (0.01));
  clientApps.Start (Seconds (0.01));
  lteHelper->EnableTraces ();
  // Uncomment to enable PCAP tracing
  //p2ph.EnablePcapAll("lena-epc-first");

  Simulator::Stop(Seconds(simTime));
  Simulator::Run();

  /*GtkConfigStore config;
  config.ConfigureAttributes();*/

  Simulator::Destroy();
  return 0;

}

