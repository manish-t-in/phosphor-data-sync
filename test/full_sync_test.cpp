// SPDX-License-Identifier: Apache-2.0

#include "manager_test.hpp"

std::filesystem::path ManagerTest::dataSyncCfgDir;
std::filesystem::path ManagerTest::tmpDataSyncDataDir;

using FullSyncStatus = sdbusplus::common::xyz::openbmc_project::control::
    SyncBMCData::FullSyncStatus;

/*
 * Test the Full sync is triggered from the Active BMC to the Passive BMC,
 * ensuring that the Full Sync status is successfully completed
 */

TEST_F(ManagerTest, FullSyncA2PTest)
{
    using namespace std::literals;
    namespace ed = data_sync::ext_data;

    std::unique_ptr<ed::ExternalDataIFaces> extDataIface =
        std::make_unique<ed::MockExternalDataIFaces>();

    ed::MockExternalDataIFaces* mockExtDataIfaces =
        dynamic_cast<ed::MockExternalDataIFaces*>(extDataIface.get());

    ON_CALL(*mockExtDataIfaces, fetchBMCRedundancyMgrProps())
        // NOLINTNEXTLINE
        .WillByDefault([&mockExtDataIfaces]() -> sdbusplus::async::task<> {
        mockExtDataIfaces->setBMCRole(ed::BMCRole::Active);
        mockExtDataIfaces->setBMCRedundancy(true);
        co_return;
    });

    EXPECT_CALL(*mockExtDataIfaces, fetchSiblingBmcIP())
        // NOLINTNEXTLINE
        .WillRepeatedly([]() -> sdbusplus::async::task<> { co_return; });

    EXPECT_CALL(*mockExtDataIfaces, fetchRbmcCredentials())
        // NOLINTNEXTLINE
        .WillRepeatedly([]() -> sdbusplus::async::task<> { co_return; });

    nlohmann::json jsonData = {
        {"Files",
         {{{"Path", ManagerTest::tmpDataSyncDataDir.string() + "/srcFile1"},
           {"DestinationPath",
            ManagerTest::tmpDataSyncDataDir.string() + "/destFile1"},
           {"Description", "FullSync from Active to Passive bmc"},
           {"SyncDirection", "Active2Passive"},
           {"SyncType", "Immediate"}},
          {{"Path", ManagerTest::tmpDataSyncDataDir.string() + "/srcFile2"},
           {"DestinationPath",
            ManagerTest::tmpDataSyncDataDir.string() + "/destFile2"},
           {"Description", "FullSync from Active to Passive bmc"},
           {"SyncDirection", "Active2Passive"},
           {"SyncType", "Immediate"}},
          {{"Path", ManagerTest::tmpDataSyncDataDir.string() + "/srcFile3"},
           {"DestinationPath",
            ManagerTest::tmpDataSyncDataDir.string() + "/destFile3"},
           {"Description", "FullSync from Active to Passive bmc"},
           {"SyncDirection", "Active2Passive"},
           {"SyncType", "Immediate"}},
          {{"Path", ManagerTest::tmpDataSyncDataDir.string() + "/srcFile4"},
           {"DestinationPath",
            ManagerTest::tmpDataSyncDataDir.string() + "/destFile4"},
           {"Description", "FullSync from Active to Passive bmc"},
           {"SyncDirection", "Active2Passive"},
           {"SyncType", "Immediate"}}}},

        {"Directories",
         {{{"Path", ManagerTest::tmpDataSyncDataDir.string() + "/srcDir"},
           {"DestinationPath",
            ManagerTest::tmpDataSyncDataDir.string() + "/destDir"},
           {"Description", "FullSync from Active to Passive bmc directory"},
           {"SyncDirection", "Active2Passive"},
           {"SyncType", "Immediate"}}}}};

    std::string srcDir = jsonData["Directories"][0]["Path"];
    std::string destDir = jsonData["Directories"][0]["DestinationPath"];

    std::filesystem::create_directory(ManagerTest::tmpDataSyncDataDir /
                                      "srcDir");

    std::filesystem::create_directories(ManagerTest::tmpDataSyncDataDir /
                                        "srcDir" / "subDir");

    std::string dirFile = srcDir + "/dirFile";
    std::string subDirFile =
        (ManagerTest::tmpDataSyncDataDir / srcDir / "subDir" / "subDirFile")
            .string();

    ManagerTest::writeData(dirFile, "Data in directory file");
    ManagerTest::writeData(subDirFile, "Data in source directory file");

    ASSERT_EQ(ManagerTest::readData(dirFile), "Data in directory file");
    ASSERT_EQ(ManagerTest::readData(subDirFile),
              "Data in source directory file");

    std::string srcFile1{jsonData["Files"][0]["Path"]};
    std::string srcFile2{jsonData["Files"][1]["Path"]};
    std::string srcFile3{jsonData["Files"][2]["Path"]};
    std::string srcFile4{jsonData["Files"][3]["Path"]};

    std::string destFile1{jsonData["Files"][0]["DestinationPath"]};
    std::string destFile2{jsonData["Files"][1]["DestinationPath"]};
    std::string destFile3{jsonData["Files"][2]["DestinationPath"]};
    std::string destFile4{jsonData["Files"][3]["DestinationPath"]};

    writeConfig(jsonData);
    sdbusplus::async::context ctx;

    std::string data1{"Data written on the file1\n"};
    std::string data2{"Data written on the file2\n"};
    std::string data3{"Data written on the file3\n"};
    std::string data4{"Data written on the file4\n"};

    ManagerTest::writeData(srcFile1, data1);
    ManagerTest::writeData(srcFile2, data2);
    ManagerTest::writeData(srcFile3, data3);
    ManagerTest::writeData(srcFile4, data4);

    ASSERT_EQ(ManagerTest::readData(srcFile1), data1);
    ASSERT_EQ(ManagerTest::readData(srcFile2), data2);
    ASSERT_EQ(ManagerTest::readData(srcFile3), data3);
    ASSERT_EQ(ManagerTest::readData(srcFile4), data4);

    data_sync::Manager manager{ctx, std::move(extDataIface),
                               ManagerTest::dataSyncCfgDir};

    auto waitingForFullSyncToFinish =
        // NOLINTNEXTLINE
        [&](sdbusplus::async::context& ctx) -> sdbusplus::async::task<void> {
        auto status = manager.getFullSyncStatus();

        while (status != FullSyncStatus::FullSyncCompleted &&
               status != FullSyncStatus::FullSyncFailed)
        {
            co_await sdbusplus::async::sleep_for(ctx,
                                                 std::chrono::milliseconds(50));
            status = manager.getFullSyncStatus();
        }

        EXPECT_EQ(status, FullSyncStatus::FullSyncCompleted)
            << "FullSync status is not Completed!";

        EXPECT_EQ(ManagerTest::readData(destFile1), data1);
        EXPECT_EQ(ManagerTest::readData(destFile2), data2);
        EXPECT_EQ(ManagerTest::readData(destFile3), data3);
        EXPECT_EQ(ManagerTest::readData(destFile4), data4);

        EXPECT_EQ(ManagerTest::readData(dirFile), "Data in directory file");
        EXPECT_EQ(ManagerTest::readData(subDirFile),
                  "Data in source directory file");

        ctx.request_stop();

        co_return;
    };

    ctx.spawn(waitingForFullSyncToFinish(ctx));

    ctx.run();
}

/*
 * Test the Full sync is triggered from the Passive BMC to the Active BMC,
 * ensuring that the Full Sync status is successfully completed
 */

TEST_F(ManagerTest, FullSyncP2ATest)
{
    using namespace std::literals;
    namespace ed = data_sync::ext_data;

    std::unique_ptr<ed::ExternalDataIFaces> extDataIface =
        std::make_unique<ed::MockExternalDataIFaces>();

    ed::MockExternalDataIFaces* mockExtDataIfaces =
        dynamic_cast<ed::MockExternalDataIFaces*>(extDataIface.get());

    ON_CALL(*mockExtDataIfaces, fetchBMCRedundancyMgrProps())
        // NOLINTNEXTLINE
        .WillByDefault([&mockExtDataIfaces]() -> sdbusplus::async::task<> {
        mockExtDataIfaces->setBMCRole(ed::BMCRole::Passive);
        mockExtDataIfaces->setBMCRedundancy(true);
        co_return;
    });

    EXPECT_CALL(*mockExtDataIfaces, fetchSiblingBmcIP())
        // NOLINTNEXTLINE
        .WillRepeatedly([]() -> sdbusplus::async::task<> { co_return; });

    EXPECT_CALL(*mockExtDataIfaces, fetchRbmcCredentials())
        // NOLINTNEXTLINE
        .WillRepeatedly([]() -> sdbusplus::async::task<> { co_return; });

    nlohmann::json jsonData = {
        {"Files",
         {{{"Path", ManagerTest::tmpDataSyncDataDir.string() + "/srcFile1"},
           {"DestinationPath",
            ManagerTest::tmpDataSyncDataDir.string() + "/destFile1"},
           {"Description", "FullSync from Passive to Active bmc"},
           {"SyncDirection", "Passive2Active"},
           {"SyncType", "Immediate"}},
          {{"Path", ManagerTest::tmpDataSyncDataDir.string() + "/srcFile2"},
           {"DestinationPath",
            ManagerTest::tmpDataSyncDataDir.string() + "/destFile2"},
           {"Description", "FullSync from Passive to Active bmc"},
           {"SyncDirection", "Passive2Active"},
           {"SyncType", "Immediate"}},
          {{"Path", ManagerTest::tmpDataSyncDataDir.string() + "/srcFile3"},
           {"DestinationPath",
            ManagerTest::tmpDataSyncDataDir.string() + "/destFile3"},
           {"Description", "FullSync from Passive to Active bmc"},
           {"SyncDirection", "Passive2Active"},
           {"SyncType", "Immediate"}},
          {{"Path", ManagerTest::tmpDataSyncDataDir.string() + "/srcFile4"},
           {"DestinationPath",
            ManagerTest::tmpDataSyncDataDir.string() + "/destFile4"},
           {"Description", "FullSync from Passive to Active bmc"},
           {"SyncDirection", "Active2Passive"},
           {"SyncType", "Immediate"}}}},
        {"Directories",
         {{{"Path", ManagerTest::tmpDataSyncDataDir.string() + "/srcDir"},
           {"DestinationPath",
            ManagerTest::tmpDataSyncDataDir.string() + "/destDir"},
           {"Description", "Parse test directory"},
           {"SyncDirection", "Active2Passive"},
           {"SyncType", "Immediate"}}}}};

    std::string srcDir = jsonData["Directories"][0]["Path"];
    std::string destDir = jsonData["Directories"][0]["DestinationPath"];

    std::filesystem::create_directory(ManagerTest::tmpDataSyncDataDir /
                                      "srcDir");

    std::filesystem::create_directories(ManagerTest::tmpDataSyncDataDir /
                                        "srcDir" / "subDir");

    std::string dirFile = srcDir + "/dirFile";
    std::string subDirFile =
        (ManagerTest::tmpDataSyncDataDir / srcDir / "subDir" / "subDirFile")
            .string();

    ManagerTest::writeData(dirFile, "Data in directory file");
    ManagerTest::writeData(subDirFile, "Data in source directory file");

    ASSERT_EQ(ManagerTest::readData(dirFile), "Data in directory file");
    ASSERT_EQ(ManagerTest::readData(subDirFile),
              "Data in source directory file");

    std::string srcFile1{jsonData["Files"][0]["Path"]};
    std::string destFile1{jsonData["Files"][0]["DestinationPath"]};

    std::string srcFile2{jsonData["Files"][1]["Path"]};
    std::string destFile2{jsonData["Files"][1]["DestinationPath"]};

    std::string srcFile3{jsonData["Files"][2]["Path"]};
    std::string destFile3{jsonData["Files"][2]["DestinationPath"]};

    std::string srcFile4{jsonData["Files"][3]["Path"]};
    std::string destFile4{jsonData["Files"][3]["DestinationPath"]};

    writeConfig(jsonData);
    sdbusplus::async::context ctx;

    std::string data1{"Data written on the file1\n"};
    ManagerTest::writeData(srcFile1, data1);

    std::string data2{"Data written on the file2\n"};
    ManagerTest::writeData(srcFile2, data2);

    std::string data3{"Data written on the file3\n"};
    ManagerTest::writeData(srcFile3, data3);

    std::string data4{"Data written on the file4\n"};
    ManagerTest::writeData(srcFile4, data4);

    ASSERT_EQ(ManagerTest::readData(srcFile1), data1);
    ASSERT_EQ(ManagerTest::readData(srcFile2), data2);
    ASSERT_EQ(ManagerTest::readData(srcFile3), data3);
    ASSERT_EQ(ManagerTest::readData(srcFile4), data4);

    data_sync::Manager manager{ctx, std::move(extDataIface),
                               ManagerTest::dataSyncCfgDir};

    auto waitingForFullSyncToFinish =
        // NOLINTNEXTLINE
        [&](sdbusplus::async::context& ctx) -> sdbusplus::async::task<void> {
        auto status = manager.getFullSyncStatus();

        while (status != FullSyncStatus::FullSyncCompleted &&
               status != FullSyncStatus::FullSyncFailed)
        {
            co_await sdbusplus::async::sleep_for(ctx,
                                                 std::chrono::milliseconds(50));
            status = manager.getFullSyncStatus();
        }

        EXPECT_EQ(status, FullSyncStatus::FullSyncCompleted)
            << "FullSync status is not Completed!";

        EXPECT_EQ(ManagerTest::readData(destFile1), data1);
        EXPECT_EQ(ManagerTest::readData(destFile2), data2);
        EXPECT_EQ(ManagerTest::readData(destFile3), data3);
        EXPECT_NE(ManagerTest::readData(destFile4), data4);

        EXPECT_EQ(ManagerTest::readData(dirFile), "Data in directory file");
        EXPECT_EQ(ManagerTest::readData(subDirFile),
                  "Data in source directory file");

        ctx.request_stop();

        co_return;
    };

    ctx.spawn(waitingForFullSyncToFinish(ctx));

    ctx.run();
}

/*
 * Test the Full sync is triggered from the Passive BMC to the Active BMC,
 * ensuring that the Full Sync status is still InProgress.
 */

TEST_F(ManagerTest, FullSyncInProgressTest)
{
    using namespace std::literals;
    namespace ed = data_sync::ext_data;

    std::unique_ptr<ed::ExternalDataIFaces> extDataIface =
        std::make_unique<ed::MockExternalDataIFaces>();

    ed::MockExternalDataIFaces* mockExtDataIfaces =
        dynamic_cast<ed::MockExternalDataIFaces*>(extDataIface.get());

    ON_CALL(*mockExtDataIfaces, fetchBMCRedundancyMgrProps())
        // NOLINTNEXTLINE
        .WillByDefault([&mockExtDataIfaces]() -> sdbusplus::async::task<> {
        mockExtDataIfaces->setBMCRole(ed::BMCRole::Passive);
        mockExtDataIfaces->setBMCRedundancy(true);
        co_return;
    });

    EXPECT_CALL(*mockExtDataIfaces, fetchSiblingBmcIP())
        // NOLINTNEXTLINE
        .WillRepeatedly([]() -> sdbusplus::async::task<> { co_return; });

    EXPECT_CALL(*mockExtDataIfaces, fetchRbmcCredentials())
        // NOLINTNEXTLINE
        .WillRepeatedly([]() -> sdbusplus::async::task<> { co_return; });

    nlohmann::json jsonData = {
        {"Files",
         {{{"Path", ManagerTest::tmpDataSyncDataDir.string() + "/srcFile1"},
           {"DestinationPath",
            ManagerTest::tmpDataSyncDataDir.string() + "/destFile1"},
           {"Description", "FullSync from Passive to Active bmc"},
           {"SyncDirection", "Passive2Active"},
           {"SyncType", "Immediate"}},
          {{"Path", ManagerTest::tmpDataSyncDataDir.string() + "/srcFile2"},
           {"DestinationPath",
            ManagerTest::tmpDataSyncDataDir.string() + "/destFile2"},
           {"Description", "FullSync from Passive to Active bmc"},
           {"SyncDirection", "Passive2Active"},
           {"SyncType", "Immediate"}},
          {{"Path", ManagerTest::tmpDataSyncDataDir.string() + "/srcFile3"},
           {"DestinationPath",
            ManagerTest::tmpDataSyncDataDir.string() + "/destFile3"},
           {"Description", "FullSync from Passive to Active bmc"},
           {"SyncDirection", "Passive2Active"},
           {"SyncType", "Immediate"}},
          {{"Path", ManagerTest::tmpDataSyncDataDir.string() + "/srcFile4"},
           {"DestinationPath",
            ManagerTest::tmpDataSyncDataDir.string() + "/destFile4"},
           {"Description", "FullSync from Passive to Active bmc"},
           {"SyncDirection", "Passive2Active"},
           {"SyncType", "Immediate"}}}},
        {"Directories",
         {{{"Path", ManagerTest::tmpDataSyncDataDir.string() + "/srcDir"},
           {"DestinationPath",
            ManagerTest::tmpDataSyncDataDir.string() + "/destDir"},
           {"Description", "Parse test directory"},
           {"SyncDirection", "Active2Passive"},
           {"SyncType", "Immediate"}}}}};

    std::string srcDir = jsonData["Directories"][0]["Path"];
    std::string destDir = jsonData["Directories"][0]["DestinationPath"];

    std::filesystem::create_directory(ManagerTest::tmpDataSyncDataDir /
                                      "srcDir");

    std::filesystem::create_directories(ManagerTest::tmpDataSyncDataDir /
                                        "srcDir" / "subDir");

    std::string dirFile = srcDir + "/dirFile";
    ManagerTest::writeData(dirFile, "Data in directory file");

    ASSERT_EQ(ManagerTest::readData(dirFile), "Data in directory file");

    std::string srcFile1{jsonData["Files"][0]["Path"]};
    std::string destFile1{jsonData["Files"][0]["DestinationPath"]};

    std::string srcFile2{jsonData["Files"][1]["Path"]};
    std::string destFile2{jsonData["Files"][1]["DestinationPath"]};

    std::string srcFile3{jsonData["Files"][2]["Path"]};
    std::string destFile3{jsonData["Files"][2]["DestinationPath"]};

    std::string srcFile4{jsonData["Files"][3]["Path"]};
    std::string destFile4{jsonData["Files"][3]["DestinationPath"]};

    writeConfig(jsonData);
    sdbusplus::async::context ctx;

    std::string data1{"Data written on the file1\n"};
    ManagerTest::writeData(srcFile1, data1);

    std::string data2{"Data written on the file2\n"};
    ManagerTest::writeData(srcFile2, data2);

    std::string data3{"Data written on the file3\n"};
    ManagerTest::writeData(srcFile3, data3);

    std::string data4{"Data written on the file4\n"};
    ManagerTest::writeData(srcFile4, data4);

    ASSERT_EQ(ManagerTest::readData(srcFile1), data1);
    ASSERT_EQ(ManagerTest::readData(srcFile2), data2);
    ASSERT_EQ(ManagerTest::readData(srcFile3), data3);
    ASSERT_EQ(ManagerTest::readData(srcFile4), data4);

    data_sync::Manager manager{ctx, std::move(extDataIface),
                               ManagerTest::dataSyncCfgDir};

    auto waitingForFullSyncToFinish =
        // NOLINTNEXTLINE
        [&](sdbusplus::async::context& ctx) -> sdbusplus::async::task<void> {
        auto status = manager.getFullSyncStatus();
        while (status != FullSyncStatus::FullSyncInProgress)
        {
            co_await sdbusplus::async::sleep_for(ctx,
                                                 std::chrono::nanoseconds(200));
            status = manager.getFullSyncStatus();
        }

        co_await sdbusplus::async::sleep_for(ctx,
                                             std::chrono::microseconds(100));

        EXPECT_EQ(status, FullSyncStatus::FullSyncInProgress)
            << "FullSync status is not InProgress!";
        ctx.request_stop();

        co_return;
    };

    ctx.spawn(waitingForFullSyncToFinish(ctx));
    ctx.run();
}

/*
 * Test the Full sync is triggered from the Passive BMC to the Active BMC,
 * ensuring that the Full Sync status is Failed due to some ongoing issue.
 */

TEST_F(ManagerTest, FullSyncFailed)
{
    using namespace std::literals;
    namespace ed = data_sync::ext_data;

    std::unique_ptr<ed::ExternalDataIFaces> extDataIface =
        std::make_unique<ed::MockExternalDataIFaces>();

    ed::MockExternalDataIFaces* mockExtDataIfaces =
        dynamic_cast<ed::MockExternalDataIFaces*>(extDataIface.get());

    ON_CALL(*mockExtDataIfaces, fetchBMCRedundancyMgrProps())
        // NOLINTNEXTLINE
        .WillByDefault([&mockExtDataIfaces]() -> sdbusplus::async::task<> {
        mockExtDataIfaces->setBMCRole(ed::BMCRole::Passive);
        mockExtDataIfaces->setBMCRedundancy(true);
        co_return;
    });

    EXPECT_CALL(*mockExtDataIfaces, fetchSiblingBmcIP())
        // NOLINTNEXTLINE
        .WillRepeatedly([]() -> sdbusplus::async::task<> { co_return; });

    EXPECT_CALL(*mockExtDataIfaces, fetchRbmcCredentials())
        // NOLINTNEXTLINE
        .WillRepeatedly([]() -> sdbusplus::async::task<> { co_return; });

    nlohmann::json jsonData = {
        {"Files",
         {{{"Path", ManagerTest::tmpDataSyncDataDir.string() + "/srcFile1"},
           {"DestinationPath",
            ManagerTest::tmpDataSyncDataDir.string() + "/destFile1"},
           {"Description", "FullSync from Passive to Active bmc"},
           {"SyncDirection", "Passive2Active"},
           {"SyncType", "Immediate"}},
          {{"Path", ManagerTest::tmpDataSyncDataDir.string() + "/srcFile2"},
           {"DestinationPath",
            ManagerTest::tmpDataSyncDataDir.string() + "/destFile2"},
           {"Description", "FullSync from Passive to Active bmc"},
           {"SyncDirection", "Passive2Active"},
           {"SyncType", "Immediate"}},
          {{"Path", ManagerTest::tmpDataSyncDataDir.string() + "/srcFile3"},
           {"DestinationPath",
            ManagerTest::tmpDataSyncDataDir.string() + "/destFile3"},
           {"Description", "FullSync from Passive to Active bmc"},
           {"SyncDirection", "Passive2Active"},
           {"SyncType", "Immediate"}},
          {{"Path", "/path/to/src/srcFile4"},
           {"DestinationPath",
            ManagerTest::tmpDataSyncDataDir.string() + "/destFile4"},
           {"Description", "FullSync from Passive to Active bmc"},
           {"SyncDirection", "Passive2Active"},
           {"SyncType", "Immediate"}}}}};

    std::string srcFile1{jsonData["Files"][0]["Path"]};
    std::string destFile1{jsonData["Files"][0]["DestinationPath"]};

    std::string srcFile2{jsonData["Files"][1]["Path"]};
    std::string destFile2{jsonData["Files"][1]["DestinationPath"]};

    std::string srcFile3{jsonData["Files"][2]["Path"]};
    std::string destFile3{jsonData["Files"][2]["DestinationPath"]};

    std::string srcFile4{jsonData["Files"][3]["Path"]};
    std::string destFile4{jsonData["Files"][3]["DestinationPath"]};

    writeConfig(jsonData);
    sdbusplus::async::context ctx;

    std::string data1{"Data written on the file1\n"};
    ManagerTest::writeData(srcFile1, data1);

    std::string data2{"Data written on the file2\n"};
    ManagerTest::writeData(srcFile2, data2);

    std::string data3{"Data written on the file3\n"};
    ManagerTest::writeData(srcFile3, data3);

    ASSERT_EQ(ManagerTest::readData(srcFile1), data1);
    ASSERT_EQ(ManagerTest::readData(srcFile2), data2);
    ASSERT_EQ(ManagerTest::readData(srcFile3), data3);

    // Commented out the writing and verification for srcFile4 to simulate a
    // failure scenario, where the source file at
    // "/tmp/pdsDataDirXXXXXX/srcFile4" does not exist. This causes the rsync
    // operation to fail (return false), as the file is unavailable for syncing,
    // which helps test the failure path.

    std::string data4{"Data written on the file4\n"};
    // ManagerTest::writeData(srcFile4, data4);
    // ASSERT_EQ(ManagerTest::readData(srcFile4), data4);

    data_sync::Manager manager{ctx, std::move(extDataIface),
                               ManagerTest::dataSyncCfgDir};

    auto waitingForFullSyncToFinish =
        // NOLINTNEXTLINE
        [&](sdbusplus::async::context& ctx) -> sdbusplus::async::task<void> {
        auto status = manager.getFullSyncStatus();

        while (status != FullSyncStatus::FullSyncCompleted &&
               status != FullSyncStatus::FullSyncFailed)
        {
            co_await sdbusplus::async::sleep_for(ctx,
                                                 std::chrono::milliseconds(50));
            status = manager.getFullSyncStatus();
        }

        EXPECT_EQ(status, FullSyncStatus::FullSyncFailed)
            << "FullSync status is not Failed!!";

        EXPECT_EQ(ManagerTest::readData(destFile1), data1);
        EXPECT_EQ(ManagerTest::readData(destFile2), data2);
        EXPECT_EQ(ManagerTest::readData(destFile3), data3);
        EXPECT_NE(ManagerTest::readData(destFile4), data4);

        ctx.request_stop();

        co_return;
    };

    ctx.spawn(waitingForFullSyncToFinish(ctx));

    ctx.run();
}
