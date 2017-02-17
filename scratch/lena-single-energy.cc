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
#include "ns3/lte-ue-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include <climits>
#include <algorithm>

using namespace ns3;


NS_LOG_COMPONENT_DEFINE ("SingleEnergy");

Ptr<UniformRandomVariable> randomGen = 0;

Ptr<ListPositionAllocator> GetPosition (int n, double radius, double height, double dist=0, double delta=0);
void Record (std::map<uint16_t, Vector>, std::map<uint16_t, Vector>, std::vector<Vector>, std::map<uint64_t, std::pair<uint16_t, uint16_t> >);

int
main (int argc, char *argv[])
{
  double simTime = 1.1;
  double interPacketInterval = 100;

  uint32_t numberOfMacros = 1;
  uint32_t numberOfSmalls = 7;
  uint32_t numberOfUes = 1;
  uint32_t startTime = 2;
  uint32_t startInterval = 100;
  double rateReq = 100000;

  double radius = 140;
  double smallDist = 10.0;

  double rsprThresh = 78.0;

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
  cmd.AddValue("pktInterval", "Inter packet interval [ms])", interPacketInterval);
  cmd.AddValue("numberOfUes", "total number of UEs", numberOfUes);
  cmd.AddValue("startTime", "start time for sleep algorithm", startTime);
  cmd.AddValue("startInterval", "start interval for ues", startInterval);
  cmd.AddValue("numberOfSmalls", "number of small cells", numberOfSmalls);
  cmd.AddValue("rateRequire", " ", rateReq);
  cmd.AddValue("radius", "", radius);
  cmd.AddValue("smallDist", "", smallDist);
  cmd.Parse(argc, argv);

  // init random
  RngSeedManager::SetSeed (time(NULL));
  randomGen = CreateObject<UniformRandomVariable> ();
  randomGen->SetAttribute ("Min", DoubleValue (-radius));
  randomGen->SetAttribute ("Max", DoubleValue (radius));

  // create helper
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  lteHelper->SetSchedulerType ("ns3::PfFfMacScheduler");
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();

  cmd.Parse (argc, argv);

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

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);


  NodeContainer macroNodes;
  NodeContainer smallNodes;
  NodeContainer ueNodes;

  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  
  // install macro enbs
  Ptr<ListPositionAllocator> macroPosAlloc = Create<ListPositionAllocator> ();
  macroPosAlloc->Add (Vector(0, 0, 25));
  macroNodes.Create (numberOfMacros);
  mobility.SetPositionAllocator(macroPosAlloc);
  mobility.Install(macroNodes);

  Config::SetDefault ("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue(320));
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (46));
  Config::SetDefault ("ns3::SetCoverSleepAlgorithm::StartTime", TimeValue(Seconds(startTime)));
  Config::SetDefault ("ns3::SetCoverSleepAlgorithm::RateRequirement", DoubleValue(rateReq));
  lteHelper->SetEnbDeviceAttribute ("DlEarfcn", UintegerValue (2510));
  lteHelper->SetEnbDeviceAttribute ("UlEarfcn", UintegerValue (2510+18000));
  lteHelper->SetEnbDeviceAttribute ("DlBandwidth", UintegerValue (100));
  lteHelper->SetEnbDeviceAttribute ("UlBandwidth", UintegerValue (100));
  lteHelper->SetSleepAlgorithmType ("ns3::SetCoverSleepAlgorithm");
  NetDeviceContainer enbLteDevs = lteHelper->InstallMacroEnbDevices (macroNodes);

  // install small enbs
  Ptr<ListPositionAllocator> smallPosAlloc = GetPosition (numberOfSmalls, radius, 10.0, smallDist, 10);
  smallNodes.Create (numberOfSmalls);
  mobility.SetPositionAllocator (smallPosAlloc);
  mobility.Install (smallNodes);

  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (30));
  lteHelper->SetEnbAntennaModelType ("ns3::IsotropicAntennaModel");
  lteHelper->SetEnbDeviceAttribute ("DlEarfcn", UintegerValue (3050));
  lteHelper->SetEnbDeviceAttribute ("UlEarfcn", UintegerValue (3050+18000));
  lteHelper->SetEnbDeviceAttribute ("DlBandwidth", UintegerValue (100));
  lteHelper->SetEnbDeviceAttribute ("UlBandwidth", UintegerValue (100));
  lteHelper->SetSleepAlgorithmType ("ns3::NoOpSleepAlgorithm");
  NetDeviceContainer picoLteDevs = lteHelper->InstallSmallEnbDevices (smallNodes, numberOfSmalls);

  // binding macro and smalls
  lteHelper->RegisterSmallCells (enbLteDevs, picoLteDevs);
  lteHelper->AddX2Interface (NodeContainer(macroNodes, smallNodes));

  // install ues
  Ptr<ListPositionAllocator> uePosAlloc = GetPosition(numberOfUes, radius, 1.5);
  ueNodes.Create (numberOfUes);
  mobility.SetPositionAllocator (uePosAlloc);
  mobility.Install (ueNodes);

  Config::SetDefault ("ns3::SetCoverSleepAlgorithm::RsrpThresh", DoubleValue (rsprThresh));
  lteHelper->SetUeDeviceAttribute ("DlEarfcn", UintegerValue (2510));
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);

  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
  {
    Ptr<Node> ueNode = ueNodes.Get (u);
    Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
    ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
  }

  // install apps
  ApplicationContainer clientApps;
  ApplicationContainer serverApps;
  uint16_t dlPort = 1234;
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
  {
      PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
      serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get(u)));

      UdpClientHelper dlClient (ueIpIface.GetAddress (u), dlPort);
      dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
      dlClient.SetAttribute ("MaxPackets", UintegerValue(1000000));
      clientApps.Add (dlClient.Install (remoteHost));
  }

  // attach ues
  for(uint32_t i=0; i < numberOfUes; ++i)
  {
    uint32_t delay = startInterval * i;
    Simulator::Schedule (MilliSeconds (delay), &LteHelper::AttachDelay, lteHelper, ueLteDevs.Get (i));
    Simulator::Schedule (MilliSeconds (delay), &Application::Start, serverApps.Get (i));
    Simulator::Schedule (MilliSeconds (delay), &Application::Start, clientApps.Get (i));
  }

  // stat packets
  Simulator::Schedule (MilliSeconds (numberOfUes * startInterval + 1000), &LteHelper::GetRecvBytes, lteHelper, serverApps); 
  Simulator::Schedule (MilliSeconds (startTime * 1000 - 10), &LteHelper::GetRecvBytes, lteHelper, serverApps); 
  Simulator::Schedule (MilliSeconds (startTime * 1000 + startInterval * 100), &LteHelper::GetRecvBytes, lteHelper, serverApps); 
  Simulator::Schedule (MilliSeconds (simTime * 1000 - 10), &LteHelper::GetRecvBytes, lteHelper, serverApps); 

  Simulator::Stop(Seconds(simTime));
  Simulator::Run();

  std::map<uint16_t, Vector> mcPos, scPos;
  for(uint32_t i=0; i<numberOfMacros; ++i)
  {
    Ptr<LteEnbNetDevice> dev = DynamicCast<LteEnbNetDevice> (enbLteDevs.Get (i));
    uint16_t cellId = dev->GetCellId ();
    Ptr<Node> node = dev->GetNode ();
    mcPos[cellId] = node->GetObject<MobilityModel> ()->GetPosition ();
  }

  for(uint32_t i=0; i<numberOfSmalls; ++i)
  {
    Ptr<LteEnbNetDevice> dev = DynamicCast<LteEnbNetDevice> (picoLteDevs.Get (i));
    uint16_t cellId = dev->GetCellId ();
    Ptr<Node> node = dev->GetNode ();
    scPos[cellId] = node->GetObject<MobilityModel> ()->GetPosition ();
  }

  std::vector<Vector> uePos;
  std::map<uint64_t, std::pair<uint16_t, uint16_t> > conn;
  for(uint32_t i=0; i<numberOfUes; ++i)
  {
    Ptr<Node> node = ueNodes.Get (i);
    Ptr<LteUeRrc> rrc = DynamicCast<LteUeNetDevice> (ueLteDevs.Get (i))->GetRrc ();
    uint64_t imsi = rrc->GetImsi ();
    uint16_t mid = rrc->GetCellId ();
    uint16_t sid = rrc->GetSmallCellId ();
    uePos.push_back (node->GetObject<MobilityModel> ()->GetPosition ());
    conn[imsi] = std::pair<uint16_t, uint16_t> (mid, sid);
  }

  Record (mcPos, scPos, uePos, conn);

  Simulator::Destroy();

  return 0;

}

Ptr<ListPositionAllocator>
GetPosition (int n, double radius, double h, double dist, double delta)
{
  Ptr<ListPositionAllocator> pos = CreateObject<ListPositionAllocator> ();
  double range=radius*radius;
  for(int i=0; i<n; ++i)
  {
    double x=0, y=0;
    while(true) {
      x = randomGen->GetValue ();
      y = randomGen->GetValue ();
      if(x*x+y*y > range-delta*delta)
        continue;
      Vector v(x, y, h);
      int j= dist <= 1e-6 ? i : 0;
      while(j < i)
      {
        if(CalculateDistance (pos->GetNext (), v) < dist )
          break;
        ++j;
      }
      if(j==i)
        break;
    }
    pos->Add (Vector(x, y, h));
  }
  return pos;
}

void
Record (std::map<uint16_t, Vector> mcPos, std::map<uint16_t, Vector> scPos, std::vector<Vector> uePos, std::map<uint64_t, std::pair<uint16_t, uint16_t> > conn)
{
  std::ofstream of("data/pos.dat", std::ofstream::ate);

  of << mcPos.size () << "\n";
  std::map<uint16_t, Vector>::iterator it;
  for(it=mcPos.begin (); it!=mcPos.end (); ++it)
  {
    of << it->first << " " << it->second << "\n";
  }

  of << scPos.size () << "\n";
  for(it=scPos.begin (); it!=scPos.end (); ++it)
  {
    of << it->first << " " << it->second << "\n";
  }

  of << uePos.size () << "\n";
  for(uint32_t i=0; i<uePos.size (); ++i)
  {
    of << i+1 << " " << uePos[i] << "\n";
  }

  of << conn.size () << "\n";
  std::map<uint64_t, std::pair<uint16_t, uint16_t> >::iterator cit;
  for(cit=conn.begin (); cit!=conn.end (); ++cit)
  {
    of << cit->first << " " << cit->second.first << ":" << cit->second.second << "\n";
  }

  of.close ();
}