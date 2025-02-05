// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "external_data_ifaces_impl.hpp"

#include <sdbusplus/async.hpp>
#include <sdbusplus/message.hpp>
#include <xyz/openbmc_project/Control/SyncBMCData/aserver.hpp>

namespace data_sync
{
class Manager;

namespace dbus_ifaces
{

/**
 * @class SyncBMCDataIface
 *
 * @brief SyncBMCDataIface class implements the dbus server functionality. It
 *        provides methods to perform full synchronization
 *
 */

class SyncBMCDataIface :
    public sdbusplus::aserver::xyz::openbmc_project::control::SyncBMCData<
        SyncBMCDataIface>
{
  public:
    SyncBMCDataIface(const SyncBMCDataIface&) = delete;
    SyncBMCDataIface& operator=(const SyncBMCDataIface&) = delete;
    SyncBMCDataIface(SyncBMCDataIface&&) = delete;
    SyncBMCDataIface& operator=(SyncBMCDataIface&&) = delete;
    virtual ~SyncBMCDataIface() = default;

    /**
     * @brief Constructor for SyncBMCDataIface.
     *
     * @param[in] ctx Reference to the async D-Bus context.
     * @param[in] manager Reference of the manager.
     */
    SyncBMCDataIface(sdbusplus::async::context& ctx,
                     data_sync::Manager& manager);

    /**
     * @brief Handles the FullSync method call for the SyncBmcData interface,
     *
     * @param[in] type Method type identifier.
     * @param[in] msg D-Bus message containing the method call data.
     */
    sdbusplus::async::task<> method_call(start_full_sync_t type);

  private:
    /**
     * @brief Reference to the Manager object.
     */
    Manager& _manager;

    /**
     * @brief The async context object used to perform operations
     *        asynchronously as required.
     */
    sdbusplus::async::context& _ctx;
};
} // namespace dbus_ifaces
} // namespace data_sync
