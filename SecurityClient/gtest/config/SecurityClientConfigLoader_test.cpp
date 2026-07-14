/*
 * Copyright (c) 2026 AUTOCRYPT Co., Ltd. All rights reserved.
 *
 * This software is the confidential and proprietary information of
 * AUTOCRYPT Co., Ltd. ("Confidential Information").
 *
 * You shall not disclose such Confidential Information and shall use
 * it only in accordance with the terms of the license agreement
 * you entered into with AUTOCRYPT Co., Ltd.
 *
 * For more information, please refer to the LICENSE.md file in the
 * root directory or contact contact@autocrypt.io.
 */

#include "../../config/SecurityClientConfigLoader.hpp"

#include "Log.hpp"
#include "config_dds_ids.h"

#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

namespace {

using AC::ddsIds::securityClient::SecurityClientConfig;
using AC::ddsIds::securityClient::SecurityClientConfigLoader;

TEST(SecurityClientConfigLoaderTest, LoadsRuntimeConfigAndPolicyConfig)
{
    std::filesystem::path policyRulePath = POLICY_RULE_DIR;
    const std::filesystem::path runtimePath = policyRulePath / "sample_runtime_config.json";
    const std::filesystem::path policyPath = policyRulePath / "sample_policy_config.json";

    SecurityClientConfigLoader loader;
    SecurityClientConfig config;

    ASSERT_TRUE(loader.load(runtimePath, policyPath, config));
    EXPECT_EQ("security-client-v2", config.system.clientId);
    EXPECT_EQ("info", config.system.logLevel);
    ASSERT_FALSE(config.collection.sources.empty());
    EXPECT_EQ("rt/DoorSwitch", config.collection.sources.front().topic);
    ASSERT_FALSE(config.policies.signalMismatchRules.empty());
    EXPECT_EQ(100u, config.policies.signalMismatchRules.front().ruleId);
    EXPECT_EQ("rt/DoorUnlockLockIndicator", config.policies.signalMismatchRules.front().topic);
}

TEST(SecurityClientConfigLoaderTest, RejectsMissingRuntimeFile)
{
    SecurityClientConfigLoader loader;
    SecurityClientConfig config;

    EXPECT_FALSE(loader.load("/tmp/does-not-exist-runtime.json",
                             std::filesystem::path(__FILE__).parent_path() / "sample_policy_config.json", config));
}

TEST(SecurityClientConfigLoaderTest, RejectsInvalidDetectorType)
{
    const auto runtimePath = std::filesystem::temp_directory_path() / "security_client_invalid_detector_runtime.json";
    {
        std::ofstream output(runtimePath);
        ASSERT_TRUE(output.is_open());
        output << R"({
            "system": {"client_id": "sc-test", "log_level": "info"},
            "collection": {"sources": []},
            "analysis_detection": {
                "detectors": [
                    {"id": "bad-detector", "type": "not-a-real-type", "enabled": true}
                ]
            },
            "alert_response": {"alert_sinks": [], "response_handlers": []}
        })";
    }

    // const auto baseDir = std::filesystem::path(__FILE__).parent_path() / "../../config";
    const auto policyPath = "sample_policy_config.json";

    SecurityClientConfigLoader loader;
    SecurityClientConfig config;

    EXPECT_FALSE(loader.load(runtimePath, policyPath, config));

    std::filesystem::remove(runtimePath);
}

TEST(SecurityClientConfigLoaderTest, RejectsInvalidSeverity)
{
    const auto policyPath = std::filesystem::temp_directory_path() / "security_client_invalid_severity_policy.json";
    {
        std::ofstream output(policyPath);
        ASSERT_TRUE(output.is_open());
        output << R"({
            "HOST_IDS": [
                {
                    "TOPIC_NAME": "rt/DoorUnlockLockIndicator",
                    "CUSTOM_RULE": [
                        {
                            "SIGNAL": "vehicle_cabin_door_row1_left_is_locked",
                            "SIGNAL_RULE": "TRUE",
                            "COMPARE_TOPIC": "rt/DoorSwitch",
                            "COMPARE_SIGNAL": "vehicle_sm_cabin_door_row1_left_is_locked",
                            "COMPARE_SIGNAL_RULE": "TRUE",
                            "RULE_ID": "100",
                            "severity": "not-a-real-severity"
                        }
                    ]
                }
            ],
            "NETWORK_IDS": []
        })";
    }

    const auto runtimePath = "sample_runtime_config.json";

    SecurityClientConfigLoader loader;
    SecurityClientConfig config;

    EXPECT_FALSE(loader.load(runtimePath, policyPath, config));

    std::filesystem::remove(policyPath);
}

}  // namespace
