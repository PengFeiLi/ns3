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

#include "set-cover-sleep-algorithm.h"
#include <ns3/simulator.h>
#include <ns3/lte-common.h>
#include <ns3/math.h>
#include <ns3/log.h>
#include <ns3/double.h>

#include <queue>
#include <vector>
#include <algorithm>
#include <iterator>
#include <sstream>
#include <fstream>


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SetCoverSleepAlgorithm");

NS_OBJECT_ENSURE_REGISTERED (SetCoverSleepAlgorithm);


#define DBM2W(dbm) (pow(10.0, dbm/10.0)/1000.0)


static const double SpectralEfficiencyForCqi[16] = {
  0.0, // out of range
  0.15, 0.23, 0.38, 0.6, 0.88, 1.18,
  1.48, 1.91, 2.41,
  2.73, 3.32, 3.9, 4.52, 5.12, 5.55
};

static bool isBelongTo (uint16_t smallCellId, uint16_t macroCellId)
{
    return ((smallCellId & 0x003F) && (smallCellId & 0xFFC0) == macroCellId);
}


SetCoverSleepAlgorithm::BwRntiCompare::BwRntiCompare (const std::map<uint16_t, uint16_t>& connections, uint16_t cellId)
    : m_connections (connections),
      m_cellId (cellId)
{
}

bool
SetCoverSleepAlgorithm::BwRntiCompare::operator() (const BwRnti& br1, const BwRnti& br2)
{
    if (br1.bw != br2.bw)
      return br1.bw > br2.bw;

    bool hasbr1 = m_connections.count (br1.rnti);
    bool hasbr2 = m_connections.count (br2.rnti);

    if ((hasbr1 && hasbr2) || (!hasbr1&&!hasbr2))
        return br1.rnti > br2.rnti;

    return hasbr2;
}

SetCoverSleepAlgorithm::SetCoverSleepAlgorithm ()
    : m_sleepManagementSapUser (0),
      m_startTime (Seconds (1)),
      m_runPeriod (Seconds (1800)),
      m_threshold (0),
      m_measId (0),
      m_pb (1.0),
      m_maxW (20000000.0),
      m_rho (100000.0),
      m_alpha (0.1),
      m_rsrpThresh (-120.0),
      m_maxSe (5.55),
      m_noisePower (0.0)
{
    NS_LOG_FUNCTION (this);
    m_sleepManagementSapProvider = new MemberLteSleepManagementSapProvider<SetCoverSleepAlgorithm> (this);
}


SetCoverSleepAlgorithm::~SetCoverSleepAlgorithm ()
{
    NS_LOG_FUNCTION (this);
}


void
SetCoverSleepAlgorithm::DoDispose ()
{
    NS_LOG_FUNCTION (this);
    delete m_sleepManagementSapProvider;
}


TypeId
SetCoverSleepAlgorithm::GetTypeId ()
{
    static TypeId tid = TypeId ("ns3::SetCoverSleepAlgorithm")
        .SetParent<LteSleepAlgorithm> ()
        .SetGroupName ("Lte")
        .AddConstructor<SetCoverSleepAlgorithm> ()
        .AddAttribute ("StartTime", "Time to start(s)",
                       TimeValue (Seconds (1)),
                       MakeTimeAccessor (&SetCoverSleepAlgorithm::m_startTime),
                       MakeTimeChecker ())
        .AddAttribute ("RunPeriod", "Period to start(s)",
                       TimeValue (Seconds (1800)),
                       MakeTimeAccessor (&SetCoverSleepAlgorithm::m_runPeriod),
                       MakeTimeChecker ())
        .AddAttribute ("NoisePower", "Power of noise(W)",
                       DoubleValue (0.0),
                       MakeDoubleAccessor (&SetCoverSleepAlgorithm::m_noisePower),
                       MakeDoubleChecker<double> ())
        .AddAttribute ("RateRequirement", "Rate requirement for users(bps)",
                       DoubleValue (100000.0),
                       MakeDoubleAccessor (&SetCoverSleepAlgorithm::m_rho),
                       MakeDoubleChecker<double> ())
    ;
    return tid;
}


void
SetCoverSleepAlgorithm::SetLteSleepManagementSapUser (LteSleepManagementSapUser* s)
{
    NS_LOG_FUNCTION (this << s);
    m_sleepManagementSapUser = s;
}


LteSleepManagementSapProvider*
SetCoverSleepAlgorithm::GetLteSleepManagementSapProvider ()
{
    NS_LOG_FUNCTION (this);
    return m_sleepManagementSapProvider;
}


void
SetCoverSleepAlgorithm::DoInitialize ()
{
    NS_LOG_FUNCTION (this);
    LteSleepAlgorithm::DoInitialize ();

    NS_LOG_LOGIC (this << " requesting Event A4 measurements"
                     << " (threshold=" << (uint16_t) m_threshold << ")");
    LteRrcSap::ReportConfigEutra reportConfig;
    reportConfig.eventId = LteRrcSap::ReportConfigEutra::EVENT_A4;
    reportConfig.threshold1.choice = LteRrcSap::ThresholdEutra::THRESHOLD_RSRP;
    reportConfig.threshold1.range = m_threshold;
    reportConfig.triggerQuantity = LteRrcSap::ReportConfigEutra::RSRP;
    reportConfig.reportInterval = LteRrcSap::ReportConfigEutra::MS480;
    m_measId = m_sleepManagementSapUser->AddUeMeasReportConfigForSleep (reportConfig);

    Simulator::Schedule ( m_startTime, &SetCoverSleepAlgorithm::Run, this);
}

void
SetCoverSleepAlgorithm::DoReportUeMeas (uint16_t rnti, LteRrcSap::MeasResults measResults)
{
    NS_LOG_FUNCTION (this);
    m_rsrpMap[rnti] = measResults;
}

void
SetCoverSleepAlgorithm::DoReportDlCqi (uint16_t cellId, EpcX2Sap::DlCqiParams dlCqiparams)
{
    NS_LOG_FUNCTION (this);
    m_cqiMap[cellId] = dlCqiparams;
}

void
SetCoverSleepAlgorithm::ConvertCqiToSpectralEfficiency ()
{
    NS_LOG_FUNCTION(this);

    std::map<uint16_t, EpcX2Sap::DlCqiParams>::iterator it = m_cqiMap.begin ();
    while (it != m_cqiMap.end ())
    {
        uint16_t cellId = it->first;
        std::vector<uint16_t> rntis = it->second.rntis;
        std::vector<uint8_t> cqis = it->second.cqis;
        for (unsigned i=0; i<rntis.size (); ++i)
        {
            m_seMap[(rntis[i]<<16)|cellId] = SpectralEfficiencyForCqi[cqis[i]];
        }
        ++it;
    }
}

void
SetCoverSleepAlgorithm::CalculateSpectralEfficiencyAndCoverage ()
{
    NS_LOG_FUNCTION (this);
    std::map<uint16_t, uint16_t>::iterator it = m_ueToCell.begin ();
    while (it != m_ueToCell.end ())
    {
        uint16_t rnti = it->first;
        uint16_t pCellId = it->second;
        m_ues.insert (rnti);
        std::list<LteRrcSap::MeasResultEutra> measResultListEutra = m_rsrpMap[rnti].measResultListEutra;
        std::list<LteRrcSap::MeasResultEutra>::iterator lit = measResultListEutra.begin ();
        
        double totalPower = 0.0;
        while (lit != measResultListEutra.end ())
        {
            if(lit->physCellId!=m_cellId && isBelongTo (lit->physCellId, m_cellId))
                totalPower += DBM2W (EutranMeasurementMapping::RsrpRange2Dbm (lit->rsrpResult));
            ++lit;
        }

        NS_LOG_INFO ("rnti " << rnti << " cellId " << pCellId << " totalPower " << totalPower);

        for (lit = measResultListEutra.begin (); lit!=measResultListEutra.end (); ++lit)
         {
            if(lit->physCellId==m_cellId || !isBelongTo (lit->physCellId, m_cellId)) continue;
            double rsrp = EutranMeasurementMapping::RsrpRange2Dbm (lit->rsrpResult);
            if (rsrp < m_rsrpThresh)
                continue;
            m_ubMap[lit->physCellId].insert (rnti);
            double pw = DBM2W (rsrp);
            double se = m_maxSe;
            if(totalPower - pw > 0)
                se = std::min (se, log2 (1.0 + pw / (totalPower - pw + m_noisePower)));
            m_seMap[(rnti<<16)|lit->physCellId] = se;
            NS_LOG_INFO ("cellId " << lit->physCellId << " rsrp " << rsrp << " power " << pw << " se " << se);
         }
         ++it;
    }
}

void
SetCoverSleepAlgorithm::Run ()
{
    NS_LOG_FUNCTION (this);
    
    m_smallCells = m_sleepManagementSapUser->GetSmallCellSet ();
    m_cells = m_smallCells;
    m_sleepManagementSapUser->GetConnectionMap (m_ueToCell, m_idMap);
    ConvertCqiToSpectralEfficiency ();
    CalculateSpectralEfficiencyAndCoverage ();

    PrintDetails ();

    CurrentStatus ();

    std::map<uint16_t, uint16_t> newConnection;
    while (!m_ues.empty () && !m_ubMap.empty ())
    {
        std::map<uint16_t, std::set<uint16_t> >::iterator mit = m_ubMap.begin ();
        while (mit != m_ubMap.end ())
        {
            if (!mit->second.empty ())
            {
                std::set<uint16_t> s;
                std::set_intersection (mit->second.begin (), mit->second.end (),
                                        m_ues.begin (), m_ues.end (),
                                        std::inserter(s, s.begin ())
                                    );
                mit->second = s;
            }
            ++mit;
        }

        uint16_t bj = 0;
        double maxSum = 0.0;
        std::map<uint16_t, std::set<uint16_t> >::iterator usuit = m_ubMap.begin ();
        while (usuit != m_ubMap.end ())
        {
            double sum = 0.0;
            std::set<uint16_t>::iterator ssit = usuit->second.begin ();
            while (ssit != usuit->second.end ())
            {
                sum += m_rho / m_seMap[(*ssit<<16)|usuit->first];
                ++ssit;
            }
            if (sum > maxSum)
            {
                bj = usuit->first;
                maxSum = sum;
            }
            ++usuit;
        }

        NS_LOG_INFO ("maximum used bandwidth " << maxSum << " cellId " << bj);

        double W = (1.0 - m_alpha) * m_maxW;
        std::priority_queue<BwRnti, std::vector<BwRnti>, BwRntiCompare> pqueue(BwRntiCompare(m_ueToCell, bj));
        std::set<uint16_t> &bjub = m_ubMap[bj];
        std::set<uint16_t>::iterator sit = bjub.begin ();
        while (sit != bjub.end ())
        {
            BwRnti bwRnti;
            bwRnti.rnti = *sit;
            bwRnti.bw = m_rho / m_seMap[(*sit<<16)|bj];
            NS_LOG_INFO ("bw " << bwRnti.bw << " RNTI " << bwRnti.rnti);
            pqueue.push (bwRnti);
            ++sit;
        }
        while (W > 0 && !pqueue.empty ())
        {
            BwRnti bwRnti = pqueue.top ();
            pqueue.pop ();
            if (W >= bwRnti.bw)
            {
                m_ues.erase (bwRnti.rnti);
                bjub.erase (bwRnti.rnti);
                W -= bwRnti.bw;
                if (m_ueToCell[bwRnti.rnti] != bj)
                    newConnection[bwRnti.rnti] = bj;
                NS_LOG_INFO ("find minumum used bandwidth " << bwRnti.bw << " RNTI " << bwRnti.rnti);
            }
            else
            {
              break;
            }
        }
        m_ubMap.erase (bj);
        m_smallCells.erase (bj);
    }

    NewStatus (newConnection);

    Record ();

    LteSleepManagementSap::SleepPolicy sleepPolicy;
    sleepPolicy.sleepCells = m_smallCells;
    sleepPolicy.newConnection = newConnection;
    PrintSleepPolicy (sleepPolicy);
    m_sleepManagementSapUser->SleepTrigger (sleepPolicy);

    Reset ();
    Simulator::Schedule (m_runPeriod, &SetCoverSleepAlgorithm::Run, this);
}

void
SetCoverSleepAlgorithm::CurrentStatus ()
{
    std::map<uint16_t, uint16_t>::iterator it;
    for (it=m_ueToCell.begin (); it!=m_ueToCell.end(); ++it)
    {
        Connection conn;
        conn.imsi = m_idMap[it->first];
        conn.macro = m_cellId;
        conn.oldsmall = it->second;
        conn.newsmall = it->second;
        m_conn[it->first] = conn;
    }
}

void
SetCoverSleepAlgorithm::NewStatus (std::map<uint16_t, uint16_t>& newConnection)
{
    std::map<uint16_t, uint16_t>::iterator it;
    for (it=newConnection.begin (); it!=newConnection.end(); ++it)
    {
        m_conn[it->first].newsmall = it->second;
    }
}

void
SetCoverSleepAlgorithm::Record ()
{
    std::ostringstream fname(std::ostringstream::ate);
    fname << "data/macro_" << m_cellId << ".dat";
    std::ofstream of(fname.str ().c_str (), std::ofstream::ate);
    std::set<uint16_t>::iterator sit;

    for(sit=m_cells.begin (); sit != m_cells.end (); ++sit)
        of << *sit << " ";
    of << '\n';

    for(sit=m_smallCells.begin (); sit != m_smallCells.end (); ++sit)
        of << *sit << " ";
    of << '\n';

    std::map<uint16_t, Connection>::iterator it;
    for(it=m_conn.begin (); it!=m_conn.end (); ++it)
    {
        Connection& conn = it->second;
        of << conn.imsi
           << " " << it->first
           << " " << conn.macro
           << " " << conn.oldsmall
           << " " << conn.newsmall
           << "\n";
    }
    of.close ();
}

void
SetCoverSleepAlgorithm::Reset ()
{
    m_smallCells.clear ();
    m_ues.clear ();
    m_ueToCell.clear ();
    m_idMap.clear ();
    m_rsrpMap.clear ();
    m_cqiMap.clear ();
    m_ubMap.clear ();
    m_seMap.clear ();
    m_conn.clear ();
}

void
SetCoverSleepAlgorithm::PrintDetails ()
{
    std::stringstream ss;
    std::set<uint16_t>::iterator usit;
    std::map<uint16_t, uint16_t>::iterator uumit;
    std::map<uint16_t, std::set<uint16_t> >::iterator usuit;
    std::map<uint32_t, double>::iterator udmit;

    usit = m_smallCells.begin ();
    ss << "details\n";
    ss << "====== Small Cells ======\n";
    while (usit != m_smallCells.end ())
    {
        ss << *usit++ << " ";
    }
    ss << "\n";

    usit = m_ues.begin ();
    ss << "====== UEs ======\n";
    while (usit != m_ues.end ())
    {
        ss << *usit++ << " ";
    }
    ss << "\n";

    uumit = m_ueToCell.begin ();
    ss << "====== Connections ======\n";
    while (uumit != m_ueToCell.end ())
    {
        ss << "[" << uumit->first << " , " << uumit->second << "] ";
        ++uumit;
    }
    ss << "\n";

    usuit = m_ubMap.begin ();
    ss << "====== Cover Set ======\n";
    while (usuit != m_ubMap.end ())
    {
        ss << usuit->first << ": ";
        usit = usuit->second.begin ();
        while (usit != usuit->second.end ())
        {
            ss << *usit++ << " ";
        }
        ss << "\n";
        ++usuit;
    }

    udmit = m_seMap.begin ();
    ss << "====== Spectral Efficiency ======\n";
    while (udmit != m_seMap.end ())
    {
        ss << ((udmit->first>>16)&0xffff) << " " << (udmit->first&0xffff) << " " << udmit->second << "\n";
        ++udmit;
    }

    NS_LOG_INFO (ss.str ());
}

void
SetCoverSleepAlgorithm::PrintSleepPolicy (LteSleepManagementSap::SleepPolicy& sleepPolicy)
{
    std::stringstream ss;
    std::set<uint16_t>::iterator sit = sleepPolicy.sleepCells.begin ();
    ss << "sleeppolicy\n";
    ss << "====== sleep cells ======\n";
    while (sit != sleepPolicy.sleepCells.end ())
    {
      ss << *sit++ << " ";
    }
    ss << "\n";

    std::map<uint16_t, uint16_t>::iterator mit = sleepPolicy.newConnection.begin ();
    ss << "====== new connections ======\n";
    while (mit != sleepPolicy.newConnection.end ())
    {
      ss << "(" << mit->first << " , " << mit->second << ") ";
      ++mit;
    }
    ss << "\n";

    NS_LOG_INFO (ss.str ());
}


} // end of namespace ns3