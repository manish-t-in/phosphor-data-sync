// SPDX-License-Identifier: Apache-2.0

#include "external_data_ifaces.hpp"

#include <gmock/gmock.h>

namespace data_sync::ext_data
{

class MockExternalDataIFaces : public ExternalDataIFaces
{
  public:
    MockExternalDataIFaces() = default;

    MOCK_METHOD(sdbusplus::async::task<>, fetchBMCRedundancyMgrProps, (),
                (override));
    MOCK_METHOD(sdbusplus::async::task<>, fetchSiblingBmcIP, (), (override));
    MOCK_METHOD(sdbusplus::async::task<>, fetchRbmcCredentials, (), (override));
    void setBMCRole(const BMCRole& role)
    {
        return bmcRole(role);
    }
    void setBMCRedundancy(const BMCRedundancy& bmcRedundancy)
    {
        return this->bmcRedundancy(bmcRedundancy);
    }
};

} // namespace data_sync::ext_data
