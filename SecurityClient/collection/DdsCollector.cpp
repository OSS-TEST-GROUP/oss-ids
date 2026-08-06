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

#include "DdsCollector.hpp"

#include "Log.hpp"

#include <chrono>

namespace {

std::int64_t now_ms()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

const char* bool_text(bool value)
{
    return value ? "true" : "false";
}

constexpr std::int32_t kReaderHistoryDepth = 50;

}  // namespace

namespace AC {
namespace ddsIds {
namespace securityClient {

bool DdsCollector::configure(const SourceConfig& config)
{
    config_ = config;
    if (config.type == "DoorSwitch_")
    {
        sampleType_ = SampleType::door_switch;
    }
    else if (config.type == "DoorUnlockLockIndicator_")
    {
        sampleType_ = SampleType::door_lock_indicator;
    }
    else
    {
        sampleType_ = SampleType::unknown;
        LOG_ERR("DdsCollector does not support type {}", config.type);
        return false;
    }

    LOG_TRA("DdsCollector configured for topic={} type={} transport={} domain={}", config.topic, config.type,
            config.transportId, config.domainId);
    return true;
}

bool DdsCollector::initialize()
{
    if (!createReader())
    {
        LOG_ERR("DdsCollector failed to initialize DDS reader for topic={}", config_.topic);
        return false;
    }

    running_.store(true);
    LOG_INF("DdsCollector initialized for topic={} domain={} type={}", config_.topic, config_.domainId, config_.type);
    return true;
}

bool DdsCollector::poll(std::vector<CollectedSample>& outSamples)
{
    outSamples.clear();

    if (!running_.load() || reader_ == nullptr)
    {
        LOG_WRN("DdsCollector poll requested while inactive for topic={}", config_.topic);
        return false;
    }

    LOG_TRA("DdsCollector polling topic={} source={}", config_.topic, config_.id);

    eprosima::fastdds::dds::SampleInfo info;
    if (sampleType_ == SampleType::door_switch)
    {
        sdv_vss::msg::dds_::DoorSwitch_ sample;
        while (reader_->take_next_sample(&sample, &info) == eprosima::fastdds::dds::RETCODE_OK)
        {
            if (info.valid_data)
            {
                outSamples.push_back(makeDoorSwitchSample(sample));
            }
        }
    }
    else if (sampleType_ == SampleType::door_lock_indicator)
    {
        sdv_vss::msg::dds_::DoorUnlockLockIndicator_ sample;
        while (reader_->take_next_sample(&sample, &info) == eprosima::fastdds::dds::RETCODE_OK)
        {
            if (info.valid_data)
            {
                outSamples.push_back(makeDoorLockSample(sample));
            }
        }
    }

    LOG_DBG("DdsCollector collected {} sample(s) from topic={} source={}", outSamples.size(), config_.topic,
            config_.id);
    return true;
}

void DdsCollector::stop()
{
    running_.store(false);
    LOG_INF("DdsCollector stopped for topic={}", config_.topic);
}

void DdsCollector::finalize()
{
    releaseReader();
    running_.store(false);
    LOG_INF("DdsCollector finalized for topic={}", config_.topic);
}

bool DdsCollector::createReader()
{
    using namespace eprosima::fastdds::dds;

    DomainParticipantQos participantQos = PARTICIPANT_QOS_DEFAULT;
    participantQos.name(config_.id.c_str());
    participant_ =
        DomainParticipantFactory::get_shared_instance()->create_participant(config_.domainId, participantQos);
    if (participant_ == nullptr)
    {
        return false;
    }

    switch (sampleType_)
    {
    case SampleType::door_switch:
        typeSupport_ = TypeSupport(new sdv_vss::msg::dds_::DoorSwitch_PubSubType());
        break;
    case SampleType::door_lock_indicator:
        typeSupport_ = TypeSupport(new sdv_vss::msg::dds_::DoorUnlockLockIndicator_PubSubType());
        break;
    default:
        return false;
    }

    typeSupport_.register_type(participant_);
    subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, StatusMask::none());
    if (subscriber_ == nullptr)
    {
        releaseReader();
        return false;
    }

    topic_ = participant_->create_topic(config_.topic.c_str(), typeSupport_.get_type_name(), TOPIC_QOS_DEFAULT);
    if (topic_ == nullptr)
    {
        releaseReader();
        return false;
    }

    DataReaderQos readerQos = DATAREADER_QOS_DEFAULT;
    readerQos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    readerQos.history().kind = KEEP_LAST_HISTORY_QOS;
    readerQos.history().depth = kReaderHistoryDepth;

    reader_ = subscriber_->create_datareader(topic_, readerQos, nullptr, StatusMask::all());
    if (reader_ == nullptr)
    {
        releaseReader();
        return false;
    }

    return true;
}

void DdsCollector::releaseReader()
{
    if (participant_ != nullptr)
    {
        participant_->delete_contained_entities();
        eprosima::fastdds::dds::DomainParticipantFactory::get_shared_instance()->delete_participant(participant_);
    }

    participant_ = nullptr;
    subscriber_ = nullptr;
    topic_ = nullptr;
    reader_ = nullptr;
}

CollectedSample DdsCollector::makeDoorSwitchSample(const sdv_vss::msg::dds_::DoorSwitch_& sample) const
{
    CollectedSample collected;
    collected.sourceId = config_.id;
    collected.transport = "dds";
    collected.topicName = config_.topic;
    collected.typeName = config_.type;
    collected.timestampMs = now_ms();
    collected.attributes["vehicle_sm_cabin_door_row1_left_is_locked"] =
        bool_text(sample.vehicle_sm_cabin_door_row1_left_is_locked());
    return collected;
}

CollectedSample DdsCollector::makeDoorLockSample(const sdv_vss::msg::dds_::DoorUnlockLockIndicator_& sample) const
{
    CollectedSample collected;
    collected.sourceId = config_.id;
    collected.transport = "dds";
    collected.topicName = config_.topic;
    collected.typeName = config_.type;
    collected.timestampMs = now_ms();
    collected.attributes["vehicle_cabin_door_row1_left_is_locked"] =
        bool_text(sample.vehicle_cabin_door_row1_left_is_locked());
    collected.attributes["vehicle_cabin_door_row2_right_is_locked"] =
        bool_text(sample.vehicle_cabin_door_row2_right_is_locked());
    return collected;
}

}  // namespace securityClient
}  // namespace ddsIds
}  // namespace AC
