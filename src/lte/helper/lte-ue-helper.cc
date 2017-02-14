#include "lte-ue-helper.h"
#include <ns3/log.h>
#include <ns3/double.h>


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LteUeHelper");

NS_OBJECT_ENSURE_REGISTERED (LteUeHelper);


LteUeHelper::LteUeHelper ()
  : m_uesedUe (0)
{
}


LteUeHelper::~LteUeHelper ()
{
}


TypeId
LteUeHelper::GetTypeId ()
{
    static TypeId tid = TypeId ("ns3::LteUeHelper")
        .SetParent<Object> ()
        .AddConstructor<LteHelper> ()
        .AddAttribute ("TotalUe", "total number of ues",
                       UintegerValue (1500),
                       MakeUintegerAccessor (&LteUeHelper::m_totalUe),
                       MakeUintegerChecker<uint32_t> ())
        .AddAttribute ("MacroDistance", "Distance between macro cells",
                       DoubleValue (400.0),
                       MakeDoubleAccessor (&LteUeHelper::m_macroDist),
                       MakeDoubleChecker<double> ())
        .AddAttribute ("UeHeight", "Height of UEs",
                       DoubleValue (1.5),
                       MakeDoubleAccessor (&LteUeHelper::m_height),
                       MakeDoubleChecker<double> ())
        .AddAttribute ("PacketInterval", "Interval between packets (ms)",
                       UintegerValue (100),
                       MakeUintegerAccessor (&LteUeHelper::m_pktInterval),
                       MakeUintegerChecker<uint32_t> ())
        .AddAttribute ("MaxPackets", "Max number of packets to send",
                       UintegerValue (1000000),
                       MakeUintegerAccessor (&LteUeHelper::m_maxPkts),
                       MakeUintegerChecker<uint32_t> ())
        .AddAttribute ("DownlinkClientPort", "Downlink client udp port",
                       UintegerValue (1234),
                       MakeUintegerAccessor (&LteUeHelper::m_dlPort),
                       MakeUintegerChecker<uint16_t> ())
        .AddAttribute ("StartInterval", "Interval between two ues to start ",
                       UintegerValue (100),
                       MakeUintegerAccessor (&LteUeHelper::m_startInterval),
                       MakeUintegerChecker<uint32_t> ())
        ;

  return tid;
}

void
LteUeHelper::DoInitialize ()
{
  NS_LOG_FUNCTION (this);

  // position random
  double radius_d = m_macroDist / 2;

  m_randomX = CreateObject<UniformRandomVariable> ();
  m_randomX->SetAttribute ("Min", DoubleValue (-radius_d*3));
  m_randomX->SetAttribute ("Max", DoubleValue (radius_d*3) );

  m_randomY = CreateObject<UniformRandomVariable> ();
  m_randomY->SetAttribute ("Min", DoubleValue (-radius_d*5/std::sqrt(3)));
  m_randomY->SetAttribute ("Max", DoubleValue (radius_d*5/std::sqrt(3)));

  // init ues
  m_ueNodeContainer.Create (m_totalUe);
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator (GetNRandPos (m_totalUe));
  mobility.Install (m_ueNodeContainer);
  m_ueDevContainer = m_lteHelper->InstallUeDevice (m_ueNodeContainer);

  // internet
  InternetStackHelper internet;
  internet.Install (m_ueNodeContainer);
  Ptr<PointToPointEpcHelper> epcHelper = DynamicCast<PointToPointEpcHelper>(m_lteHelper->GetEpcHelper ());
  m_ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (m_ueDevContainer));

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  for (uint32_t u = 0; u < m_totalUe; ++u)
  {
    Ptr<Node> ueNode = m_ueNodeContainer.Get (u);
    Ptr<LteUeNetDevice> dev = DynamicCast<LteUeNetDevice>(m_ueDevContainer.Get (u));
    // Set the default gateway for the UE
    Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
    ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
  }

  for (uint32_t u = 0; u < m_totalUe; ++u)
  {
      PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), m_dlPort));
      m_serverApps.Add (dlPacketSinkHelper.Install (m_ueNodeContainer.Get(u)));

      UdpClientHelper dlClient (m_ueIpIface.GetAddress (u), m_dlPort);
      dlClient.SetAttribute ("Interval", TimeValue (MilliSeconds(m_pktInterval)));
      dlClient.SetAttribute ("MaxPackets", UintegerValue(m_maxPkts));

      m_clientApps.Add (dlClient.Install (m_remoteHost));
  }

  Simulator::Schedule (MilliSeconds (1000), &LteUeHelper::Run, this);
}


void
LteUeHelper::SetLteHelper (Ptr<LteHelper> lteHelper)
{
  NS_LOG_FUNCTION (this);
  m_lteHelper = lteHelper;
}

void
LteUeHelper::SetEnbMap (EnbDevMap& em)
{
  m_enbMap.insert (em.begin(), em.end());
}

void
LteUeHelper::SetRemoteHost (Ptr<Node> rh) 
{
  NS_LOG_FUNCTION (this);
  m_remoteHost = rh;
}

void
LteUeHelper::Run ()
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG (m_uesedUe<m_totalUe, "therer is not enough ues to run");

  m_lteHelper->Attach (m_ueDevContainer.Get (m_uesedUe));
  m_serverApps.Get (m_uesedUe)->Start ();
  m_clientApps.Get (m_uesedUe)->Start ();

  if(++m_uesedUe < m_totalUe)
    Simulator::Schedule (MilliSeconds (m_startInterval), &LteUeHelper::Run, this);
}

Ptr<ListPositionAllocator>
LteUeHelper::GetNRandPos(int n)
{
  m_uePos.clear ();
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  for(int i=0; i<n; ++i) {
    Vector pos = GetRandPos ();
    positionAlloc->Add (pos);
    m_uePos.push_back (pos);
  }
  return positionAlloc;
}

Vector
LteUeHelper::GetRandPos ()
{
  NS_LOG_FUNCTION (this);

  double val_x, val_y;
  do {
    val_x = m_randomX->GetValue();
    val_y = m_randomY->GetValue();
  } while (!ValidatePos (val_x, val_y));

  return Vector(val_x, val_y, m_height);
}

std::vector<Vector>
LteUeHelper::GetUePos ()
{
  return m_uePos;
}


bool
LteUeHelper::ValidatePos (double val_x, double val_y)
{
  NS_LOG_FUNCTION (this);

  int8_t blockIdx;
  double block_x, block_bias;
  double h = m_macroDist / 2 / std::sqrt (3);

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


void
LteUeHelper::AllocPosition (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this);

  Ptr<Object> object = node;
  Ptr<MobilityModel> model = object->GetObject<MobilityModel> ();
  if (model == 0)
    {
      model = CreateObject<ConstantPositionMobilityModel> ();
      object->AggregateObject (model);
    }
  model->SetPosition (GetRandPos ());
}

std::map<uint64_t, std::pair<uint16_t, uint16_t> >
LteUeHelper::GetUeConn ()
{
  std::map<uint64_t, std::pair<uint16_t, uint16_t> > conn;
  for(uint32_t i=0; i<m_totalUe; ++i)
  {
    Ptr<LteUeRrc> rrc = DynamicCast<LteUeNetDevice> (m_ueDevContainer.Get (i))->GetRrc ();
    uint64_t imsi = rrc->GetImsi ();
    uint16_t mid = rrc->GetCellId ();
    uint16_t sid = rrc->GetSmallCellId ();
    conn[imsi] = std::pair<uint16_t, uint16_t> (mid, sid);
  }
  return conn;
}

} // end of namespace ns3