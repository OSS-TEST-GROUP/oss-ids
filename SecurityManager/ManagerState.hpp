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

#ifndef __MANAGERSTATE_HPP__
#define __MANAGERSTATE_HPP__

#include "detectionReportPubSubTypes.hpp"
#include "fastdds/dds/core/condition/WaitSet.hpp"
#include "fastdds/dds/publisher/Publisher.hpp"
#include "fastdds/dds/subscriber/DataReader.hpp"
#include "fastdds/dds/topic/TypeSupport.hpp"

#include <array>
#include <atomic>
#include <string>

#ifndef DDS_DOMAIN_DEFAULT
#define DDS_DOMAIN_DEFAULT 0
#endif

using namespace eprosima::fastdds::dds;

namespace AC {
namespace ddsIds {

constexpr int DEFAULT_INPUT_DOMAIN_ID = DDS_DOMAIN_DEFAULT;
constexpr int DEFAULT_OUTPUT_DOMAIN_ID = DDS_DOMAIN_DEFAULT;
constexpr auto DET_TOPIC_PRIMARY = "DetectionReport";
constexpr auto DET_TOPIC_LEGACY = "security/DetectionReport";

struct TopicBridge
{
    std::string in_name;
    std::string out_name;
    bool matched {false};
    Topic* topic_in {nullptr};
    DataReader* reader {nullptr};
    ReadCondition* readcond {nullptr};
    StatusCondition* statuscond {nullptr};
    Topic* topic_out {nullptr};
    DataWriter* writer {nullptr};
    InstanceHandle_t writer_handle;
    bool out_matched {false};
};

class ManagerState
{
public:
    ManagerState();
    ~ManagerState();

    bool config(int input_domain, int output_domain, std::string topic);
    bool run();
    void stop();

private:
    bool createInputEntities();
    bool createOutputEntities();
    void cleanup();
    void bridgeLoop();

    DomainParticipant* input_participant {nullptr};
    DomainParticipant* output_participant {nullptr};
    std::unique_ptr<WaitSet> waitset;
    int input_domain {DEFAULT_INPUT_DOMAIN_ID};
    int output_domain {DEFAULT_OUTPUT_DOMAIN_ID};
    std::string topic_override;
    size_t topic_count {};
    std::array<TopicBridge, 2> topics {};

    TypeSupport detection_type;

    std::atomic_bool g_running {true};
};

}  // namespace ddsIds
}  // namespace AC

#endif  // __MANAGERSTATE_HPP__
