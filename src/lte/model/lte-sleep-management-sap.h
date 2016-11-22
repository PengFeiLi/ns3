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

namespace ns3 {


/**
 * \brief Service Access Point offered by the sleep algorithm instance
 *        to the macro cell RRC instance.
 */
class LteSleepManagementSapProvider
{
public:
    virtual ~LteSleepManagementSapProvider ();

    virtual void ReportSleepRsrp (uint16_t rnti, LteRrcSap::MeasResults measResults) = 0;

    virtual void ReportSleepSpectrumEfficiency (uint16_t cellId, LteRrcSap::SeResults seResults) = 0;
}; // end of LteSleepManagementSapProvider


/**
 * \brief Service Access Point offered by the macro cell RRC instance to the
 *        sleep algorithm instance.
 */
class LteSleepManagementSapUser
{
public:
    virtual ~LteSleepManagementSapUser ();

    virtual void CollectSleepInformation () = 0;

    virtual void SleepTrigger (LteRrcSap::SleepPolicy sleepPolicy) = 0;
}; // end of LteSleepManagementSapUser


template <class C>
class MemberLteSleepManagementSapProvider : public LteSleepManagementSapProvider
{
public:
    MemberLteSleepManagementSapProvider (C* owner);

    virtual void ReportSleepRsrp (uint16_t rnti, LteRrcSap::MeasResults measResults);

    virtual void ReportSleepSpectrumEfficiency (uint16_t cellId, LteRrcSap::SeResults seResults); 

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
MemberLteSleepManagementSapProvider<C>::ReportSleepRsrp (uint16_t rnti, LteRrcSap::MeasResults measResults)
{
    m_owner->DoReportSleepCellRsrp (rnti, measResults);
}

template <class C>
void
MemberLteSleepManagementSapProvider<C>::ReportSleepSpectrumEfficiency (uint16_t cellId, LteRrcSap::SeResults seResults)
{
    m_owner->DoReportSleepSpectrumEfficiency (cellId, seResults);
}


template <class C>
class MemberLteSleepManagementSapUser : public LteSleepManagementSapUser
{
public:
    MemberLteSleepManagementSapUser (C* owner);

    virtual void CollectSleepInformation ();

    virtual void SleepTrigger (LteRrcSap::SleepPolicy sleepPolicy);

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
void
MemberLteSleepManagementSapUser<C>::CollectSleepInformation ()
{
    m_owner->DoCollectSleepInformation ();
}

template <class C>
void
MemberLteSleepManagementSapUser<C>::SleepTrigger (LteRrcSap::SleepPolicy sleepPolicy)
{
    m_owner->DoSleepTrigger (sleepPolicy);
}

} // end of namespace ns3

 #endif /* LTE_SLEEP_MANAGEMENT_SAP_H */