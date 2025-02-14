// SPDX-License-Identifier: Apache-2.0

#include "manager.hpp"

#include <nlohmann/json.hpp>
#include <phosphor-logging/lg2.hpp>

#include <cstdlib>
#include <exception>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>

namespace data_sync
{

Manager::Manager(sdbusplus::async::context& ctx,
                 std::unique_ptr<ext_data::ExternalDataIFaces>&& extDataIfaces,
                 const fs::path& dataSyncCfgDir) :
    _ctx(ctx), _extDataIfaces(std::move(extDataIfaces)),
    _dataSyncCfgDir(dataSyncCfgDir), _dbusIfaces(ctx, *this)
{
    _ctx.spawn(init());
}

// NOLINTNEXTLINE
sdbusplus::async::task<> Manager::init()
{
    co_await sdbusplus::async::execution::when_all(
        parseConfiguration(), _extDataIfaces->startExtDataFetches());

    // TODO: Implement logic to trigger full sync based on the availability of
    // sibling BMC.
    co_await startFullSync();

    co_return co_await startSyncEvents();
}

// NOLINTNEXTLINE
sdbusplus::async::task<> Manager::parseConfiguration()
{
    auto parse = [this](const auto& configFile) {
        try
        {
            std::ifstream file;
            file.open(configFile.path());

            nlohmann::json configJSON(nlohmann::json::parse(file));

            if (configJSON.contains("Files"))
            {
                std::ranges::transform(
                    configJSON["Files"],
                    std::back_inserter(this->_dataSyncConfiguration),
                    [](const auto& element) {
                    return config::DataSyncConfig(element);
                });
            }
            if (configJSON.contains("Directories"))
            {
                std::ranges::transform(
                    configJSON["Directories"],
                    std::back_inserter(this->_dataSyncConfiguration),
                    [](const auto& element) {
                    return config::DataSyncConfig(element);
                });
            }
        }
        catch (const std::exception& e)
        {
            // TODO Create error log
            lg2::error("Failed to parse the configuration file : {CONFIG_FILE},"
                       " exception : {EXCEPTION}",
                       "CONFIG_FILE", configFile.path(), "EXCEPTION", e);
        }
    };

    if (fs::exists(_dataSyncCfgDir) && fs::is_directory(_dataSyncCfgDir))
    {
        std::ranges::for_each(fs::directory_iterator(_dataSyncCfgDir), parse);
    }

    co_return;
}

bool Manager::isSyncEligible(const config::DataSyncConfig& dataSyncCfg)
{
    using enum config::SyncDirection;
    using enum ext_data::BMCRole;

    if ((dataSyncCfg._syncDirection == Bidirectional) ||
        ((dataSyncCfg._syncDirection == Active2Passive) &&
         this->_extDataIfaces->bmcRole() == Active) ||
        ((dataSyncCfg._syncDirection == Passive2Active) &&
         this->_extDataIfaces->bmcRole() == Passive))
    {
        return true;
    }
    else
    {
        // TODO Trace is required, will overflow?
        lg2::debug("Sync is not required for [{PATH}] due to "
                   "SyncDirection: {SYNC_DIRECTION} BMCRole: {BMC_ROLE}",
                   "PATH", dataSyncCfg._path, "SYNC_DIRECTION",
                   dataSyncCfg.getSyncDirectionInStr(), "BMC_ROLE",
                   _extDataIfaces->bmcRole());
    }
    return false;
}

// NOLINTNEXTLINE
sdbusplus::async::task<> Manager::startSyncEvents()
{
    std::ranges::for_each(
        _dataSyncConfiguration |
            std::views::filter([this](const auto& dataSyncCfg) {
        return this->isSyncEligible(dataSyncCfg);
    }),
        [this](const auto& dataSyncCfg) {
        using enum config::SyncType;
        if (dataSyncCfg._syncType == Immediate)
        {
            this->_ctx.spawn(this->monitorDataToSync(dataSyncCfg));
        }
        else if (dataSyncCfg._syncType == Periodic)
        {
            this->_ctx.spawn(this->monitorTimerToSync(dataSyncCfg));
        }
    });
    co_return;
}

// NOLINTNEXTLINE
sdbusplus::async::task<bool>
    Manager::syncData(const config::DataSyncConfig& dataSyncCfg)
{
    using namespace std::string_literals;
    std::string syncCmd{"rsync --archive --compress"};
    syncCmd.append(" "s + dataSyncCfg._path);

#ifdef UNIT_TEST
    syncCmd.append(" "s);
#else
    // TODO Support for remote (i,e sibling BMC) copying needs to be added.
#endif

    // Add destination data path
    syncCmd.append(dataSyncCfg._destPath.value_or(dataSyncCfg._path));
    int result = std::system(syncCmd.c_str()); // NOLINT
    if (result != 0)
    {
        // TODO:
        // Retry and create error log and disable redundancy if retry is failed.
        lg2::error("Error syncing: {PATH}", "PATH", dataSyncCfg._path);
        co_return false;
    }
    co_return true;
}

// NOLINTNEXTLINE
sdbusplus::async::task<> Manager::monitorDataToSync(
    [[maybe_unused]] const config::DataSyncConfig& dataSyncCfg)
{
    // TODO Create inotify events to monitor data for sync
    co_return;
}

sdbusplus::async::task<>
    // NOLINTNEXTLINE
    Manager::monitorTimerToSync(const config::DataSyncConfig& dataSyncCfg)
{
    while (!_ctx.stop_requested())
    {
        co_await sdbusplus::async::sleep_for(
            _ctx, dataSyncCfg._periodicityInSec.value());
        co_await syncData(dataSyncCfg);
    }
    co_return;
}

bool Manager::isSiblingBmcAvailable()
{
    if (_extDataIfaces->siblingBmcIP().empty())
    {
        return true;
    }
    return false;
}

// NOLINTNEXTLINE
sdbusplus::async::task<void> Manager::startFullSync()
{
    std::mutex mtx;
    std::vector<bool> syncResults;
    int completedTasks = 0, spawnedTasks = 0;

    for (const auto& cfg : _dataSyncConfiguration)
    {
        if (isSyncEligible(cfg))
        {
            _ctx.spawn(this->syncData(cfg) |
                       stdexec::then([&syncResults, this, &mtx](bool result) {
                {
                    std::lock_guard<std::mutex> lock(mtx);
                    syncResults.push_back(result);
                }
            }));
            spawnedTasks++; // Count the number of spawned tasks
        }
    }

    while (completedTasks != spawnedTasks)
    {
        co_await sdbusplus::async::sleep_for(_ctx,
                                             std::chrono::milliseconds(50));
        {
            std::lock_guard<std::mutex> lock(mtx);
            completedTasks = syncResults.size();
        }
    }

    co_return;
}

} // namespace data_sync
