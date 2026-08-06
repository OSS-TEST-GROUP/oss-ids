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

#include "ManagerState.hpp"

#include "Log.hpp"
#include "fastdds/dds/domain/DomainParticipant.hpp"
#include "fastdds/dds/domain/DomainParticipantFactory.hpp"
#include "fastdds/dds/publisher/DataWriter.hpp"
#include "fastdds/dds/publisher/DataWriterListener.hpp"
#include "fastdds/dds/subscriber/Subscriber.hpp"

#include <memory>

namespace AC {
namespace ddsIds {

ManagerState::ManagerState()
{
    detection_type = TypeSupport(new security::DetectionReportPubSubType());
}

ManagerState::~ManagerState()
{
    cleanup();
}

bool ManagerState::config(int input_domain, int output_domain, std::string topic)
{
    this->input_domain = input_domain;
    this->output_domain = output_domain;
    this->topic_override = topic;
    this->topic_count = 1;
    this->topics[0].in_name = this->topic_override.empty() ? DET_TOPIC_PRIMARY : this->topic_override;
    this->topics[0].out_name = DET_TOPIC_LEGACY;
    return true;
}

bool ManagerState::run()
{
    if (!createInputEntities())
    {
        cleanup();
        return false;
    }

    if (!createOutputEntities())
    {
        cleanup();
        return false;
    }

    bridgeLoop();
    LOG_INF("[SecurityManager] shutting down\n");
    cleanup();

    // return true when shutdown was requested (graceful stop), false on unexpected failure
    return !g_running.load();
}

void ManagerState::stop()
{
    g_running.store(false);
}

bool ManagerState::createInputEntities()
{
    DomainParticipantQos pqos = PARTICIPANT_QOS_DEFAULT;
    pqos.name("SecurityManagerIn");
    input_participant = DomainParticipantFactory::get_shared_instance()->create_participant(input_domain, pqos);
    if (input_participant == nullptr)
    {
        LOG_ERR("create_participant(source={}) failed\n", input_domain);
        return false;
    }

    detection_type.register_type(input_participant);

    Subscriber* subscriber = input_participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr, StatusMask::none());
    if (subscriber == nullptr)
    {
        LOG_ERR("create_subscriber failed\n");
        return false;
    }

    waitset = std::make_unique<WaitSet>();

    DataReaderQos rqos = DATAREADER_QOS_DEFAULT;
    rqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    rqos.history().kind = KEEP_LAST_HISTORY_QOS;
    rqos.history().depth = 1;

    for (size_t index = 0; index < topic_count; ++index)
    {
        auto& topic = topics[index];
        topic.topic_in =
            input_participant->create_topic(topic.in_name.c_str(), detection_type.get_type_name(), TOPIC_QOS_DEFAULT);
        if (topic.topic_in == nullptr)
        {
            LOG_ERR("create_topic({}, domain={}) failed\n", topic.in_name, input_domain);
            return false;
        }

        topic.reader = subscriber->create_datareader(topic.topic_in, rqos, nullptr, StatusMask::all());
        if (topic.reader == nullptr)
        {
            LOG_ERR("create_reader({}) failed\n", topic.in_name);
            return false;
        }

        topic.readcond = topic.reader->create_readcondition(ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
        if (topic.readcond == nullptr)
        {
            LOG_ERR("create_readcondition({}) failed\n", topic.in_name);
            return false;
        }
        waitset->attach_condition(*topic.readcond);

        topic.statuscond = &topic.reader->get_statuscondition();
        topic.statuscond->set_enabled_statuses(StatusMask::subscription_matched());
        waitset->attach_condition(*topic.statuscond);
    }

    return true;
}
bool ManagerState::createOutputEntities()
{
    DomainParticipantQos pqos = PARTICIPANT_QOS_DEFAULT;
    pqos.name("SecurityManagerOut");
    output_participant = DomainParticipantFactory::get_shared_instance()->create_participant(output_domain, pqos);
    if (output_participant == nullptr)
    {
        LOG_ERR("create_participant(dest={}) failed\n", output_domain);
        return false;
    }

    detection_type.register_type(output_participant);

    auto* publisher = output_participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr, StatusMask::none());
    if (publisher == nullptr)
    {
        LOG_ERR("create_publisher failed\n");
        return false;
    }

    DataWriterQos wqos = DATAWRITER_QOS_DEFAULT;
    wqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    wqos.history().kind = KEEP_LAST_HISTORY_QOS;
    wqos.history().depth = 1;

    for (size_t index = 0; index < topic_count; ++index)
    {
        auto& topic = topics[index];
        topic.topic_out =
            output_participant->create_topic(topic.out_name.c_str(), detection_type.get_type_name(), TOPIC_QOS_DEFAULT);
        if (topic.topic_out == nullptr)
        {
            LOG_ERR("create_topic({}, domain={}) failed\n", topic.out_name, output_domain);
            return false;
        }

        topic.writer = publisher->create_datawriter(topic.topic_out, wqos, nullptr, StatusMask::all());
        if (topic.writer == nullptr)
        {
            LOG_ERR("create_writer({}) failed\n", topic.out_name);
            return false;
        }

        topic.writer_handle = topic.writer->get_instance_handle();
        topic.out_matched = false;
    }
    return true;
}
void ManagerState::cleanup()
{
    if (waitset != nullptr)
    {
        for (size_t index = 0; index < topic_count; ++index)
        {
            auto& topic = topics[index];
            if (topic.readcond != nullptr)
            {
                waitset->detach_condition(*topic.readcond);
            }
            if (topic.statuscond != nullptr)
            {
                waitset->detach_condition(*topic.statuscond);
            }
        }
    }
    waitset.reset();

    if (input_participant != nullptr)
    {
        input_participant->delete_contained_entities();
        DomainParticipantFactory::get_shared_instance()->delete_participant(input_participant);
        input_participant = nullptr;
    }
    if (output_participant != nullptr)
    {
        output_participant->delete_contained_entities();
        DomainParticipantFactory::get_shared_instance()->delete_participant(output_participant);
        output_participant = nullptr;
    }
}

void ManagerState::bridgeLoop()
{
    LOG_INF("[SecurityManager] bridging DetectionReport on domain {} -> {}\n", input_domain, output_domain);

    ConditionSeq active;
    Duration_t timeout {0, 500000000};

    while (g_running.load())
    {
        active.clear();
        if (waitset->wait(active, timeout) != RETCODE_OK)
        {
            continue;
        }
        if (active.empty())
        {
            continue;
        }

        for (auto* condition : active)
        {
            for (size_t index = 0; index < topic_count; ++index)
            {
                auto& topic = topics[index];
                if (topic.writer != nullptr && !topic.out_matched)
                {
                    PublicationMatchedStatus publication_status {};
                    if (topic.writer->get_publication_matched_status(publication_status) == RETCODE_OK &&
                        publication_status.current_count > 0)
                    {
                        LOG_INF("[SecurityManager] writer matched: topic={} current={} total={}\n", topic.out_name,
                                publication_status.current_count, publication_status.total_count);
                        topic.out_matched = true;
                    }
                }

                if (topic.readcond == condition)
                {
                    security::DetectionReport sample;
                    SampleInfo info;
                    while (topic.reader->take_next_sample(&sample, &info) == RETCODE_OK)
                    {
                        if (!info.valid_data)
                        {
                            continue;
                        }

                        if (topic.writer_handle != InstanceHandle_t() && info.publication_handle == topic.writer_handle)
                        {
                            continue;
                        }

                        LOG_INF("[MANAGER RECV] ({}) client_id={} topic={} rule={} ts_ms={}\n", topic.in_name,
                                sample.client_id(), sample.dds_topic(), sample.rule(), sample.timestamp_ms());

                        if (topic.writer->write(&sample) != RETCODE_OK)
                        {
                            LOG_ERR("[ERROR] write({} -> domain {}) failed\n", topic.out_name, output_domain);
                        }
                        else
                        {
                            LOG_INF("[MANAGER FWD ] ({} -> domain {}) client_id={} topic={} rule={}\n", topic.out_name,
                                    output_domain, sample.client_id(), sample.dds_topic(), sample.rule());
                        }
                    }
                }
                else if (topic.statuscond == condition)
                {
                    SubscriptionMatchedStatus status {};
                    if (topic.reader->get_subscription_matched_status(status) == RETCODE_OK)
                    {
                        LOG_INF("[SecurityManager] subscription status: topic={} current={} total={}\n", topic.in_name,
                                status.current_count, status.total_count);
                    }
                }
            }
        }
    }
}

}  // namespace ddsIds
}  // namespace AC
