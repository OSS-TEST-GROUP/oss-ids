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

#ifndef DDS_IDS_OSS_SECURITYCLIENT_CONFIG_SECURITYCLIENTCONFIG_HPP
#define DDS_IDS_OSS_SECURITYCLIENT_CONFIG_SECURITYCLIENTCONFIG_HPP

#include "DetectorConfig.hpp"
#include "PolicyConfig.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace AC {
namespace ddsIds {
namespace securityClient {

enum class CacheMode {
    latest,
};

enum class AlertSinkType {
    dds_detection_report,
};

enum class ResponseHandlerType {
    noop,
};

struct SystemConfig
{
    std::string clientId {"security-client-v2"};
    std::string logLevel {"info"};
};

struct TransportConfig
{
    std::string id;
    std::string type {"dds"};
    int domainId {};
};

struct CacheConfig
{
    CacheMode mode {CacheMode::latest};
    std::int64_t ttlMs {};
};

struct SourceConfig
{
    std::string id;
    std::string transportId;
    std::string topic;
    std::string type;
    CacheConfig cache;
    int domainId {};
    TopicStateMachine stateMachine;
};

struct AlertSinkConfig
{
    std::string id;
    AlertSinkType type {AlertSinkType::dds_detection_report};
    bool enabled {true};
    int domainId {};
    std::string topic {"DetectionReport"};
};

struct ResponseHandlerConfig
{
    std::string id;
    ResponseHandlerType type {ResponseHandlerType::noop};
    bool enabled {true};
};

struct CollectionConfig
{
    std::vector<TransportConfig> transports;
    std::vector<SourceConfig> sources;
};

struct AlertResponseConfig
{
    std::vector<AlertSinkConfig> alertSinks;
    std::vector<ResponseHandlerConfig> responseHandlers;
};

struct SecurityClientConfig
{
    SystemConfig system;
    CollectionConfig collection;
    AnalysisDetectionConfig analysisDetection;
    AlertResponseConfig alertResponse;
    PolicyConfig policies;
};

}  // namespace securityClient
}  // namespace ddsIds
}  // namespace AC

#endif
