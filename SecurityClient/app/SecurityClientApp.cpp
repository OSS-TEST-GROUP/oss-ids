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

#include "SecurityClientApp.hpp"

#include "Log.hpp"

#include <chrono>
#include <filesystem>
#include <memory>
#include <thread>
#include <vector>

namespace AC {
namespace ddsIds {
namespace securityClient {

namespace {

std::filesystem::path resolveConfigPath(const std::filesystem::path& candidate)
{
    if (candidate.empty())
    {
        return {};
    }

    const std::filesystem::path repoRoot =
        std::filesystem::path(__FILE__).parent_path().parent_path().parent_path().parent_path();
    const std::vector<std::filesystem::path> searchRoots = {
        repoRoot,
        std::filesystem::current_path(),
        std::filesystem::current_path() / "..",
        std::filesystem::current_path() / "../..",
    };

    for (const auto& root : searchRoots)
    {
        const auto resolved = (root / candidate).lexically_normal();
        if (std::filesystem::exists(resolved))
        {
            return resolved;
        }
    }

    return candidate;
}

void apply_launch_overrides(const AppLaunchOptions& options, SecurityClientConfig& config)
{
    if (!options.clientId.empty())
    {
        config.system.clientId = options.clientId;
    }

    if (!options.logLevel.empty())
    {
        config.system.logLevel = options.logLevel;
    }

    if (!options.detectionDomainId.has_value())
    {
        return;
    }

    for (auto& sink : config.alertResponse.alertSinks)
    {
        if (sink.type == AlertSinkType::dds_detection_report)
        {
            sink.domainId = *options.detectionDomainId;
        }
    }
}

}  // namespace

bool SecurityClientApp::configure(const AppLaunchOptions& options)
{
    launchOptions_ = options;
    LOG_INF("SecurityClientApp configure: runtime={} policy={}", options.runtimeConfigPath.string(),
            options.policyConfigPath.string());

    if (launchOptions_.runtimeConfigPath.empty())
    {
        launchOptions_.runtimeConfigPath = "SecurityClient/config/sample_runtime_config.json";
    }
    if (launchOptions_.policyConfigPath.empty())
    {
        launchOptions_.policyConfigPath = "sample_policy_config.json";
    }

    const auto runtimePath = resolveConfigPath(launchOptions_.runtimeConfigPath);
    const auto policyPath = resolveConfigPath(launchOptions_.policyConfigPath);

    if (!configLoader_.load(runtimePath, policyPath, config_))
    {
        LOG_ERR("SecurityClientApp failed to load runtime/policy configuration from {} / {}", runtimePath.string(),
                policyPath.string());
        return false;
    }

    apply_launch_overrides(launchOptions_, config_);

    LOG_INF("SecurityClientApp configuration loaded from {} / {}", runtimePath.string(), policyPath.string());
    configured_ = true;
    return true;
}

bool SecurityClientApp::configure(const AppLaunchOptions& options, const SecurityClientConfig& configOverride)
{
    launchOptions_ = options;
    config_ = configOverride;
    apply_launch_overrides(launchOptions_, config_);
    configured_ = true;
    LOG_INF("SecurityClientApp configure override with {} detector(s)", config_.analysisDetection.detectors.size());
    return true;
}

bool SecurityClientApp::initialize()
{
    LOG_TRA("SecurityClientApp initialize start");
    if (!configured_)
    {
        return false;
    }

    if (!collectionStage_.configure(config_.collection))
    {
        LOG_ERR("SecurityClientApp collection stage configure failed");
        return false;
    }
    if (!analysisStage_.configure(config_.analysisDetection, config_.policies))
    {
        LOG_ERR("SecurityClientApp analysis stage configure failed");
        return false;
    }
    if (!alertStage_.configure(config_.system, config_.alertResponse))
    {
        LOG_ERR("SecurityClientApp alert stage configure failed");
        return false;
    }

    if (!collectionStage_.initialize())
    {
        LOG_ERR("SecurityClientApp collection stage initialize failed");
        return false;
    }
    if (!analysisStage_.initialize(&collectionStage_.observationStore()))
    {
        LOG_ERR("SecurityClientApp analysis stage initialize failed");
        return false;
    }

    for (const auto& sinkConfig : config_.alertResponse.alertSinks)
    {
        if (!sinkConfig.enabled)
        {
            continue;
        }

        switch (sinkConfig.type)
        {
        case AlertSinkType::dds_detection_report:
            alertStage_.addAlertSink(std::make_unique<DdsDetectionReportSink>());
            break;
        default:
            break;
        }
    }

    for (const auto& handlerConfig : config_.alertResponse.responseHandlers)
    {
        if (!handlerConfig.enabled)
        {
            continue;
        }

        switch (handlerConfig.type)
        {
        case ResponseHandlerType::noop:
            alertStage_.addResponseHandler(std::make_unique<NoopResponseHandler>());
            break;
        default:
            break;
        }
    }

    if (!alertStage_.initialize())
    {
        LOG_ERR("SecurityClientApp alert stage initialize failed");
        return false;
    }

    initialized_ = true;
    LOG_INF("SecurityClientApp initialized successfully");
    return true;
}

int SecurityClientApp::run()
{
    LOG_TRA("SecurityClientApp run start");
    if (!initialized_)
    {
        return EXIT_FAILURE;
    }

    std::vector<CollectedSample> samples;
    if (!collectionStage_.start())
    {
        LOG_ERR("SecurityClientApp collection stage start failed");
        return EXIT_FAILURE;
    }

    running_.store(true);
    std::size_t findingCount = 0;

    while (running_.load())
    {
        samples.clear();
        if (!collectionStage_.pollOnce(samples))
        {
            LOG_ERR("SecurityClientApp collection poll failed");
            return EXIT_FAILURE;
        }

        std::vector<DetectionFinding> findings;
        if (!analysisStage_.analyze(samples, findings))
        {
            LOG_ERR("SecurityClientApp analysis failed");
            return EXIT_FAILURE;
        }

        if (!findings.empty() && !alertStage_.publish(findings))
        {
            LOG_ERR("SecurityClientApp alert publish failed");
            return EXIT_FAILURE;
        }

        findingCount += findings.size();
        if (launchOptions_.singlePass)
        {
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    running_.store(false);
    LOG_INF("SecurityClientApp run completed with {} finding(s)", findingCount);

    return EXIT_SUCCESS;
}

void SecurityClientApp::stop()
{
    running_.store(false);
    collectionStage_.stop();
    analysisStage_.stop();
    alertStage_.stop();
}

void SecurityClientApp::finalize()
{
    collectionStage_.finalize();
    analysisStage_.finalize();
    alertStage_.finalize();

    initialized_ = false;
    configured_ = false;
    running_.store(false);
}

const SecurityClientConfig& SecurityClientApp::config() const
{
    return config_;
}

}  // namespace securityClient
}  // namespace ddsIds
}  // namespace AC
