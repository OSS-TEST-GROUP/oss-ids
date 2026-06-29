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

#include "../../app/SecurityClientApp.hpp"

#include "Log.hpp"

#include <gtest/gtest.h>

namespace {

using AC::ddsIds::securityClient::AppLaunchOptions;
using AC::ddsIds::securityClient::SecurityClientApp;

TEST(SecurityClientAppTest, ConfigureAndRunUseSampleFixtures)
{
    SecurityClientApp app;

    AppLaunchOptions options;
    options.clientId = "sc-test";
    options.runtimeConfigPath = "SecurityClient/config/sample_runtime_config.json";
    options.policyConfigPath = "SecurityClient/config/sample_policy_config.json";
    options.logLevel = "info";
    options.singlePass = true;

    EXPECT_TRUE(app.configure(options));
    EXPECT_TRUE(app.initialize());
    EXPECT_EQ(EXIT_SUCCESS, app.run());

    app.stop();
    app.finalize();
}

TEST(SecurityClientAppTest, ConfigureAppliesCliOverridesToLoadedConfig)
{
    SecurityClientApp app;

    AppLaunchOptions options;
    options.clientId = "cli-security-client";
    options.policyConfigPath = "policy_rule/config_dds_v1.0.json";
    options.logLevel = "debug";
    options.detectionDomainId = 7;

    ASSERT_TRUE(app.configure(options));

    EXPECT_EQ("cli-security-client", app.config().system.clientId);
    EXPECT_EQ("debug", app.config().system.logLevel);
    ASSERT_EQ(1u, app.config().alertResponse.alertSinks.size());
    EXPECT_EQ(7, app.config().alertResponse.alertSinks.front().domainId);
    EXPECT_EQ("DetectionReport", app.config().alertResponse.alertSinks.front().topic);
    EXPECT_EQ(2u, app.config().policies.signalMismatchRules.size());

    app.finalize();
}

}  // namespace
