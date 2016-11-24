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

#include "swes-sleep-algorithm.h"
#include <ns3/log.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SwesSleepAlgorithm");

NS_OBJECT_ENSURE_REGISTERED (SwesSleepAlgorithm);


SwesSleepAlgorithm::SwesSleepAlgorithm ()
    : m_sleepManagementSapUser (0),
      m_threshold (0),
      m_measId (0)
{
    NS_LOG_FUNCTION (this);
    m_sleepManagementSapProvider = new MemberLteSleepManagementSapProvider<SwesSleepAlgorithm> (this);
}


SwesSleepAlgorithm::~SwesSleepAlgorithm ()
{
    NS_LOG_FUNCTION (this);
}


void
SwesSleepAlgorithm::DoDispose ()
{
    NS_LOG_FUNCTION (this);
    delete m_sleepManagementSapProvider;
}


TypeId
SwesSleepAlgorithm::GetTypeId ()
{
    static TypeId tid = TypeId ("ns3::SwesSleepAlgorithm")
        .SetParent<LteSleepAlgorithm> ()
        .SetGroupName ("Lte")
        .AddConstructor<SwesSleepAlgorithm> ()
    ;
    return tid;
}


void
SwesSleepAlgorithm::SetLteSleepManagementSapUser (LteSleepManagementSapUser* s)
{
    NS_LOG_FUNCTION (this << s);
    m_sleepManagementSapUser = s;
}


LteSleepManagementSapProvider*
SwesSleepAlgorithm::GetLteSleepManagementSapProvider ()
{
    NS_LOG_FUNCTION (this);
    return m_sleepManagementSapProvider;
}


void
SwesSleepAlgorithm::DoInitialize ()
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
}


void
SwesSleepAlgorithm::DoReportUeMeas (uint16_t rnti, LteRrcSap::MeasResults measResults)
{
    NS_LOG_FUNCTION (this);
}


} // end of namespace ns3