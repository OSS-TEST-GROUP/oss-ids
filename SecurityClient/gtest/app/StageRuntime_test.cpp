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

#include "../../app/StageRuntime.hpp"

#include "Log.hpp"
#include "config_dds_ids.h"

#include <filesystem>

#include <gtest/gtest.h>

namespace {

using AC::ddsIds::securityClient::AppLaunchOptions;
using AC::ddsIds::securityClient::SecurityClientConfig;
using AC::ddsIds::securityClient::SecurityClientConfigLoader;
using AC::ddsIds::securityClient::StageRuntime;

TEST(StageRuntimeTest, ConfigureAndRunUseSampleFixtures)
{
    StageRuntime runtime;
    SecurityClientConfigLoader loader;
    SecurityClientConfig config;
    AppLaunchOptions options;

    std::filesystem::path policyRulePath = POLICY_RULE_DIR;
    const auto runtimeConfigPath = policyRulePath / "sample_runtime_config.json";
    const auto policyConfigPath = policyRulePath / "sample_policy_config.json";

    options.clientId = "sc-stage-runtime";
    options.runtimeConfigPath = runtimeConfigPath;
    options.policyConfigPath = policyConfigPath;
    options.singlePass = true;

    EXPECT_TRUE(loader.load(runtimeConfigPath, policyConfigPath, config));
    EXPECT_TRUE(runtime.configure(config, options));
    EXPECT_TRUE(runtime.initialize());
    EXPECT_EQ(EXIT_SUCCESS, runtime.run());

    runtime.stop();
    runtime.finalize();
}

}  // namespace
