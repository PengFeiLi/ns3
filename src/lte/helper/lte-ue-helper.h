#ifndef LTE_UE_HELPER
#define LTE_UE_HELPER

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

#include <queue>

namespace ns3 {


class LteUeHelper : public Object
{
    typedef std::map<uint16_t, Ptr<LteEnbNetDevice> > EnbDevMap;

public:
    LteUeHelper ();

    virtual ~LteUeHelper ();

    static TypeId GetTypeId ();

    void SetLteHelper (Ptr<LteHelper> lteHelper);

    void SetEnbMap (EnbDevMap& em);

    void SetRemoteHost (Ptr<Node> rh);

    std::map<uint64_t, std::pair<uint16_t, uint16_t> > GetUeConn ();

    std::vector<Vector> GetUePos ();

protected:
    virtual void DoInitialize (void);

    bool ValidatePos (double x, double y);

    Vector GetRandPos ();

    Ptr<ListPositionAllocator> GetNRandPos(int n);

    void AllocPosition (Ptr<Node> node);

    void Run ();

private:
    Ptr<LteHelper> m_lteHelper;

    double m_macroDist;

    double m_height;

    Ptr<UniformRandomVariable> m_randomX;

    Ptr<UniformRandomVariable> m_randomY;

    uint32_t m_totalUe;

    uint32_t m_uesedUe;

    EnbDevMap m_enbMap;

    std::vector<Vector> m_uePos;

    NodeContainer m_ueNodeContainer;

    NetDeviceContainer m_ueDevContainer;

    Ipv4InterfaceContainer m_ueIpIface;

    ApplicationContainer m_clientApps;

    ApplicationContainer m_serverApps;

    Ptr<Node> m_remoteHost;

    uint16_t m_dlPort;

    uint32_t m_pktInterval;

    uint32_t m_maxPkts;

    uint32_t m_startInterval;
}; // end of LteUeHelper


} // end of namespace ns3


#endif /* LTE_UE_HELPER */