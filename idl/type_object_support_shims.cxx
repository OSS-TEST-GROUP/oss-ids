#include <fastcdr/xcdr/external.hpp>
#include <fastcdr/xcdr/optional.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/log/Log.hpp>
#include <fastdds/dds/xtypes/common.hpp>
#include <fastdds/dds/xtypes/type_representation/ITypeObjectRegistry.hpp>
#include <fastdds/dds/xtypes/type_representation/TypeObject.hpp>
#include <fastdds/dds/xtypes/type_representation/TypeObjectUtils.hpp>

using namespace eprosima::fastdds::dds::xtypes;

namespace {

bool ensure_string_type(TypeIdentifierPair& type_ids)
{
    auto return_code = eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->type_object_registry().get_type_identifiers(
        "anonymous_string_unbounded", type_ids);
    if (eprosima::fastdds::dds::RETCODE_OK == return_code)
    {
        return true;
    }

    SBound bound = 0;
    StringSTypeDefn string_sdefn = TypeObjectUtils::build_string_s_type_defn(bound);
    return eprosima::fastdds::dds::RETCODE_BAD_PARAMETER !=
           TypeObjectUtils::build_and_register_s_string_type_identifier(string_sdefn, "anonymous_string_unbounded", type_ids);
}

bool add_member(CompleteStructMemberSeq& member_seq, const char* name, MemberId member_id, const TypeIdentifierPair& type_ids)
{
    StructMemberFlag member_flags = TypeObjectUtils::build_struct_member_flag(
        eprosima::fastdds::dds::xtypes::TryConstructFailAction::DISCARD, false, false, false, false);
    bool common_ec {false};
    CommonStructMember common = TypeObjectUtils::build_common_struct_member(
        member_id,
        member_flags,
        TypeObjectUtils::retrieve_complete_type_identifier(type_ids, common_ec));
    if (!common_ec)
    {
        return false;
    }
    eprosima::fastcdr::optional<AppliedBuiltinMemberAnnotations> member_ann_builtin;
    eprosima::fastcdr::optional<AppliedAnnotationSeq> ann_custom;
    CompleteMemberDetail detail = TypeObjectUtils::build_complete_member_detail(name, member_ann_builtin, ann_custom);
    CompleteStructMember member = TypeObjectUtils::build_complete_struct_member(common, detail);
    TypeObjectUtils::add_complete_struct_member(member_seq, member);
    return true;
}

}  // namespace

namespace builtin_interfaces {
namespace msg {

void register_Time_type_identifier(
        eprosima::fastdds::dds::xtypes::TypeIdentifierPair& type_ids)
{
    auto return_code = eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->type_object_registry().get_type_identifiers(
        "builtin_interfaces::msg::Time", type_ids);
    if (eprosima::fastdds::dds::RETCODE_OK == return_code)
    {
        return;
    }

    TypeIdentifierPair int32_ids;
    TypeIdentifierPair uint32_ids;
    if (eprosima::fastdds::dds::RETCODE_OK !=
            eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->type_object_registry().get_type_identifiers(
                "_int32_t", int32_ids) ||
            eprosima::fastdds::dds::RETCODE_OK !=
            eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->type_object_registry().get_type_identifiers(
                "_uint32_t", uint32_ids))
    {
        EPROSIMA_LOG_ERROR(XTYPES_TYPE_REPRESENTATION, "Failed to resolve primitive TypeIdentifiers for builtin_interfaces::msg::Time");
        return;
    }

    StructTypeFlag struct_flags = TypeObjectUtils::build_struct_type_flag(
        eprosima::fastdds::dds::xtypes::ExtensibilityKind::APPENDABLE, false, false);
    QualifiedTypeName type_name = "builtin_interfaces::msg::Time";
    eprosima::fastcdr::optional<AppliedBuiltinTypeAnnotations> type_ann_builtin;
    eprosima::fastcdr::optional<AppliedAnnotationSeq> ann_custom;
    CompleteTypeDetail detail = TypeObjectUtils::build_complete_type_detail(type_ann_builtin, ann_custom, type_name.to_string());
    CompleteStructHeader header = TypeObjectUtils::build_complete_struct_header(TypeIdentifier(), detail);
    CompleteStructMemberSeq member_seq;

    if (!add_member(member_seq, "sec", 0x00000000, int32_ids) ||
            !add_member(member_seq, "nanosec", 0x00000001, uint32_ids))
    {
        EPROSIMA_LOG_ERROR(XTYPES_TYPE_REPRESENTATION, "Failed to build builtin_interfaces::msg::Time TypeObject");
        return;
    }

    CompleteStructType struct_type = TypeObjectUtils::build_complete_struct_type(struct_flags, header, member_seq);
    if (eprosima::fastdds::dds::RETCODE_BAD_PARAMETER ==
            TypeObjectUtils::build_and_register_struct_type_object(struct_type, type_name.to_string(), type_ids))
    {
        EPROSIMA_LOG_ERROR(XTYPES_TYPE_REPRESENTATION, "builtin_interfaces::msg::Time already registered for a different type.");
    }
}

} // namespace msg
} // namespace builtin_interfaces

namespace std_msgs {
namespace msg {

void register_Header_type_identifier(
        eprosima::fastdds::dds::xtypes::TypeIdentifierPair& type_ids)
{
    auto return_code = eprosima::fastdds::dds::DomainParticipantFactory::get_instance()->type_object_registry().get_type_identifiers(
        "std_msgs::msg::Header", type_ids);
    if (eprosima::fastdds::dds::RETCODE_OK == return_code)
    {
        return;
    }

    TypeIdentifierPair time_ids;
    builtin_interfaces::msg::register_Time_type_identifier(time_ids);
    TypeIdentifierPair string_ids;
    if (!ensure_string_type(string_ids))
    {
        EPROSIMA_LOG_ERROR(XTYPES_TYPE_REPRESENTATION, "Failed to resolve string TypeIdentifier for std_msgs::msg::Header");
        return;
    }

    StructTypeFlag struct_flags = TypeObjectUtils::build_struct_type_flag(
        eprosima::fastdds::dds::xtypes::ExtensibilityKind::APPENDABLE, false, false);
    QualifiedTypeName type_name = "std_msgs::msg::Header";
    eprosima::fastcdr::optional<AppliedBuiltinTypeAnnotations> type_ann_builtin;
    eprosima::fastcdr::optional<AppliedAnnotationSeq> ann_custom;
    CompleteTypeDetail detail = TypeObjectUtils::build_complete_type_detail(type_ann_builtin, ann_custom, type_name.to_string());
    CompleteStructHeader header = TypeObjectUtils::build_complete_struct_header(TypeIdentifier(), detail);
    CompleteStructMemberSeq member_seq;

    if (!add_member(member_seq, "stamp", 0x00000000, time_ids) ||
            !add_member(member_seq, "frame_id", 0x00000001, string_ids))
    {
        EPROSIMA_LOG_ERROR(XTYPES_TYPE_REPRESENTATION, "Failed to build std_msgs::msg::Header TypeObject");
        return;
    }

    CompleteStructType struct_type = TypeObjectUtils::build_complete_struct_type(struct_flags, header, member_seq);
    if (eprosima::fastdds::dds::RETCODE_BAD_PARAMETER ==
            TypeObjectUtils::build_and_register_struct_type_object(struct_type, type_name.to_string(), type_ids))
    {
        EPROSIMA_LOG_ERROR(XTYPES_TYPE_REPRESENTATION, "std_msgs::msg::Header already registered for a different type.");
    }
}

} // namespace msg
} // namespace std_msgs
