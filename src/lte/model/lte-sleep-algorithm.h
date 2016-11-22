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

#ifndef LTE_SLEEP_ALGORITHM_H
#define LTE_SLEEP_ALGORITHM_H

#include <ns3/object.h>
#include <ns3/lte-rrc-sap.h>

namespace ns3 {


class LteSleepManagementSapUser;
class LteSleepManagementSapProvider;


/**
 * \brief The abstract base class of a sleep algorithm the operates using
 *        the Sleep Management SAP interface.
 *
 * Sleep algorithm receives measurement reports from an macro cell RRC instance
 * and tells the macro cell RRC instance when to do a sleep
 */

class LteSleepAlgorithm : public Object
{
public:
    LteSleepAlgorithm ();
    virtual ~LteSleepAlgorithm ();

    static TypeId GetTypeId ();

    virtual void SetLteSleepManagementSapUser (LteSleepManagementSapUser* s) = 0;

    virtual LteSleepManagementSapProvider* GetLteSleepManagementSapProvider () = 0;

protected:
    virtual void DoDispose ();

    virtual void DoReportSleepCellRsrp (uint16_t rnti, LteRrcSap::MeasResults measResults) = 0;

    virtual void DoReportSleepSpectrumEfficiency (uint16_t cellId, LteRrcSap::SeResults seResults) = 0;
}; // end of class LteSleepAlgorithm

} // end of namespace ns3

#endif /* LTE_SLEEP_ALGORITHM_H */