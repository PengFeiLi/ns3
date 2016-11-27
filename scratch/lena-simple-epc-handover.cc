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

NS_LOG_COMPONENT_DEFINE ("LenaSimpleEpcHandover");

void
NotifyConnectionEstablishedUe (std::string context,
                               uint64_t imsi,
                               uint16_t cellid,
                               uint16_t rnti)
{
  std::cout << Simulator::Now ().GetSeconds () << " " << context
            << " UE IMSI " << imsi
            << ": connected to CellId " << cellid
            << " with RNTI " << rnti
            << std::endl;
}

void
NotifyHandoverStartUe (std::string context,
                       uint64_t imsi,
                       uint16_t cellid,
                       uint16_t rnti,
                       uint16_t targetCellId)
{
  std::cout << Simulator::Now ().GetSeconds () << " " << context
            << " UE IMSI " << imsi
            << ": previously connected to CellId " << cellid
            << " with RNTI " << rnti
            << ", doing handover to CellId " << targetCellId
            << std::endl;
}

void
NotifyHandoverEndOkUe (std::string context,
                       uint64_t imsi,
                       uint16_t cellid,
                       uint16_t rnti)
{
  std::cout << Simulator::Now ().GetSeconds () << " " << context
            << " UE IMSI " << imsi
            << ": successful handover to CellId " << cellid
            << " with RNTI " << rnti
            << std::endl;
}

void
NotifyConnectionEstablishedEnb (std::string context,
                                uint64_t imsi,
                                uint16_t cellid,
                                uint16_t rnti)
{
  std::cout << Simulator::Now ().GetSeconds () << " " << context
            << " eNB CellId " << cellid
            << ": successful connection of UE with IMSI " << imsi
            << " RNTI " << rnti
            << std::endl;
}

void
NotifyHandoverStartEnb (std::string context,
                        uint64_t imsi,
                        uint16_t cellid,
                        uint16_t rnti,
                        uint16_t targetCellId)
{
  std::cout << Simulator::Now ().GetSeconds () << " " << context
            << " eNB CellId " << cellid
            << ": start handover of UE with IMSI " << imsi
            << " RNTI " << rnti
            << " to CellId " << targetCellId
            << std::endl;
}

void
NotifyHandoverEndOkEnb (std::string context,
                        uint64_t imsi,
                        uint16_t cellid,
                        uint16_t rnti)
{
  std::cout << Simulator::Now ().GetSeconds () << " " << context
            << " eNB CellId " << cellid
            << ": completed handover of UE with IMSI " << imsi
            << " RNTI " << rnti
            << std::endl;
}


int
main (int argc, char *argv[])
{

  double simTime = 2.0;
  double interPacketInterval = 100;
  bool uplink = true;
  bool downlink = true;

  uint32_t macroEnbs = 1;
  uint32_t picoEnbs = macroEnbs * 4;

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
  cmd.AddValue("interPacketInterval", "Inter packet interval [ms])", interPacketInterval);
  cmd.AddValue("uplink", "whether enable uplink data transmission", uplink);
  cmd.AddValue("downlink", "whether enable downlink data transmission", downlink);
  cmd.Parse(argc, argv);

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  lteHelper->SetSchedulerType ("ns3::PfFfMacScheduler");
  lteHelper->SetSleepAlgorithmType ("ns3::NoOpSleepAlgorithm");
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

  enbNodes.Create(macroEnbs);
  picoNodes.Create(picoEnbs);
  ueNodes.Create(1);

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
  uePositionAlloc->Add (Vector(10 ,10, 0));
  mobility.SetPositionAllocator (uePositionAlloc);
  mobility.Install (ueNodes);

  //Install macro cells
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (46));
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

  lteHelper->HandoverRequest (Seconds (0.800), ueLteDevs.Get (0), enbLteDevs.Get (0), picoLteDevs.Get (1));

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

  lteHelper->Attach (ueLteDevs);

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

      if(downlink)
        clientApps.Add (dlClient.Install (remoteHost));

      if(uplink)
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

   // connect custom trace sinks for RRC connection establishment and handover notification
  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbRrc/ConnectionEstablished",
                   MakeCallback (&NotifyConnectionEstablishedEnb));
  Config::Connect ("/NodeList/*/DeviceList/*/LteUeRrc/ConnectionEstablished",
                   MakeCallback (&NotifyConnectionEstablishedUe));
  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverStart",
                   MakeCallback (&NotifyHandoverStartEnb));
  Config::Connect ("/NodeList/*/DeviceList/*/LteUeRrc/HandoverStart",
                   MakeCallback (&NotifyHandoverStartUe));
  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverEndOk",
                   MakeCallback (&NotifyHandoverEndOkEnb));
  Config::Connect ("/NodeList/*/DeviceList/*/LteUeRrc/HandoverEndOk",
                   MakeCallback (&NotifyHandoverEndOkUe));

  Simulator::Stop(Seconds(simTime));
  Simulator::Run();

  /*GtkConfigStore config;
  config.ConfigureAttributes();*/

  Simulator::Destroy();
  return 0;

}

