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


NS_LOG_COMPONENT_DEFINE ("LenaSimpleEpcEnergy");

#define SQUARE(x) ((x)*(x));

Ptr<ListPositionAllocator> GetMacroPosition (uint32_t n, double macroDist, double height);
Ptr<ListPositionAllocator> GetSmallPosition (uint32_t &n, double macroDist, double height);
void ClusterEnb (std::vector<ClusterInfo>&, std::vector<EnbInfo>&, std::vector<EnbInfo>&);

void Record (std::map<uint16_t, Vector>, std::map<uint16_t, Vector>, std::vector<Vector>, std::map<uint64_t, std::pair<uint16_t, uint16_t> >);

int
main (int argc, char *argv[])
{
  double simTime = 1.1;
  double interPacketInterval = 100;

  uint32_t numberOfMacros = 7;
  uint32_t numberOfSmalls = 7;
  uint32_t totalUe = 1;
  uint32_t startTime = 2;
  uint32_t startInterval = 100;

  double macroDist = 400;

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
  cmd.AddValue("pktInterval", "Inter packet interval [ms])", interPacketInterval);
  cmd.AddValue("totalUe", "total number of UEs", totalUe);
  cmd.AddValue("startTime", "start time for sleep algorithm", startTime);
  cmd.AddValue("startInterval", "start interval for ues", startInterval);
  cmd.AddValue("numberOfSmalls", "number of small cells", numberOfSmalls);
  cmd.Parse(argc, argv);

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

  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  
  Ptr<ListPositionAllocator> macroPosAlloc = GetMacroPosition (numberOfMacros, macroDist, 25.0);
  macroNodes.Create (numberOfMacros);
  mobility.SetPositionAllocator(macroPosAlloc);
  mobility.Install(macroNodes);

  Ptr<ListPositionAllocator> smallPosAlloc = GetSmallPosition (numberOfSmalls, macroDist, 10.0);
  smallNodes.Create (numberOfSmalls);
  mobility.SetPositionAllocator (smallPosAlloc);
  mobility.Install (smallNodes);

  std::vector<ClusterInfo> cinfo(numberOfMacros, ClusterInfo());
  std::vector<EnbInfo> minfo(numberOfMacros, EnbInfo());
  std::vector<EnbInfo> sinfo(numberOfSmalls, EnbInfo());
  for(uint32_t i=0; i<numberOfMacros; ++i)
  {
    cinfo[i].cid = i+1;
    minfo[i].cid = i+1;
    minfo[i].type = 1;
    minfo[i].pos = macroNodes.Get (i)->GetObject<MobilityModel>() ->GetPosition ();
  }

  for(uint32_t i=0; i<numberOfSmalls; ++i)
  {
    sinfo[i].type = 2;
    sinfo[i].pos = smallNodes.Get (i)->GetObject<MobilityModel>() ->GetPosition ();
  }

  ClusterEnb (cinfo, minfo, sinfo);
  
  Config::SetDefault ("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue(320));
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (46));
  Config::SetDefault ("ns3::SetCoverSleepAlgorithm::StartTime", TimeValue(Seconds(startTime)));
  lteHelper->SetEnbDeviceAttribute ("DlEarfcn", UintegerValue (2510));
  lteHelper->SetEnbDeviceAttribute ("UlEarfcn", UintegerValue (2510+18000));
  lteHelper->SetEnbDeviceAttribute ("DlBandwidth", UintegerValue (100));
  lteHelper->SetEnbDeviceAttribute ("UlBandwidth", UintegerValue (100));
  lteHelper->SetSleepAlgorithmType ("ns3::SetCoverSleepAlgorithm");
  NetDeviceContainer enbLteDevs = lteHelper->InstallMacroEnbDevices (macroNodes);

  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (30));
  lteHelper->SetEnbAntennaModelType ("ns3::IsotropicAntennaModel");
  lteHelper->SetEnbDeviceAttribute ("DlEarfcn", UintegerValue (3050));
  lteHelper->SetEnbDeviceAttribute ("UlEarfcn", UintegerValue (3050+18000));
  lteHelper->SetEnbDeviceAttribute ("DlBandwidth", UintegerValue (100));
  lteHelper->SetEnbDeviceAttribute ("UlBandwidth", UintegerValue (100));
  lteHelper->SetSleepAlgorithmType ("ns3::NoOpSleepAlgorithm");
  NetDeviceContainer picoLteDevs = lteHelper->InstallSmallEnbDevicesCluster (smallNodes, cinfo, sinfo, 100);

  std::map<uint16_t, Ptr<LteEnbNetDevice> > enbMap;
  NetDeviceContainer enbs(enbLteDevs, picoLteDevs);
  for(uint32_t i=0; i<enbs.GetN (); ++i) {
    Ptr<LteEnbNetDevice> dev = DynamicCast<LteEnbNetDevice>(enbs.Get (i));
    enbMap[dev->GetCellId ()] = dev;
  }

  Config::SetDefault ("ns3::LteUeHelper::TotalUe", UintegerValue(totalUe));
  Config::SetDefault ("ns3::LteUeHelper::StartInterval", UintegerValue(startInterval));
  Config::SetDefault ("ns3::LteUeHelper::PacketInterval", UintegerValue(interPacketInterval));
  lteHelper->SetUeDeviceAttribute ("DlEarfcn", UintegerValue (2510));
  Ptr<LteUeHelper> lteUeHelper = CreateObject<LteUeHelper> ();
  lteUeHelper->SetLteHelper (lteHelper);
  lteUeHelper->SetEnbMap (enbMap);
  lteUeHelper->SetRemoteHost (remoteHost);
  lteUeHelper->Initialize ();

  lteHelper->RegisterSmallCells (enbLteDevs, picoLteDevs);
  lteHelper->AddX2Interface (NodeContainer(macroNodes, smallNodes));

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

  Record (mcPos, scPos, lteUeHelper->GetUePos (), lteUeHelper->GetUeConn ());

  Simulator::Destroy();

  return 0;

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
  for(uint32_t i=0; i<=uePos.size (); ++i)
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

Ptr<ListPositionAllocator>
GetMacroPosition (uint32_t n, double macroDist, double height)
{
  double yDist = std::sqrt (3) / 2 * macroDist;
  double offset_x,offset_y;  
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint16_t i = 0; i < n; i++)
    {
         switch(i)
          {
          case 0:
            offset_x = -macroDist / 2;
            offset_y = -yDist;
            break;
          case 1:
            offset_x = macroDist / 2;
            offset_y = -yDist;
            break;
          case 2:
            offset_x = -macroDist;
            offset_y = 0;
            break;
          case 3:
            offset_x = 0;
            offset_y = 0;
            break;
          case 4:
            offset_x = macroDist;
            offset_y = 0;
            break;
          case 5:
            offset_x = -macroDist / 2;
            offset_y = yDist;
            break;
          case 6:
            offset_x = macroDist / 2;
            offset_y = yDist;
            break;
          }
          positionAlloc->Add (Vector( offset_x,  offset_y, height));
    }

    return positionAlloc;
}

bool CheckPoint(double val_x, double val_y, double macroDist){
  int8_t blockIdx;
  double block_x, block_bias;
  double h = macroDist / 2 / std::sqrt (3);

  if(val_x<-400){
    blockIdx = 1;
    block_x = val_x+400;
    block_bias = h;
  }else if(val_x<-200){
    blockIdx = 1;
    block_x = val_x+200;
    block_bias = h*4;
  }else if(val_x<0){
    blockIdx = -1;
    block_x = val_x;
    block_bias = h*4;
  }
  else if(val_x<200){
    blockIdx = 1;
    block_x = val_x-200;
    block_bias = h*4;
  }
  else if(val_x<400){
    blockIdx = -1;
    block_x = val_x-400;
    block_bias = h*4;
  }
  else{
    blockIdx = -1;
    block_x = val_x-600;
    block_bias = h;
  }

  double temp_y = blockIdx*std::sqrt(3)/3*block_x+(blockIdx+1)/2*200/std::sqrt(3)+block_bias;
  if(val_y>temp_y || val_y<-temp_y){
    return false;
  }

  return true;
}

Vector
RandSmallPos(double macroDist, double height)
{
  double radius_d = macroDist / 2;
  Ptr<UniformRandomVariable> randomVar = CreateObject<UniformRandomVariable> ();
  randomVar->SetAttribute ("Min", DoubleValue (-radius_d*3));//避免热点范围超出宏站范围DoubleValue (-radius_d + 40)
  randomVar->SetAttribute ("Max", DoubleValue (radius_d*3) );//避免热点范围超出宏站范围DoubleValue (radius_d - 40)
  double val_x = randomVar->GetValue();
  double val_temp = radius_d*5/std::sqrt(3);//避免热点范围超出宏站sqrt((enbRadius-40)*(enbRadius-40)-val_x*val_x)
  randomVar->SetAttribute ("Min", DoubleValue (-val_temp));
  randomVar->SetAttribute ("Max", DoubleValue (val_temp));
  double val_y = randomVar->GetValue();
  Vector pos(val_x, val_y, height);

  return pos;
}

Ptr<ListPositionAllocator>
GetSmallPosition(uint32_t &n, double macroDist, double height)
{
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  uint16_t count = 0;
  for (uint16_t i = 0; i <n; i++)
  {
    Vector pos;
    pos = RandSmallPos (macroDist, height);
    if(CheckPoint (pos.x, pos.y, macroDist)){
      positionAlloc->Add (pos);
      if(n<100)
        NS_LOG_INFO (pos.x << "\t" << pos.y);
      count ++;
    }
  }
  n = count;
  return positionAlloc;
}

void
ClusterEnb (std::vector<ClusterInfo>& cinfo, std::vector<EnbInfo>& minfo, std::vector<EnbInfo>& sinfo)
{
  uint32_t macroEnbs = minfo.size ();
  uint32_t picoEnbs = sinfo.size ();
  for (uint32_t i = 0; i < macroEnbs; i++)
  {
    for (uint32_t j = 0; j < picoEnbs; j++)
    {
      if(sinfo[j].cid == 0)
      {
        double dist = CalculateDistance (minfo[i].pos, sinfo[j].pos);//SQUARE(minfo[i].pos.x-sinfo[j].pos.x) + SQUARE(minfo[i].pos.y-sinfo[j].pos.y);
        if(dist <= 200)
         {
          uint32_t cid = minfo[i].cid;
          sinfo[j].cid = cid;
          cinfo[cid-1].num += 1;
         }
      }
    }
  }

    for(uint32_t j=0; j < picoEnbs; j++)
    {
      if(sinfo[j].cid == 0)
      {    
        uint32_t idx1=-1, idx2=-1;
        double v1, v2;
        v1 = v2 =std::numeric_limits<double>::max ();
        for(uint32_t i = 0; i < macroEnbs; i++)
        {
          double dist = CalculateDistance (minfo[i].pos, sinfo[j].pos);
          if (dist<v1)
          {
            v2 = v1;
            v1 = dist;
            idx2 = idx1;
            idx1 = i;
          } else if (dist<v2) {
            v2 = dist;
            idx2 = i;
          }
        }
        uint32_t cid = minfo[0].cid;
        if(macroEnbs >= 2)
        {
          if((v2 - v1)<=20 && cinfo[minfo[idx2].cid-1].num<cinfo[minfo[idx1].cid-1].num)
          {
            cid = minfo[idx2].cid;
          }
          else
          {
            cid = minfo[idx1].cid;
          }
        }
        sinfo[j].cid = cid;
        cinfo[cid-1].num += 1;
      }
    }
}