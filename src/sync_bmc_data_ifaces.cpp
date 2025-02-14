// SPDX-License-Identifier: Apache-2.0

#include "sync_bmc_data_ifaces.hpp"

#include "manager.hpp"

#include <phosphor-logging/lg2.hpp>

namespace data_sync::dbus_iface
{

using syncBMCData =
    sdbusplus::common::xyz::openbmc_project::control::SyncBMCData;

SyncBMCDataIfaces::SyncBMCDataIfaces(sdbusplus::async::context& ctx,
                                     data_sync::Manager& manager) :
    sdbusplus::aserver::xyz::openbmc_project::control::SyncBMCData<
        SyncBMCDataIfaces>(ctx, syncBMCData::instance_path),
    _manager(manager), _ctx(ctx)
{
    emit_added();
}

// NOLINTNEXTLINE
sdbusplus::async::task<>
    SyncBMCDataIfaces::method_call([[maybe_unused]] start_full_sync_t type,
                                   [[maybe_unused]] sdbusplus::message_t& msg)
{
    if (_manager.isSiblingBmcAvailable())
    {
        lg2::error(
            "Error: Sibling BMC is not available, Unable to retrieve the BMC IP ");
        throw sdbusplus::xyz::openbmc_project::Control::SyncBMCData::Error::
            SiblingBMCNotAvailable();
    }

    if (full_sync_status_ == FullSyncStatus::FullSyncInProgress)
    {
        lg2::error(
            "Error: Full Sync in progress. Operation cannot proceed at this time ");
        throw sdbusplus::xyz::openbmc_project::Control::SyncBMCData::Error::
            FullSyncInProgress();
    }

    co_return _ctx.spawn(_manager.startFullSync());
}

bool SyncBMCDataIfaces::set_property([[maybe_unused]] full_sync_status_t type,
                                     FullSyncStatus new_status)
{
    if (full_sync_status_ == new_status)
    {
        return false;
    }
    full_sync_status_ = new_status;
    return true;
}

} // namespace data_sync::dbus_iface
