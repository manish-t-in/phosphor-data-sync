// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "external_data_ifaces_impl.hpp"

#include <sdbusplus/async.hpp>
#include <sdbusplus/message.hpp>
#include <xyz/openbmc_project/Control/SyncBMCData/aserver.hpp>

namespace data_sync
{
class Manager;

namespace dbus_iface
{

/**
 * @class SyncBMCDataIfaces
 *
 * @brief SyncBMCDataIfaces class implements specific server functionality
 *        and method handling for the syncBMCData interface.
 */

class SyncBMCDataIfaces :
    public sdbusplus::aserver::xyz::openbmc_project::control::SyncBMCData<
        SyncBMCDataIfaces>
{
  public:
    SyncBMCDataIfaces() = default;
    SyncBMCDataIfaces(const SyncBMCDataIfaces&) = delete;
    SyncBMCDataIfaces& operator=(const SyncBMCDataIfaces&) = delete;
    SyncBMCDataIfaces(SyncBMCDataIfaces&&) = delete;
    SyncBMCDataIfaces& operator=(SyncBMCDataIfaces&&) = delete;
    virtual ~SyncBMCDataIfaces() = default;

    /**
     * @brief Constructor for SyncBMCDataIfaces.
     *
     * @param[in] ctx Reference to the async D-Bus context.
     * @param[in] mgr Reference of the manager.
     */
    SyncBMCDataIfaces(sdbusplus::async::context& ctx,
                      data_sync::Manager& manager);

    /**
     * @brief Handles the method call for a specific interface method.
     *
     * @param[in] type Method type identifier.
     * @param[in] msg D-Bus message containing the method call data.
     */
    sdbusplus::async::task<> method_call(start_full_sync_t type,
                                         sdbusplus::message_t& msg);

    /**
     * @brief This function is responsible for setting a specific property
     *        over D-Bus.
     *
     * @param[in] type The property type identifier.
     * @param[in] status The property enum value.
     *
     * @return true on success, false on failure.
     */
    bool set_property(full_sync_status_t type, FullSyncStatus status);

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

} // namespace dbus_iface
} // namespace data_sync
