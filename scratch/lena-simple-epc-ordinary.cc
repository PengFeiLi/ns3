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


int
main (int argc, char *argv[])
{

  double simTime = 1.1;
  double interPacketInterval = 100;
  bool uplink = true;
  bool downlink = true;

  uint32_t macroEnbs = 1;
  uint32_t picoEnbs = macroEnbs * 4;
  uint32_t numberOfNodes = picoEnbs;

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
  cmd.AddValue("interPacketInterval", "Inter packet interval [ms])", interPacketInterval);
  cmd.AddValue("uplink", "whether enable uplink data transmission", uplink);
  cmd.AddValue("downlink", "whether enable downlink data transmission", downlink);
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
  ueNodes.Create(numberOfNodes);

  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

  //set mobility of macro cells
  Ptr<ListPositionAllocator> macroPositionAlloc = CreateObject<ListPositionAllocator> ();
  macroPositionAlloc->Add (Vector( 0, 0, 0));
  mobility.SetPositionAllocator(macroPositionAlloc);
  mobility.Install(enbNodes);

  //Set mobility of small cells
  Ptr<ListPositionAllocator> smallPositionAlloc = CreateObject<ListPositionAllocator> ();
  smallPositionAlloc->Add (Vector(125, 125, 0));
  smallPositionAlloc->Add (Vector(-125, 125, 0));
  smallPositionAlloc->Add (Vector(-125, -125, 0));
  smallPositionAlloc->Add (Vector(125, -125, 0));
  mobility.SetPositionAllocator (smallPositionAlloc);
  mobility.Install (picoNodes);

  // set mobility of ues
  Ptr<ListPositionAllocator> uePositionAlloc = CreateObject<ListPositionAllocator> ();
  uePositionAlloc->Add (Vector(125, 0, 0));
  uePositionAlloc->Add (Vector(0, -125, 0));
  uePositionAlloc->Add (Vector(-125, 0, 0));
  uePositionAlloc->Add (Vector(0, 125, 0));
  // uePositionAlloc->Add (Vector(125, 125, 0));
  // uePositionAlloc->Add (Vector(-125, 125, 0));
  // uePositionAlloc->Add (Vector(-125, -125, 0));
  // uePositionAlloc->Add (Vector(125, -125, 0));
  mobility.SetPositionAllocator (uePositionAlloc);
  mobility.Install (ueNodes);

  //Install macro cells
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (46));
  Config::SetDefault ("ns3::SetCoverSleepAlgorithm::RunPeriod", TimeValue (Seconds (2)));
  // lteHelper->SetEnbAntennaModelType ("ns3::ParabolicAntennaModel");//抛物面天线模型
  // lteHelper->SetEnbAntennaModelAttribute ("Beamwidth", DoubleValue (70));//DoubleValue (70)
  lteHelper->SetEnbDeviceAttribute ("DlEarfcn", UintegerValue (100));
  lteHelper->SetEnbDeviceAttribute ("UlEarfcn", UintegerValue (100+18000));
  lteHelper->SetEnbDeviceAttribute ("DlBandwidth", UintegerValue (50));
  lteHelper->SetEnbDeviceAttribute ("UlBandwidth", UintegerValue (50));
  lteHelper->SetSleepAlgorithmType ("ns3::SetCoverSleepAlgorithm");
  NetDeviceContainer enbLteDevs = lteHelper->InstallMacroEnbDevices (enbNodes);//lteHexGridEnbTopologyHelper->SetPositionAndInstallEnbDevice (enbNodes);

  //Install small cells
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (30));
  lteHelper->SetEnbAntennaModelType ("ns3::IsotropicAntennaModel");
  lteHelper->SetEnbDeviceAttribute ("DlEarfcn", UintegerValue (200));//原先注释
  lteHelper->SetEnbDeviceAttribute ("UlEarfcn", UintegerValue (200+18000));//原先注释
  lteHelper->SetEnbDeviceAttribute ("DlBandwidth", UintegerValue (50));
  lteHelper->SetEnbDeviceAttribute ("UlBandwidth", UintegerValue (50));
  lteHelper->SetSleepAlgorithmType ("ns3::NoOpSleepAlgorithm");
  NetDeviceContainer picoLteDevs = lteHelper->InstallSmallEnbDevices (picoNodes, 4);

  //Install ues
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);
  lteHelper->RegisterSmallCells (enbLteDevs, picoLteDevs);
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
  for (ueIndex=0; ueIndex < ueLteDevs.GetN (); ++ueIndex)
    lteHelper->Attach (ueLteDevs.Get (ueIndex));

  // Install and start applications on UEs and remote host
  uint16_t dlPort = 1234;
  uint16_t ulPort = 2000;
  ApplicationContainer clientApps;
  ApplicationContainer serverApps;
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      ++ulPort;
      PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
      PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));

      serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get(u)));
      serverApps.Add (ulPacketSinkHelper.Install (remoteHost));

      UdpClientHelper dlClient (ueIpIface.GetAddress (u), dlPort);
      dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
      dlClient.SetAttribute ("MaxPackets", UintegerValue(1000000));

      UdpClientHelper ulClient (remoteHostAddr, ulPort);
      ulClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
      ulClient.SetAttribute ("MaxPackets", UintegerValue(1000000));

       if(downlink)
        clientApps.Add (dlClient.Install (remoteHost));

      if(uplink)
        clientApps.Add (ulClient.Install (ueNodes.Get(u)));
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

