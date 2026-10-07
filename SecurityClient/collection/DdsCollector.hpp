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

#ifndef DDS_IDS_OSS_SECURITYCLIENT_COLLECTION_DDSCOLLECTOR_HPP
#define DDS_IDS_OSS_SECURITYCLIENT_COLLECTION_DDSCOLLECTOR_HPP

#include "DoorSwitchPubSubTypes.hpp"
#include "DoorUnlockLockIndicatorPubSubTypes.hpp"
#include "autoware_vehicle_msgs/msg/ControlModeReportPubSubTypes.hpp"
#include "autoware_vehicle_msgs/msg/GearReportPubSubTypes.hpp"
#include "autoware_vehicle_msgs/msg/VelocityReportPubSubTypes.hpp"
#include "tier4_api_msgs/msg/DoorStatusPubSubTypes.hpp"
#include "IDataCollector.hpp"
#include "fastdds/dds/domain/DomainParticipant.hpp"
#include "fastdds/dds/domain/DomainParticipantFactory.hpp"
#include "fastdds/dds/publisher/Publisher.hpp"
#include "fastdds/dds/subscriber/DataReader.hpp"
#include "fastdds/dds/subscriber/Subscriber.hpp"
#include "fastdds/dds/topic/Topic.hpp"
#include "fastdds/dds/topic/TypeSupport.hpp"

#include <atomic>

namespace AC {
namespace ddsIds {
namespace securityClient {

class DdsCollector : public IDataCollector
{
public:
    bool configure(const SourceConfig& config) override;
    bool initialize() override;
    bool poll(std::vector<CollectedSample>& outSamples) override;
    void stop() override;
    void finalize() override;

private:
    enum class SampleType {
        unknown,
        door_switch,
        door_lock_indicator,
        door_status,
        gear_report,
        control_mode_report,
        velocity_report,
    };

    bool createReader();
    void releaseReader();
    CollectedSample makeDoorSwitchSample(const sdv_vss::msg::dds_::DoorSwitch_& sample) const;
    CollectedSample makeDoorLockSample(const sdv_vss::msg::dds_::DoorUnlockLockIndicator_& sample) const;
    CollectedSample makeDoorStatusSample(const tier4_api_msgs::msg::DoorStatus& sample) const;
    CollectedSample makeGearReportSample(const autoware_vehicle_msgs::msg::GearReport& sample) const;
    CollectedSample makeControlModeSample(const autoware_vehicle_msgs::msg::ControlModeReport& sample) const;
    CollectedSample makeVelocityReportSample(const autoware_vehicle_msgs::msg::VelocityReport& sample) const;

    SourceConfig config_;
    SampleType sampleType_ {SampleType::unknown};
    eprosima::fastdds::dds::DomainParticipant* participant_ {nullptr};
    eprosima::fastdds::dds::Subscriber* subscriber_ {nullptr};
    eprosima::fastdds::dds::Topic* topic_ {nullptr};
    eprosima::fastdds::dds::DataReader* reader_ {nullptr};
    eprosima::fastdds::dds::TypeSupport typeSupport_;
    std::atomic_bool running_ {false};
    int pollCount_ {0};
};

}  // namespace securityClient
}  // namespace ddsIds
}  // namespace AC

#endif
