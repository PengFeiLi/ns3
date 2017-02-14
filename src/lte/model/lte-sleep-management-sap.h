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

#ifndef LTE_SLEEP_MANAGEMENT_SAP_H
#define LTE_SLEEP_MANAGEMENT_SAP_H

#include <ns3/object.h>
#include <ns3/lte-rrc-sap.h>
#include <ns3/epc-x2-sap.h>

#include <set>
#include <map>


namespace ns3 {


class LteSleepManagementSap
{
public:
    virtual ~LteSleepManagementSap ();

    struct SleepPolicy
    {
      std::set<uint16_t> sleepCells;
      std::map<uint16_t, uint16_t> newConnection;
    };

};


/**
 * \brief Service Access Point offered by the sleep algorithm instance
 *        to the macro cell RRC instance.
 */
class LteSleepManagementSapProvider : public LteSleepManagementSap
{
public:
    virtual ~LteSleepManagementSapProvider ();

    virtual void ReportUeMeas (uint16_t rnti, LteRrcSap::MeasResults measResults) = 0;

    virtual void ReportDlCqi (uint16_t cellId, EpcX2Sap::DlCqiParams params) = 0;

}; // end of LteSleepManagementSapProvider


/**
 * \brief Service Access Point offered by the macro cell RRC instance to the
 *        sleep algorithm instance.
 */
class LteSleepManagementSapUser : public LteSleepManagementSap
{
public:
    virtual ~LteSleepManagementSapUser ();

    virtual uint8_t AddUeMeasReportConfigForSleep (LteRrcSap::ReportConfigEutra reportConfig) = 0;

    virtual void SleepTrigger (SleepPolicy sleepPolicy) = 0;

    virtual std::set<uint16_t> GetSmallCellSet () = 0;

    virtual void GetConnectionMap (std::map<uint16_t, uint16_t>& conn, std::map<uint16_t, uint64_t>& idmap) = 0;

}; // end of LteSleepManagementSapUser


template <class C>
class MemberLteSleepManagementSapProvider : public LteSleepManagementSapProvider
{
public:
    MemberLteSleepManagementSapProvider (C* owner);

    virtual void ReportUeMeas (uint16_t rnti, LteRrcSap::MeasResults measResults);

    virtual void ReportDlCqi (uint16_t cellId, EpcX2Sap::DlCqiParams params);

private:
    MemberLteSleepManagementSapProvider ();
    C* m_owner;
}; // end of MemberLteSleepManagementSapProvider

template <class C>
MemberLteSleepManagementSapProvider<C>::MemberLteSleepManagementSapProvider (C* owner)
    : m_owner (owner)
{
}

template <class C>
void
MemberLteSleepManagementSapProvider<C>::ReportUeMeas (uint16_t rnti, LteRrcSap::MeasResults measResults)
{
    m_owner->DoReportUeMeas (rnti, measResults);
}

template <class C>
void
MemberLteSleepManagementSapProvider<C>::ReportDlCqi (uint16_t cellId, EpcX2Sap::DlCqiParams params)
{
    m_owner->DoReportDlCqi (cellId, params);
}

template <class C>
class MemberLteSleepManagementSapUser : public LteSleepManagementSapUser
{
public:
    MemberLteSleepManagementSapUser (C* owner);

    virtual uint8_t AddUeMeasReportConfigForSleep (LteRrcSap::ReportConfigEutra reportConfig);

    virtual void SleepTrigger (SleepPolicy sleepPolicy);

    virtual std::set<uint16_t> GetSmallCellSet ();

    virtual void GetConnectionMap (std::map<uint16_t, uint16_t>& conn, std::map<uint16_t, uint64_t>& idmap);

private:
    MemberLteSleepManagementSapUser ();
    C* m_owner;
}; // end of MemberLteSleepManagementSapUser

template <class C>
MemberLteSleepManagementSapUser<C>::MemberLteSleepManagementSapUser (C* owner)
    : m_owner (owner)
{
}

template <class C>
uint8_t
MemberLteSleepManagementSapUser<C>::AddUeMeasReportConfigForSleep (LteRrcSap::ReportConfigEutra reportConfig)
{
    return m_owner->DoAddUeMeasReportConfigForSleep (reportConfig);
}

template <class C>
void
MemberLteSleepManagementSapUser<C>::SleepTrigger (SleepPolicy sleepPolicy)
{
    m_owner->DoSleepTrigger (sleepPolicy);
}

template <class C>
std::set<uint16_t>
MemberLteSleepManagementSapUser<C>::GetSmallCellSet ()
{
    return m_owner->DoGetSmallCellSet ();
}

template <class C>
void
MemberLteSleepManagementSapUser<C>::GetConnectionMap (std::map<uint16_t, uint16_t>& conn, std::map<uint16_t, uint64_t>& idmap)
{
    return m_owner->DoGetConnectionMap (conn, idmap);
}


} // end of namespace ns3

 #endif /* LTE_SLEEP_MANAGEMENT_SAP_H */