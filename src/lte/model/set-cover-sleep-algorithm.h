/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Budiarto Herman
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
 * Author: Li Peng Fei <lpengfei2016@gmail.com>
 *
 */

#ifndef SET_COVER_SLEEP_ALGORITHM_H
#define SET_COVER_SLEEP_ALGORITHM_H

#include "lte-sleep-algorithm.h"
#include "lte-sleep-management-sap.h"
#include "lte-rrc-sap.h"

#include <set>
#include <map>

namespace ns3 {

/*
 * \brief implementation of "Green Mobile Access Network with Dynamic Base Station Energy Saving"
 */

class SetCoverSleepAlgorithm : public LteSleepAlgorithm
{

    friend class MemberLteSleepManagementSapProvider<SetCoverSleepAlgorithm>;

public:
    SetCoverSleepAlgorithm ();
    virtual ~ SetCoverSleepAlgorithm ();

    static TypeId GetTypeId ();

    virtual void SetLteSleepManagementSapUser (LteSleepManagementSapUser* s);

    virtual LteSleepManagementSapProvider* GetLteSleepManagementSapProvider ();

protected:
    virtual void DoInitialize ();

    virtual void DoDispose ();

    virtual void DoReportUeMeas (uint16_t rnti, LteRrcSap::MeasResults measResults);

    virtual void DoReportDlCqi (uint16_t cellId, EpcX2Sap::DlCqiParams dlCqiparams);

private:

    void ConvertCqiToSpectralEfficiency ();

    void CalculateSpectralEfficiencyAndCoverage ();

    void Run ();

    void PrintDetails ();

    void PrintSleepPolicy (LteSleepManagementSap::SleepPolicy& sleepPolicy);

    void Reset ();

    void CurrentStatus ();

    void NewStatus (std::map<uint16_t, uint16_t>& newConnection);

    void Record ();

private:

    struct BwRnti
    {
        double bw;
        uint16_t rnti;
    };

    struct BwRntiCompare
    {
    public:
        BwRntiCompare(const std::map<uint16_t, uint16_t>& connections, uint16_t cellId);

        bool operator() (const BwRnti& br1, const BwRnti& br2);

    private:
        const std::map<uint16_t, uint16_t>& m_connections;
        uint16_t m_cellId;
    };

    struct Connection 
    {
        uint64_t imsi;
        uint16_t macro;
        uint16_t oldsmall;
        uint16_t newsmall;
    };

    LteSleepManagementSapProvider* m_sleepManagementSapProvider;

    LteSleepManagementSapUser* m_sleepManagementSapUser;

    Time m_startTime;

    Time m_runPeriod;

    uint8_t m_threshold;

    uint8_t m_measId;

    // small cells managed by this macro cell
    std::set<uint16_t> m_cells;

    std::set<uint16_t> m_smallCells;

    // current served ues
    std::set<uint16_t> m_ues;

    // connection relationship
    std::map<uint16_t, uint16_t> m_ueToCell;

    std::map<uint16_t, uint64_t> m_idMap;

    // RSRP measurements
    std::map<uint16_t, LteRrcSap::MeasResults> m_rsrpMap;

    // CQI measurements
    std::map<uint16_t, EpcX2Sap::DlCqiParams> m_cqiMap;

    // key : small cellId
    // value　: ue set that can be covered by the small cell
    std::map<uint16_t, std::set<uint16_t> > m_ubMap;

    // <ue, cell> ---> spectrum efficiency
    std::map<uint32_t, double> m_seMap;

    std::map<uint16_t, Connection> m_conn;

    // Pb = 1
    double m_pb;

    // maximum bandwidth for all small cells
    double m_maxW;

    // rate requirement for all ues
    double m_rho;

    // protection margin
    double m_alpha;

    // small cell can cover the ue when RSRP is no less than this value
    // unit dbm
    double m_rsrpThresh;

    // maximum spectral efficiency of this lte
    double m_maxSe;

    // noisePower
    double m_noisePower;

}; // end of SetCoverSleepAlgorithm


} // end of namespace ns3


#endif /* SET_COVER_SLEEP_ALGORITHM_H */