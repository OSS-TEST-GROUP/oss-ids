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

#include "DdsDetectionReportSink.hpp"

#include "Log.hpp"

#include <chrono>
#include <thread>

#include <fmt/core.h>

namespace AC {
namespace ddsIds {
namespace securityClient {

namespace {

constexpr unsigned int kPrimaryWriter = 1u << 0;
constexpr auto kMatchPollInterval = std::chrono::milliseconds(100);
constexpr auto kMatchTimeout = std::chrono::seconds(5);

std::int64_t nowMs()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

}  // namespace

bool DdsDetectionReportSink::configure(const AlertSinkConfig& config, const SystemConfig& systemConfig)
{
    config_ = config;
    systemConfig_ = systemConfig;
    LOG_TRA("DdsDetectionReportSink configured for topic={} domain={}", config.topic, config.domainId);
    return true;
}

bool DdsDetectionReportSink::initialize()
{
    if (!config_.enabled)
    {
        LOG_INF("DdsDetectionReportSink disabled; skipping initialization");
        return true;
    }

    participant_ = eprosima::fastdds::dds::DomainParticipantFactory::get_shared_instance()->create_participant(
        config_.domainId, eprosima::fastdds::dds::PARTICIPANT_QOS_DEFAULT);
    if (participant_ == nullptr)
    {
        LOG_ERR("DdsDetectionReportSink failed to create participant for domain {}", config_.domainId);
        return false;
    }

    typeSupport_ = eprosima::fastdds::dds::TypeSupport(new security::DetectionReportPubSubType());
    typeSupport_.register_type(participant_);

    publisher_ = participant_->create_publisher(eprosima::fastdds::dds::PUBLISHER_QOS_DEFAULT, nullptr,
                                                eprosima::fastdds::dds::StatusMask::none());
    if (publisher_ == nullptr)
    {
        LOG_ERR("DdsDetectionReportSink failed to create publisher");
        releaseWriter();
        return false;
    }

    eprosima::fastdds::dds::DataWriterQos wqos = eprosima::fastdds::dds::DATAWRITER_QOS_DEFAULT;
    wqos.reliability().kind = eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS;
    wqos.history().kind = eprosima::fastdds::dds::KEEP_LAST_HISTORY_QOS;
    wqos.history().depth = 1;

    topic_ = participant_->create_topic(config_.topic.c_str(), typeSupport_.get_type_name(),
                                        eprosima::fastdds::dds::TOPIC_QOS_DEFAULT);
    if (topic_ == nullptr)
    {
        LOG_ERR("DdsDetectionReportSink failed to create topic {}", config_.topic);
        releaseWriter();
        return false;
    }

    writer_ = publisher_->create_datawriter(topic_, wqos, nullptr, eprosima::fastdds::dds::StatusMask::all());
    if (writer_ == nullptr)
    {
        LOG_ERR("DdsDetectionReportSink failed to create writer");
        releaseWriter();
        return false;
    }

    writerMask_ = currentWriterMask();
    initialized_.store(true);
    LOG_INF("DdsDetectionReportSink initialized on topic {} with matched_subscribers={}", config_.topic,
            writerMask_ != 0);
    return true;
}

bool DdsDetectionReportSink::publish(const std::vector<DetectionFinding>& findings)
{
    LOG_TRA("DdsDetectionReportSink publish requested for {} finding(s) on topic {}", findings.size(), config_.topic);
    if (findings.empty())
    {
        return true;
    }

    if (!initialized_.load() || !config_.enabled || writer_ == nullptr)
    {
        LOG_DBG("DdsDetectionReportSink publish skipped: initialized={} enabled={} writer={}", initialized_.load(),
                config_.enabled, writer_ != nullptr);
        return true;
    }

    writerMask_ = currentWriterMask();
    if (writerMask_ == 0 && !waitForMatch())
    {
        LOG_ERR("DdsDetectionReportSink publish aborted: no matched subscriber for topic {}", config_.topic);
        return false;
    }

    for (const auto& finding : findings)
    {
        security::DetectionReport report;
        report.client_id(systemConfig_.clientId);
        report.dds_topic(finding.topicName);
        report.rule(finding.summary);
        report.timestamp_ms(nowMs());

        LOG_DBG("DdsDetectionReportSink writing DetectionReport for detector={} topic={} summary={}",
                finding.detectorId, finding.topicName, finding.summary);
        if (writer_->write(&report) != eprosima::fastdds::dds::RETCODE_OK)
        {
            LOG_ERR("DdsDetectionReportSink write failed for detector {}", finding.detectorId);
            return false;
        }

        LOG_TRA("DdsDetectionReportSink emitted DetectionReport for detector={} topic={}", finding.detectorId,
                finding.topicName);
    }

    return true;
}

void DdsDetectionReportSink::stop()
{
    initialized_.store(false);
    writerMask_ = 0;
}

void DdsDetectionReportSink::finalize()
{
    releaseWriter();
    initialized_.store(false);
    writerMask_ = 0;
}

bool DdsDetectionReportSink::createWriter()
{
    return true;
}

unsigned int DdsDetectionReportSink::currentWriterMask() const
{
    if (writer_ == nullptr)
    {
        return 0;
    }

    eprosima::fastdds::dds::PublicationMatchedStatus status {};
    if (writer_->get_publication_matched_status(status) == eprosima::fastdds::dds::RETCODE_OK &&
        status.current_count > 0)
    {
        return kPrimaryWriter;
    }

    return 0;
}

bool DdsDetectionReportSink::waitForMatch()
{
    if (writer_ == nullptr)
    {
        return false;
    }

    LOG_INF("DdsDetectionReportSink waiting for subscriber match on topic {}", config_.topic);
    const auto deadline = std::chrono::steady_clock::now() + kMatchTimeout;
    while (initialized_.load() && std::chrono::steady_clock::now() < deadline)
    {
        writerMask_ = currentWriterMask();
        if (writerMask_ != 0)
        {
            LOG_INF("DdsDetectionReportSink matched subscriber on topic {}", config_.topic);
            return true;
        }

        std::this_thread::sleep_for(kMatchPollInterval);
    }

    writerMask_ = currentWriterMask();
    return writerMask_ != 0;
}

void DdsDetectionReportSink::releaseWriter()
{
    if (publisher_ != nullptr)
    {
        publisher_->delete_datawriter(writer_);
        participant_->delete_publisher(publisher_);
        publisher_ = nullptr;
        writer_ = nullptr;
    }

    if (topic_ != nullptr)
    {
        participant_->delete_topic(topic_);
        topic_ = nullptr;
    }

    if (participant_ != nullptr)
    {
        participant_->delete_contained_entities();
        eprosima::fastdds::dds::DomainParticipantFactory::get_shared_instance()->delete_participant(participant_);
        participant_ = nullptr;
    }
}

}  // namespace securityClient
}  // namespace ddsIds
}  // namespace AC
