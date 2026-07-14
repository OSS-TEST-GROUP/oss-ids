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

#include "Main.hpp"

#include "Log.hpp"
#include "Util.hpp"
#include "config_dds_ids.h"
#include "cxxopts.hpp"

#include <csignal>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace AC {
namespace ddsIds {

Main::Main()
{
    programName = "SecurityClient";
    helpString = "DDS security client";
}

bool Main::defineArgs(cxxopts::Options& options)
{
    // clang-format off
    options.add_options()
        ("i,client-id", "Security client identifier", cxxopts::value<std::string>())
        ("d,domain", "DetectionReport domain id", cxxopts::value<int>()->default_value("0"))
        ("c,config", "Policy config file path", cxxopts::value<std::string>()->default_value("policy_config.json"))
        ("r,runtime-config", "Runtime config path", cxxopts::value<std::string>()->default_value("runtime_config.json"));
    // clang-format on
    return true;
}

bool Main::configure(const cxxopts::ParseResult& optRes)
{
    if (optRes.count("client-id") == 0)
    {
        fmt::print(stderr, "Error: --client-id is required\n");
        return false;
    }

    AC::ddsIds::securityClient::AppLaunchOptions options;
    options.clientId = optRes["client-id"].as<std::string>();
    options.detectionDomainId = optRes["domain"].as<int>();
    // if (optRes.count("config") > 0)
    {
        const auto policyPath = optRes["config"].as<std::string>();
        if (!policyPath.empty())
        {
            options.policyConfigPath = policyPath;
        }
    }
    // if (optRes.count("runtime-config") > 0)
    {
        const auto runtimePath = optRes["runtime-config"].as<std::string>();
        if (!runtimePath.empty())
        {
            options.runtimeConfigPath = runtimePath;
        }
    }

    if (!app_.configure(options))
    {
        fmt::print(stderr, "Error: failed to configure SecurityClientApp\n");
        return false;
    }

    return true;
}

int Main::start()
{
    LOG_INF("SecurityClient scaffold started");
    if (!app_.initialize())
    {
        LOG_ERR("SecurityClientApp initialization failed");
        return EXIT_FAILURE;
    }

    return app_.run();
}

int Main::stop()
{
    LOG_INF("Stopping SecurityClient scaffold");
    app_.stop();
    app_.finalize();
    return 0;
}

}  // namespace ddsIds
}  // namespace AC
