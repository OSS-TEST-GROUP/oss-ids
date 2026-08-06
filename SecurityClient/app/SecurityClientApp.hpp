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

#ifndef DDS_IDS_OSS_SECURITYCLIENT_APP_SECURITYCLIENTAPP_HPP
#define DDS_IDS_OSS_SECURITYCLIENT_APP_SECURITYCLIENTAPP_HPP

#include "../alert/AlertResponseStage.hpp"
#include "../alert/DdsDetectionReportSink.hpp"
#include "../alert/NoopResponseHandler.hpp"
#include "../collection/CollectionStage.hpp"
#include "../config/SecurityClientConfig.hpp"
#include "../config/SecurityClientConfigLoader.hpp"
#include "AnalysisDetectionStage.hpp"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace AC {
namespace ddsIds {
namespace securityClient {

struct AppLaunchOptions
{
    std::string clientId;
    std::filesystem::path runtimeConfigPath;
    std::filesystem::path policyConfigPath;
    std::string logLevel;
    std::optional<int> detectionDomainId;
    bool singlePass {false};
};

class SecurityClientApp
{
public:
    bool configure(const AppLaunchOptions& options);
    bool configure(const AppLaunchOptions& options, const SecurityClientConfig& configOverride);
    bool initialize();
    int run();
    void stop();
    void finalize();
    const SecurityClientConfig& config() const;

private:
    void workerLoop();
    void waitForDrain();

    AppLaunchOptions launchOptions_;
    SecurityClientConfig config_;
    SecurityClientConfigLoader configLoader_;
    CollectionStage collectionStage_;
    AnalysisDetectionStage analysisStage_;
    AlertResponseStage alertStage_;
    bool configured_ {false};
    bool initialized_ {false};
    std::atomic_bool running_ {false};

    std::thread worker_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<std::vector<DetectionFinding>> queue_;
    bool is_running {false};
    std::atomic_bool failed_ {false};
};

}  // namespace securityClient
}  // namespace ddsIds
}  // namespace AC

#endif
