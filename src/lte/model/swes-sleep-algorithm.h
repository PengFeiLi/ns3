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

#ifndef SWES_SLEEP_ALGORITHM_H
#define SWES_SLEEP_ALGORITHM_H

#include "lte-sleep-algorithm.h"
#include "lte-sleep-management-sap.h"
#include "lte-rrc-sap.h"

namespace ns3 {


class SwesSleepAlgorithm : public LteSleepAlgorithm
{

    friend class MemberLteSleepManagementSapProvider<SwesSleepAlgorithm>;

public:
    SwesSleepAlgorithm ();
    virtual ~ SwesSleepAlgorithm ();

    static TypeId GetTypeId ();

    virtual void SetLteSleepManagementSapUser (LteSleepManagementSapUser* s);

    virtual LteSleepManagementSapProvider* GetLteSleepManagementSapProvider ();

protected:
    virtual void DoInitialize ();

    virtual void DoDispose ();

    virtual void DoReportUeMeas (uint16_t rnti, LteRrcSap::MeasResults measResults);

private:
    LteSleepManagementSapProvider* m_sleepManagementSapProvider;

    LteSleepManagementSapUser* m_sleepManagementSapUser;

    uint8_t m_threshold;

    uint8_t m_measId;

}; // end of SwesSleepAlgorithm


} // end of namespace ns3


#endif /* SWES_SLEEP_ALGORITHM_H */