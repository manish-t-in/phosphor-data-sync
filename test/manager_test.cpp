// SPDX-License-Identifier: Apache-2.0
#include "manager_test.hpp"

std::filesystem::path ManagerTest::dataSyncCfgDir;
std::filesystem::path ManagerTest::tmpDataSyncDataDir;
nlohmann::json ManagerTest::commonJsonData;

TEST_F(ManagerTest, ParseDataSyncCfg)
{
    using namespace std::literals;
    namespace ed = data_sync::ext_data;

    commonJsonData = R"(
            {
                "Files": [
                    {
                        "Path": "/file/path/to/sync",
                        "Description": "Parse test file",
                        "SyncDirection": "Active2Passive",
                        "SyncType": "Immediate"
                    }
                ],
                "Directories": [
                    {
                        "Path": "/directory/path/to/sync",
                        "Description": "Parse test directory",
                        "SyncDirection": "Passive2Active",
                        "SyncType": "Periodic",
                        "Periodicity": "PT1S",
                        "RetryAttempts": 1,
                        "RetryInterval": "PT10M",
                        "ExcludeFilesList": ["/directory/file/to/ignore"],
                        "IncludeFilesList": ["/directory/file/to/consider"]
                    }
                ]
            }
        )"_json;

    std::filesystem::path dataSyncCommonCfgFile{dataSyncCfgDir /
                                                "common_test_config.json"};
    std::ofstream cfgFile(dataSyncCommonCfgFile);
    ASSERT_TRUE(cfgFile.is_open())
        << "Failed to open " << dataSyncCommonCfgFile;
    cfgFile << commonJsonData;
    cfgFile.close();

    std::unique_ptr<ed::ExternalDataIFaces> extDataIface =
        std::make_unique<ed::MockExternalDataIFaces>();

    ed::MockExternalDataIFaces* mockExtDataIfaces =
        dynamic_cast<ed::MockExternalDataIFaces*>(extDataIface.get());

    EXPECT_CALL(*mockExtDataIfaces, fetchBMCRedundancyMgrProps())
        // NOLINTNEXTLINE
        .WillRepeatedly([]() -> sdbusplus::async::task<> { co_return; });

    EXPECT_CALL(*mockExtDataIfaces, fetchSiblingBmcIP())
        // NOLINTNEXTLINE
        .WillRepeatedly([]() -> sdbusplus::async::task<> { co_return; });

    EXPECT_CALL(*mockExtDataIfaces, fetchRbmcCredentials())
        // NOLINTNEXTLINE
        .WillRepeatedly([]() -> sdbusplus::async::task<> { co_return; });

    sdbusplus::async::context ctx;

    data_sync::Manager manager{ctx, std::move(extDataIface),
                               ManagerTest::dataSyncCfgDir};

    EXPECT_FALSE(
        manager.containsDataSyncCfg(ManagerTest::commonJsonData["Files"][0]));

    ctx.spawn(
        sdbusplus::async::sleep_for(ctx, 1ns) |
        sdbusplus::async::execution::then([&ctx]() { ctx.request_stop(); }));
    ctx.run();

    EXPECT_TRUE(
        manager.containsDataSyncCfg(ManagerTest::commonJsonData["Files"][0]));
}
