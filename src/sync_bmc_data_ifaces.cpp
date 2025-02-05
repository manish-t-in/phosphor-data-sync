// SPDX-License-Identifier: Apache-2.0

#include "sync_bmc_data_ifaces.hpp"

#include "manager.hpp"

#include <phosphor-logging/lg2.hpp>

namespace data_sync::dbus_ifaces
{

using SyncBMCData =
    sdbusplus::common::xyz::openbmc_project::control::SyncBMCData;

SyncBMCDataIface::SyncBMCDataIface(sdbusplus::async::context& ctx,
                                   data_sync::Manager& manager) :
    sdbusplus::aserver::xyz::openbmc_project::control::SyncBMCData<
        SyncBMCDataIface>(ctx, SyncBMCData::instance_path),
    _manager(manager), _ctx(ctx)
{
    emit_added();
}

sdbusplus::async::task<>
    // NOLINTNEXTLINE
    SyncBMCDataIface::method_call([[maybe_unused]] start_full_sync_t type)
{
    if (_manager.isSiblingBmcNotAvailable())
    {
        lg2::error(
            "Sibling BMC is not available, Unable to retrieve the BMC IP ");
        throw sdbusplus::xyz::openbmc_project::Control::SyncBMCData::Error::
            SiblingBMCNotAvailable();
    }

    if (full_sync_status_ == FullSyncStatus::FullSyncInProgress)
    {
        lg2::error(
            "Full Sync in progress. Operation cannot proceed at this time ");
        throw sdbusplus::xyz::openbmc_project::Control::SyncBMCData::Error::
            FullSyncInProgress();
    }

    co_return _ctx.spawn(_manager.startFullSync());
}

} // namespace data_sync::dbus_ifaces
