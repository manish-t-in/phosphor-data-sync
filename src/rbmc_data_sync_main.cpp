// SPDX-License-Identifier: Apache-2.0

#include "config.h"

#include "external_data_ifaces_impl.hpp"
#include "manager.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async/context.hpp>
#include <sdbusplus/server/manager.hpp>
#include <xyz/openbmc_project/Control/SyncBMCData/common.hpp>

int main()
{
    using SyncBMCData =
        sdbusplus::common::xyz::openbmc_project::control::SyncBMCData;

    sdbusplus::async::context ctx;
    sdbusplus::server::manager_t objManager{ctx, SyncBMCData::instance_path};

    data_sync::Manager manager{
        ctx, std::make_unique<data_sync::ext_data::ExternalDataIFacesImpl>(ctx),
        DATA_SYNC_CONFIG_DIR};

    // clang-tidy currently mangles this into something unreadable
    // NOLINTNEXTLINE
    ctx.spawn([](sdbusplus::async::context& ctx) -> sdbusplus::async::task<> {
        ctx.request_name(SyncBMCData::interface);
        co_return;
    }(ctx));

    ctx.run();

    return 0;
}
