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

#include "SecurityClientConfigLoader.hpp"

#include "Log.hpp"

#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string_view>

#include <fmt/core.h>

namespace AC {
namespace ddsIds {
namespace securityClient {
namespace {

using json = nlohmann::json;

bool read_json_file(const std::filesystem::path& path, json& outRoot)
{
    std::ifstream input(path);
    if (!input)
    {
        fmt::print(stderr, "[SecurityClientConfigLoader] unable to open {}\n", path.string());
        return false;
    }

    try
    {
        input >> outRoot;
        return true;
    }
    catch (const std::exception& ex)
    {
        fmt::print(stderr, "[SecurityClientConfigLoader] invalid JSON in {}: {}\n", path.string(), ex.what());
        return false;
    }
}

const json* find_object(const json& root, std::initializer_list<std::string_view> names)
{
    for (const auto& name : names)
    {
        if (root.contains(std::string(name)))
        {
            return &root.at(std::string(name));
        }
    }

    return nullptr;
}

const json* find_object(const json& root, std::initializer_list<const char*> names)
{
    for (const auto& name : names)
    {
        if (root.contains(name))
        {
            return &root.at(name);
        }
    }

    return nullptr;
}

std::string read_string(const json& object, std::initializer_list<std::string_view> names, std::string_view fallback)
{
    if (const auto* value = find_object(object, names))
    {
        if (value->is_string())
        {
            return value->get<std::string>();
        }
    }
    return std::string(fallback);
}

int read_int(const json& object, std::initializer_list<std::string_view> names, int fallback)
{
    if (const auto* value = find_object(object, names))
    {
        if (value->is_number_integer())
        {
            return value->get<int>();
        }
        if (value->is_string())
        {
            try
            {
                return std::stoi(value->get<std::string>());
            }
            catch (const std::exception&)
            {
                return fallback;
            }
        }
    }
    return fallback;
}

bool read_bool(const json& object, std::initializer_list<std::string_view> names, bool fallback)
{
    if (const auto* value = find_object(object, names))
    {
        if (value->is_boolean())
        {
            return value->get<bool>();
        }
        if (value->is_string())
        {
            const auto text = value->get<std::string>();
            return text == "true" || text == "TRUE" || text == "1";
        }
    }
    return fallback;
}

bool parse_cache_mode(std::string_view value, CacheMode& outMode)
{
    if (value == "latest" || value == "LATEST")
    {
        outMode = CacheMode::latest;
        return true;
    }
    return false;
}

bool parse_detector_type(std::string_view value, DetectorType& outType)
{
    if (value == "signal_mismatch" || value == "signal-mismatch" || value == "SIGNAL_MISMATCH")
    {
        outType = DetectorType::signal_mismatch;
        return true;
    }
    if (value == "cycle_rule" || value == "cycle-rule" || value == "CYCLE_RULE")
    {
        outType = DetectorType::cycle_rule;
        return true;
    }
    if (value == "rtps_header_rule" || value == "rtps-header-rule" || value == "RTPS_HEADER_RULE")
    {
        outType = DetectorType::rtps_header_rule;
        return true;
    }
    if (value == "state_transition_rule" || value == "state-transition-rule" || value == "STATE_TRANSITION_RULE" ||
        value == "state_transition" || value == "state-transition" || value == "STATE_TRANSITION")
    {
        outType = DetectorType::state_transition_rule;
        return true;
    }
    return false;
}

bool parse_alert_sink_type(std::string_view value, AlertSinkType& outType)
{
    if (value == "dds_detection_report" || value == "dds-detection-report" || value == "DDS_DETECTION_REPORT")
    {
        outType = AlertSinkType::dds_detection_report;
        return true;
    }
    return false;
}

bool parse_response_handler_type(std::string_view value, ResponseHandlerType& outType)
{
    if (value == "noop" || value == "NOOP")
    {
        outType = ResponseHandlerType::noop;
        return true;
    }
    return false;
}

bool parse_severity(std::string_view value, Severity& outSeverity)
{
    if (value == "critical" || value == "CRITICAL")
    {
        outSeverity = Severity::critical;
        return true;
    }
    if (value == "high" || value == "HIGH")
    {
        outSeverity = Severity::high;
        return true;
    }
    if (value == "medium" || value == "MEDIUM")
    {
        outSeverity = Severity::medium;
        return true;
    }
    if (value == "low" || value == "LOW")
    {
        outSeverity = Severity::low;
        return true;
    }
    return false;
}

unsigned int parse_uint(const json& value, unsigned int fallback)
{
    if (value.is_number_integer())
    {
        return static_cast<unsigned int>(value.get<unsigned int>());
    }
    if (value.is_string())
    {
        try
        {
            return static_cast<unsigned int>(std::stoul(value.get<std::string>()));
        }
        catch (const std::exception&)
        {
            return fallback;
        }
    }
    return fallback;
}

unsigned int parse_uint(const json& object, std::initializer_list<std::string_view> names, unsigned int fallback)
{
    if (const auto* value = find_object(object, names))
    {
        return parse_uint(*value, fallback);
    }
    return fallback;
}

std::int64_t parse_int64(const json& value, std::int64_t fallback)
{
    if (value.is_number_integer())
    {
        return value.get<std::int64_t>();
    }
    if (value.is_string())
    {
        try
        {
            return std::stoll(value.get<std::string>());
        }
        catch (const std::exception&)
        {
            return fallback;
        }
    }
    return fallback;
}

std::int64_t parse_int64(const json& object, std::initializer_list<std::string_view> names, std::int64_t fallback)
{
    if (const auto* value = find_object(object, names))
    {
        return parse_int64(*value, fallback);
    }
    return fallback;
}

}  // namespace

bool SecurityClientConfigLoader::loadRuntimeConfig(const std::filesystem::path& path,
                                                   SecurityClientConfig& config) const
{
    json root;
    if (!read_json_file(path, root))
    {
        return false;
    }

    config = {};

    const auto* system = find_object(root, {"system", "SYSTEM"});
    if (system != nullptr)
    {
        config.system.clientId = read_string(*system, {"client_id", "clientId"}, "security-client-v2");
        config.system.logLevel = read_string(*system, {"log_level", "logLevel"}, "info");
    }

    const auto* collection = find_object(root, {"collection", "COLLECTION"});
    if (collection != nullptr)
    {
        if (const auto* transports = find_object(*collection, {"transports", "TRANSPORTS"});
            transports != nullptr && transports->is_array())
        {
            for (const auto& item : *transports)
            {
                TransportConfig transport;
                transport.id = read_string(item, {"id", "ID"}, "");
                transport.type = read_string(item, {"type", "TYPE"}, "dds");
                transport.domainId = read_int(item, {"domain_id", "domainId"}, 0);
                config.collection.transports.push_back(transport);
            }
        }

        if (const auto* sources = find_object(*collection, {"sources", "SOURCES"});
            sources != nullptr && sources->is_array())
        {
            for (const auto& item : *sources)
            {
                SourceConfig source;
                source.id = read_string(item, {"id", "ID"}, "");
                source.transportId = read_string(item, {"transport_id", "transportId"}, "");
                source.topic = read_string(item, {"topic", "TOPIC"}, "");
                source.type = read_string(item, {"type", "TYPE"}, "");
                if (const auto* cache = find_object(item, {"cache", "CACHE"}); cache != nullptr)
                {
                    CacheMode mode {CacheMode::latest};
                    if (!parse_cache_mode(read_string(*cache, {"mode", "MODE"}, "latest"), mode))
                    {
                        LOG_ERR("SecurityClientConfigLoader invalid cache mode in source {}", source.id);
                        return false;
                    }
                    source.cache.mode = mode;
                    source.cache.ttlMs = parse_int64(*cache, {"ttl_ms", "ttlMs"}, 0LL);
                }

                if (const auto* sm = find_object(item, {"state_machine", "stateMachine", "STATE_MACHINE"}); sm != nullptr)
                {
                    source.stateMachine.topic = source.topic;
                    source.stateMachine.signal = read_string(*sm, {"signal", "SIGNAL"}, "");
                    if (const auto* states = find_object(*sm, {"states", "STATES"}); states != nullptr && states->is_object())
                    {
                        for (const auto& [name, val] : states->items())
                        {
                            int code = val.is_number_integer() ? val.get<int>() : 0;
                            source.stateMachine.states.push_back({name, code});
                        }
                    }
                    if (const auto* transitions =
                            find_object(*sm, {"valid_transitions", "validTransitions", "VALID_TRANSITIONS"});
                        transitions != nullptr && transitions->is_array())
                    {
                        for (const auto& trans : *transitions)
                        {
                            int from = read_int(trans, {"from", "FROM"}, -1);
                            int to = read_int(trans, {"to", "TO"}, -1);
                            if (from >= 0 && to >= 0)
                            {
                                source.stateMachine.validTransitions.push_back({from, to});
                            }
                        }
                    }
                }

                if (source.id.empty() || source.topic.empty())
                {
                    LOG_ERR("SecurityClientConfigLoader source entry is missing id/topic");
                    return false;
                }
                config.collection.sources.push_back(source);
            }
        }
    }

    const auto* analysis = find_object(root, {"analysis_detection", "analysisDetection", "ANALYSIS_DETECTION"});
    if (analysis != nullptr)
    {
        if (const auto* detectors = find_object(*analysis, {"detectors", "DETECTORS"});
            detectors != nullptr && detectors->is_array())
        {
            for (const auto& item : *detectors)
            {
                DetectorConfig detector;
                detector.id = read_string(item, {"id", "ID"}, "");
                const auto detectorTypeText = read_string(item, {"type", "TYPE"}, "signal_mismatch");
                DetectorType detectorType {DetectorType::signal_mismatch};
                if (!parse_detector_type(detectorTypeText, detectorType))
                {
                    LOG_ERR("SecurityClientConfigLoader unknown detector type '{}'", detectorTypeText);
                    return false;
                }
                detector.type = detectorType;
                detector.enabled = read_bool(item, {"enabled", "ENABLED"}, true);
                detector.policyRef = read_string(item, {"policy_ref", "policyRef"}, "");
                if (item.contains("inputs") || item.contains("INPUTS"))
                {
                    const auto& inputs = item.contains("inputs") ? item.at("inputs") : item.at("INPUTS");
                    if (inputs.is_array())
                    {
                        for (const auto& input : inputs)
                        {
                            detector.inputs.push_back(input.get<std::string>());
                        }
                    }
                }
                config.analysisDetection.detectors.push_back(detector);
            }
        }
    }

    const auto* alert = find_object(root, {"alert_response", "alertResponse", "ALERT_RESPONSE"});
    if (alert != nullptr)
    {
        if (const auto* sinks = find_object(*alert, {"alert_sinks", "alertSinks", "ALERT_SINKS"});
            sinks != nullptr && sinks->is_array())
        {
            for (const auto& item : *sinks)
            {
                AlertSinkConfig sink;
                sink.id = read_string(item, {"id", "ID"}, "");
                const auto sinkTypeText = read_string(item, {"type", "TYPE"}, "dds_detection_report");
                AlertSinkType sinkType {AlertSinkType::dds_detection_report};
                if (!parse_alert_sink_type(sinkTypeText, sinkType))
                {
                    LOG_ERR("SecurityClientConfigLoader unknown alert sink type '{}'", sinkTypeText);
                    return false;
                }
                sink.type = sinkType;
                sink.enabled = read_bool(item, {"enabled", "ENABLED"}, true);
                sink.domainId = read_int(item, {"domain_id", "domainId"}, 0);
                sink.topic = read_string(item, {"topic", "TOPIC"}, "DetectionReport");
                config.alertResponse.alertSinks.push_back(sink);
            }
        }

        if (const auto* handlers = find_object(*alert, {"response_handlers", "responseHandlers", "RESPONSE_HANDLERS"});
            handlers != nullptr && handlers->is_array())
        {
            for (const auto& item : *handlers)
            {
                ResponseHandlerConfig handler;
                handler.id = read_string(item, {"id", "ID"}, "");
                const auto handlerTypeText = read_string(item, {"type", "TYPE"}, "noop");
                ResponseHandlerType handlerType {ResponseHandlerType::noop};
                if (!parse_response_handler_type(handlerTypeText, handlerType))
                {
                    LOG_ERR("SecurityClientConfigLoader unknown response handler type '{}'", handlerTypeText);
                    return false;
                }
                handler.type = handlerType;
                handler.enabled = read_bool(item, {"enabled", "ENABLED"}, true);
                config.alertResponse.responseHandlers.push_back(handler);
            }
        }
    }

    return true;
}

bool SecurityClientConfigLoader::loadPolicyConfig(const std::filesystem::path& path, PolicyConfig& config) const
{
    json root;
    if (!read_json_file(path, root))
    {
        return false;
    }

    config = {};
    config.version = read_string(root, {"VERSION", "version"}, "1.1");

    const auto* hostIds = find_object(root, {"HOST_IDS", "host_ids"});
    if (hostIds != nullptr && hostIds->is_array())
    {
        for (const auto& topicRule : *hostIds)
        {
            const auto topicName = read_string(topicRule, {"TOPIC_NAME", "topic_name", "topicName"}, "");
            if (topicName.empty())
            {
                continue;
            }

            if (const auto* customRules = find_object(topicRule, {"CUSTOM_RULE", "custom_rule", "customRules"});
                customRules != nullptr && customRules->is_array())
            {
                for (const auto& item : *customRules)
                {
                    SignalMismatchRule rule;
                    rule.ruleId = parse_uint(item, {"RULE_ID", "rule_id"}, 0);
                    rule.enabled = read_bool(item, {"enabled", "ENABLED"}, true);
                    rule.topic = topicName;
                    rule.signal = read_string(item, {"SIGNAL", "signal"}, "");
                    rule.signalRule = read_bool(item, {"SIGNAL_RULE", "signal_rule", "signalRule"}, false);
                    rule.compareTopic = read_string(item, {"COMPARE_TOPIC", "compare_topic", "compareTopic"}, "");
                    rule.compareSignal = read_string(item, {"COMPARE_SIGNAL", "compare_signal", "compareSignal"}, "");
                    rule.compareSignalRule =
                        read_bool(item, {"COMPARE_SIGNAL_RULE", "compare_signal_rule", "compareSignalRule"}, false);
                    const auto severityText = read_string(item, {"severity", "SEVERITY"}, "medium");
                    Severity severity {Severity::medium};
                    if (!parse_severity(severityText, severity))
                    {
                        LOG_ERR("SecurityClientConfigLoader unknown severity '{}'", severityText);
                        return false;
                    }
                    rule.severity = severity;
                    rule.minConsecutive = parse_uint(item, {"MIN_CONSECUTIVE", "min_consecutive", "CONSECUTIVE_COUNT"}, 1);
                    rule.settlingTimeMs = parse_uint(item, {"SETTLING_TIME_MS", "settling_time_ms", "settlingTimeMs"}, 0);
                    config.signalMismatchRules.push_back(rule);
                }
            }

            if (const auto* stateRules =
                    find_object(topicRule, {"STATE_TRANSITION_RULE", "state_transition_rule", "stateTransitionRules"});
                stateRules != nullptr && stateRules->is_array())
            {
                for (const auto& item : *stateRules)
                {
                    StateTransitionRule rule;
                    rule.ruleId = read_string(item, {"RULE_ID", "rule_id", "ruleId"}, "");
                    rule.enabled = read_bool(item, {"enabled", "ENABLED"}, true);
                    rule.topic = topicName;
                    rule.signal = read_string(item, {"SIGNAL", "signal"}, "");
                    rule.description = read_string(item, {"DESCRIPTION", "description"}, "");
                    rule.throttleEnabled =
                        read_bool(item, {"THROTTLE_ENABLED", "throttle_enabled", "throttleEnabled"}, false);
                    rule.throttleMs = parse_int64(item, {"THROTTLE_MS", "throttle_ms", "throttleMs"}, 5000LL);

                    const auto severityText = read_string(item, {"severity", "SEVERITY"}, "high");
                    Severity severity {Severity::high};
                    if (parse_severity(severityText, severity))
                    {
                        rule.severity = severity;
                    }

                    if (const auto* conds = find_object(item, {"CONDITIONS", "conditions"});
                        conds != nullptr && conds->is_array())
                    {
                        for (const auto& citem : *conds)
                        {
                            StateTransitionCondition cond;
                            cond.compareTopic = read_string(citem, {"COMPARE_TOPIC", "compare_topic"}, "");
                            cond.compareSignal = read_string(citem, {"COMPARE_SIGNAL", "compare_signal"}, "");
                            cond.op = read_string(citem, {"OPERATOR", "operator", "op"}, ">");
                            if (citem.contains("VALUE") || citem.contains("value"))
                            {
                                const auto& v = citem.contains("VALUE") ? citem["VALUE"] : citem["value"];
                                if (v.is_number())
                                {
                                    cond.value = v.get<double>();
                                }
                            }
                            rule.conditions.push_back(cond);
                        }
                    }
                    config.stateTransitionRules.push_back(rule);
                }
            }
        }
    }

    const auto* networkIds = find_object(root, {"NETWORK_IDS", "network_ids"});
    if (networkIds != nullptr && networkIds->is_array())
    {
        for (const auto& topicRule : *networkIds)
        {
            const auto topicName = read_string(topicRule, {"TOPIC_NAME", "topic_name", "topicName"}, "");

            if (const auto* cycleRules = find_object(topicRule, {"CYCLE_RULE", "cycle_rule", "cycleRules"});
                cycleRules != nullptr && cycleRules->is_array())
            {
                for (const auto& item : *cycleRules)
                {
                    CycleRule rule;
                    rule.ruleId = parse_uint(item, {"RULE_ID", "rule_id"}, 0);
                    rule.enabled = read_bool(item, {"enabled", "ENABLED"}, true);
                    rule.topic = topicName;
                    rule.lowMs = parse_int64(item, {"LOW", "low"}, 0LL);
                    rule.highMs = parse_int64(item, {"HIGH", "high"}, 0LL);
                    const auto severityText = read_string(item, {"severity", "SEVERITY"}, "medium");
                    Severity severity {Severity::medium};
                    if (!parse_severity(severityText, severity))
                    {
                        LOG_ERR("SecurityClientConfigLoader unknown severity '{}'", severityText);
                        return false;
                    }
                    rule.severity = severity;
                    config.cycleRules.push_back(rule);
                }
            }

            if (const auto* rtpsRules =
                    find_object(topicRule, {"RTPS_HEADER_RULE", "rtps_header_rule", "rtpsHeaderRules"});
                rtpsRules != nullptr && rtpsRules->is_array())
            {
                for (const auto& item : *rtpsRules)
                {
                    RtpsHeaderRule rule;
                    rule.ruleId = parse_uint(item, {"RULE_ID", "rule_id"}, 0);
                    rule.enabled = read_bool(item, {"enabled", "ENABLED"}, true);
                    const auto severityText = read_string(item, {"severity", "SEVERITY"}, "medium");
                    Severity severity {Severity::medium};
                    if (!parse_severity(severityText, severity))
                    {
                        LOG_ERR("SecurityClientConfigLoader unknown severity '{}'", severityText);
                        return false;
                    }
                    rule.severity = severity;

                    if (const auto* vendorIds = find_object(item, {"VENDOR_ID", "vendor_id", "vendorIdAllowlist"});
                        vendorIds != nullptr && vendorIds->is_array())
                    {
                        for (const auto& vendorId : *vendorIds)
                        {
                            if (vendorId.is_object())
                            {
                                for (const auto& [name, value] : vendorId.items())
                                {
                                    (void)name;
                                    rule.vendorIdAllowlist.push_back(static_cast<int>(parse_uint(value, 0)));
                                }
                            }
                            else if (vendorId.is_number_integer())
                            {
                                rule.vendorIdAllowlist.push_back(vendorId.get<int>());
                            }
                        }
                    }

                    config.rtpsHeaderRules.push_back(rule);
                }
            }
        }
    }

    return true;
}

bool SecurityClientConfigLoader::load(const std::filesystem::path& runtimePath, const std::filesystem::path& policyPath,
                                      SecurityClientConfig& config) const
{
    SecurityClientConfig runtimeConfig;
    PolicyConfig policyConfig;

    if (!loadRuntimeConfig(runtimePath, runtimeConfig))
    {
        LOG_ERR("SecurityClientConfigLoader runtime load failed for {}", runtimePath.string());
        return false;
    }

    if (!loadPolicyConfig(policyPath, policyConfig))
    {
        LOG_ERR("SecurityClientConfigLoader policy load failed for {}", policyPath.string());
        return false;
    }

    config = runtimeConfig;
    config.policies = policyConfig;
    LOG_INF("SecurityClientConfigLoader merged {} sink(s) and {} rule(s)",
            runtimeConfig.alertResponse.alertSinks.size(), policyConfig.signalMismatchRules.size());
    for (const auto& src : runtimeConfig.collection.sources)
    {
        if (!src.stateMachine.topic.empty() && !src.stateMachine.validTransitions.empty())
        {
            config.policies.stateMachines.push_back(src.stateMachine);
        }
    }
    LOG_INF("SecurityClientConfigLoader merged {} sink(s), {} mismatch rule(s), {} state rule(s), {} state machine(s)",
            runtimeConfig.alertResponse.alertSinks.size(), policyConfig.signalMismatchRules.size(),
            policyConfig.stateTransitionRules.size(), config.policies.stateMachines.size());
    return true;
}

}  // namespace securityClient
}  // namespace ddsIds
}  // namespace AC
