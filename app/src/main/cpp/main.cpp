/*
 * OpenXR Controller Demo
 *
 * A controller and hand-tracking diagnostic scene built on top of PICO's
 * sample framework so the runtime, swapchains, controller bindings, GUI
 * interaction, and rendering path match the official samples.
 */

#include "AndroidOpenXrProgram.h"
#include "CartesianBranch.h"
#include "Cube.h"
#include "GuiPlane.h"
#include "GuiWindow.h"
#include "TruncatedCone.h"
#include "xr_linear.h"

#include <android_native_app_glue.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <ifaddrs.h>
#include <memory>
#include <mutex>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

using namespace PVRSampleFW;

namespace {
    constexpr XrVector3f kControllerBaseScale{0.055f, 0.055f, 0.055f};
    constexpr XrVector3f kHandPalmScale{0.045f, 0.028f, 0.045f};
    constexpr XrVector3f kFingerTipScale{0.020f, 0.020f, 0.020f};
    constexpr float kJointRadiusFloor = 0.0065f;
    constexpr size_t kHandBoneCount = 25;
    constexpr size_t kFingerCount = 5;

    constexpr uint32_t kButtonA = 1u << 0;
    constexpr uint32_t kButtonB = 1u << 1;
    constexpr uint32_t kButtonX = 1u << 2;
    constexpr uint32_t kButtonY = 1u << 3;
    constexpr uint32_t kButtonMenu = 1u << 4;
    constexpr uint32_t kButtonHome = 1u << 5;
    constexpr uint32_t kButtonGripTriggerLeft = 1u << 6;
    constexpr uint32_t kButtonGripTriggerRight = 1u << 7;
    constexpr uint32_t kButtonTriggerLeft = 1u << 8;
    constexpr uint32_t kButtonTriggerRight = 1u << 9;
    constexpr uint32_t kButtonJoystickLeft = 1u << 10;
    constexpr uint32_t kButtonJoystickRight = 1u << 11;
    constexpr uint32_t kButtonBackLeft = 1u << 12;
    constexpr uint32_t kButtonBackRight = 1u << 13;
    constexpr uint32_t kButtonTouchpadLeft = 1u << 14;
    constexpr uint32_t kButtonTouchpadRight = 1u << 15;
    constexpr uint32_t kButtonSideLeft = 1u << 16;
    constexpr uint32_t kButtonSideRight = 1u << 17;

    constexpr uint32_t kTouchJoystickLeft = 1u << 0;
    constexpr uint32_t kTouchJoystickRight = 1u << 1;
    constexpr uint32_t kTouchTriggerLeft = 1u << 2;
    constexpr uint32_t kTouchTriggerRight = 1u << 3;
    constexpr uint32_t kTouchThumbRestLeft = 1u << 4;
    constexpr uint32_t kTouchThumbRestRight = 1u << 5;
    constexpr uint32_t kTouchRockerLeft = 1u << 6;
    constexpr uint32_t kTouchRockerRight = 1u << 7;
    constexpr uint32_t kTouchX = 1u << 8;
    constexpr uint32_t kTouchY = 1u << 9;
    constexpr uint32_t kTouchA = 1u << 10;
    constexpr uint32_t kTouchB = 1u << 11;

    struct HandBoneConnection {
        XrHandJointEXT from;
        XrHandJointEXT to;
        float thickness;
    };

    struct FingerAngleChain {
        const char* name;
        std::array<XrHandJointEXT, 5> joints;
        std::array<const char*, 3> labels;
    };

    constexpr std::array<HandBoneConnection, kHandBoneCount> kHandBoneConnections{{
            {XR_HAND_JOINT_WRIST_EXT, XR_HAND_JOINT_PALM_EXT, 0.020f},
            {XR_HAND_JOINT_PALM_EXT, XR_HAND_JOINT_THUMB_METACARPAL_EXT, 0.015f},
            {XR_HAND_JOINT_THUMB_METACARPAL_EXT, XR_HAND_JOINT_THUMB_PROXIMAL_EXT, 0.013f},
            {XR_HAND_JOINT_THUMB_PROXIMAL_EXT, XR_HAND_JOINT_THUMB_DISTAL_EXT, 0.012f},
            {XR_HAND_JOINT_THUMB_DISTAL_EXT, XR_HAND_JOINT_THUMB_TIP_EXT, 0.010f},
            {XR_HAND_JOINT_PALM_EXT, XR_HAND_JOINT_INDEX_METACARPAL_EXT, 0.014f},
            {XR_HAND_JOINT_INDEX_METACARPAL_EXT, XR_HAND_JOINT_INDEX_PROXIMAL_EXT, 0.013f},
            {XR_HAND_JOINT_INDEX_PROXIMAL_EXT, XR_HAND_JOINT_INDEX_INTERMEDIATE_EXT, 0.011f},
            {XR_HAND_JOINT_INDEX_INTERMEDIATE_EXT, XR_HAND_JOINT_INDEX_DISTAL_EXT, 0.010f},
            {XR_HAND_JOINT_INDEX_DISTAL_EXT, XR_HAND_JOINT_INDEX_TIP_EXT, 0.009f},
            {XR_HAND_JOINT_PALM_EXT, XR_HAND_JOINT_MIDDLE_METACARPAL_EXT, 0.015f},
            {XR_HAND_JOINT_MIDDLE_METACARPAL_EXT, XR_HAND_JOINT_MIDDLE_PROXIMAL_EXT, 0.014f},
            {XR_HAND_JOINT_MIDDLE_PROXIMAL_EXT, XR_HAND_JOINT_MIDDLE_INTERMEDIATE_EXT, 0.012f},
            {XR_HAND_JOINT_MIDDLE_INTERMEDIATE_EXT, XR_HAND_JOINT_MIDDLE_DISTAL_EXT, 0.011f},
            {XR_HAND_JOINT_MIDDLE_DISTAL_EXT, XR_HAND_JOINT_MIDDLE_TIP_EXT, 0.009f},
            {XR_HAND_JOINT_PALM_EXT, XR_HAND_JOINT_RING_METACARPAL_EXT, 0.014f},
            {XR_HAND_JOINT_RING_METACARPAL_EXT, XR_HAND_JOINT_RING_PROXIMAL_EXT, 0.013f},
            {XR_HAND_JOINT_RING_PROXIMAL_EXT, XR_HAND_JOINT_RING_INTERMEDIATE_EXT, 0.011f},
            {XR_HAND_JOINT_RING_INTERMEDIATE_EXT, XR_HAND_JOINT_RING_DISTAL_EXT, 0.010f},
            {XR_HAND_JOINT_RING_DISTAL_EXT, XR_HAND_JOINT_RING_TIP_EXT, 0.009f},
            {XR_HAND_JOINT_PALM_EXT, XR_HAND_JOINT_LITTLE_METACARPAL_EXT, 0.013f},
            {XR_HAND_JOINT_LITTLE_METACARPAL_EXT, XR_HAND_JOINT_LITTLE_PROXIMAL_EXT, 0.012f},
            {XR_HAND_JOINT_LITTLE_PROXIMAL_EXT, XR_HAND_JOINT_LITTLE_INTERMEDIATE_EXT, 0.010f},
            {XR_HAND_JOINT_LITTLE_INTERMEDIATE_EXT, XR_HAND_JOINT_LITTLE_DISTAL_EXT, 0.009f},
            {XR_HAND_JOINT_LITTLE_DISTAL_EXT, XR_HAND_JOINT_LITTLE_TIP_EXT, 0.008f},
    }};

    constexpr std::array<FingerAngleChain, kFingerCount> kFingerAngleChains{{
            {"Thumb",
             {XR_HAND_JOINT_PALM_EXT, XR_HAND_JOINT_THUMB_METACARPAL_EXT, XR_HAND_JOINT_THUMB_PROXIMAL_EXT,
              XR_HAND_JOINT_THUMB_DISTAL_EXT, XR_HAND_JOINT_THUMB_TIP_EXT},
             {"CMC", "MCP", "IP"}},
            {"Index",
             {XR_HAND_JOINT_INDEX_METACARPAL_EXT, XR_HAND_JOINT_INDEX_PROXIMAL_EXT,
              XR_HAND_JOINT_INDEX_INTERMEDIATE_EXT, XR_HAND_JOINT_INDEX_DISTAL_EXT, XR_HAND_JOINT_INDEX_TIP_EXT},
             {"MCP", "PIP", "DIP"}},
            {"Middle",
             {XR_HAND_JOINT_MIDDLE_METACARPAL_EXT, XR_HAND_JOINT_MIDDLE_PROXIMAL_EXT,
              XR_HAND_JOINT_MIDDLE_INTERMEDIATE_EXT, XR_HAND_JOINT_MIDDLE_DISTAL_EXT, XR_HAND_JOINT_MIDDLE_TIP_EXT},
             {"MCP", "PIP", "DIP"}},
            {"Ring",
             {XR_HAND_JOINT_RING_METACARPAL_EXT, XR_HAND_JOINT_RING_PROXIMAL_EXT,
              XR_HAND_JOINT_RING_INTERMEDIATE_EXT, XR_HAND_JOINT_RING_DISTAL_EXT, XR_HAND_JOINT_RING_TIP_EXT},
             {"MCP", "PIP", "DIP"}},
            {"Little",
             {XR_HAND_JOINT_LITTLE_METACARPAL_EXT, XR_HAND_JOINT_LITTLE_PROXIMAL_EXT,
              XR_HAND_JOINT_LITTLE_INTERMEDIATE_EXT, XR_HAND_JOINT_LITTLE_DISTAL_EXT, XR_HAND_JOINT_LITTLE_TIP_EXT},
             {"MCP", "PIP", "DIP"}},
    }};

    XrPosef MakePose(float x, float y, float z) {
        XrPosef pose{};
        pose.orientation.w = 1.0f;
        pose.position = {x, y, z};
        return pose;
    }

    XrVector3f MakeUniformScale(float scale) {
        return {scale, scale, scale};
    }

    bool IsPoseValid(const XrHandJointLocationEXT& joint) {
        const XrSpaceLocationFlags required =
                XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
        return (joint.locationFlags & required) == required;
    }

    XrVector3f Midpoint(const XrVector3f& a, const XrVector3f& b) {
        return {(a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f, (a.z + b.z) * 0.5f};
    }

    XrQuaternionf QuaternionFromTo(const XrVector3f& from, const XrVector3f& to) {
        XrVector3f v0 = from;
        XrVector3f v1 = to;
        XrVector3f_Normalize(&v0);
        XrVector3f_Normalize(&v1);

        const float dot = std::clamp(XrVector3f_Dot(&v0, &v1), -1.0f, 1.0f);
        if (dot > 0.9999f) {
            XrQuaternionf result{};
            XrQuaternionf_CreateIdentity(&result);
            return result;
        }

        if (dot < -0.9999f) {
            XrVector3f axis{1.0f, 0.0f, 0.0f};
            XrVector3f_Cross(&axis, &v0, &axis);
            if (XrVector3f_Length(&axis) < 0.0001f) {
                axis = {0.0f, 1.0f, 0.0f};
                XrVector3f_Cross(&axis, &v0, &axis);
            }
            XrQuaternionf result{};
            XrQuaternionf_CreateFromAxisAngle(&result, &axis, MATH_PI);
            XrQuaternionf_Normalize(&result);
            return result;
        }

        XrVector3f axis{};
        XrVector3f_Cross(&axis, &v0, &v1);
        const float s = std::sqrt((1.0f + dot) * 2.0f);
        const float invs = 1.0f / s;
        XrQuaternionf result{};
        result.x = axis.x * invs;
        result.y = axis.y * invs;
        result.z = axis.z * invs;
        result.w = s * 0.5f;
        XrQuaternionf_Normalize(&result);
        return result;
    }

    XrPosef MakeBonePose(const XrVector3f& start, const XrVector3f& end) {
        XrVector3f direction{end.x - start.x, end.y - start.y, end.z - start.z};
        XrVector3f_Normalize(&direction);

        XrPosef pose{};
        pose.position = Midpoint(start, end);
        pose.orientation = QuaternionFromTo(XrVector3f{0.0f, 0.0f, 1.0f}, direction);
        return pose;
    }

    float AngleBetweenSegmentsDegrees(const XrVector3f& a, const XrVector3f& b, const XrVector3f& c) {
        XrVector3f v0{b.x - a.x, b.y - a.y, b.z - a.z};
        XrVector3f v1{c.x - b.x, c.y - b.y, c.z - b.z};
        const float len0 = XrVector3f_Length(&v0);
        const float len1 = XrVector3f_Length(&v1);
        if (len0 < 0.0001f || len1 < 0.0001f) {
            return 0.0f;
        }
        XrVector3f_Normalize(&v0);
        XrVector3f_Normalize(&v1);
        const float dot = std::clamp(XrVector3f_Dot(&v0, &v1), -1.0f, 1.0f);
        return std::acos(dot) * 180.0f / MATH_PI;
    }

    XrVector3f SubtractVector(const XrVector3f& a, const XrVector3f& b) {
        return {a.x - b.x, a.y - b.y, a.z - b.z};
    }

    bool NormalizeVectorSafe(XrVector3f* vector) {
        if (XrVector3f_Length(vector) < 0.0001f) {
            return false;
        }
        XrVector3f_Normalize(vector);
        return true;
    }

    XrVector3f ProjectVectorOntoPlane(const XrVector3f& vector, const XrVector3f& planeNormal) {
        const float projection = XrVector3f_Dot(&vector, &planeNormal);
        return {vector.x - planeNormal.x * projection, vector.y - planeNormal.y * projection,
                vector.z - planeNormal.z * projection};
    }

    float AngleBetweenVectorsDegrees(const XrVector3f& a, const XrVector3f& b) {
        XrVector3f lhs = a;
        XrVector3f rhs = b;
        if (!NormalizeVectorSafe(&lhs) || !NormalizeVectorSafe(&rhs)) {
            return 0.0f;
        }
        const float dot = std::clamp(XrVector3f_Dot(&lhs, &rhs), -1.0f, 1.0f);
        return std::acos(dot) * 180.0f / MATH_PI;
    }

    bool IsJointPoseValid(const std::array<XrHandJointLocationEXT, XR_HAND_JOINT_COUNT_EXT>& joints, XrHandJointEXT joint) {
        return IsPoseValid(joints[joint]);
    }

    const char* HandLabel(int hand) {
        return hand == Side::LEFT ? "Left Hand" : "Right Hand";
    }

    std::string IpToString(uint32_t hostOrderIp) {
        in_addr addr{};
        addr.s_addr = htonl(hostOrderIp);
        char buffer[INET_ADDRSTRLEN]{};
        return inet_ntop(AF_INET, &addr, buffer, sizeof(buffer)) != nullptr ? buffer : "0.0.0.0";
    }

    std::string JsonBool(bool value) {
        return value ? "true" : "false";
    }

    bool IsFingerTipJoint(XrHandJointEXT joint) {
        switch (joint) {
            case XR_HAND_JOINT_THUMB_TIP_EXT:
            case XR_HAND_JOINT_INDEX_TIP_EXT:
            case XR_HAND_JOINT_MIDDLE_TIP_EXT:
            case XR_HAND_JOINT_RING_TIP_EXT:
            case XR_HAND_JOINT_LITTLE_TIP_EXT:
                return true;
            default:
                return false;
        }
    }
}  // namespace

class ControllerDiagnosticDemo final : public AndroidOpenXrProgram {
public:
    explicit ControllerDiagnosticDemo(const std::shared_ptr<Configurations>& appConfig);
    ~ControllerDiagnosticDemo() override;

    std::string GetApplicationName() override;

protected:
    void CustomizedExtensionAndFeaturesInit() override;
    bool CustomizedSessionInit() override;
    void CustomizedSessionDestroy() override;
    bool CustomizedAppPostInit() override;
    bool CustomizedPreRenderFrame() override;
    bool CustomizedRender() override;

private:
    struct HandTrackingState {
        XrHandEXT hand{XR_HAND_LEFT_EXT};
        XrHandTrackerEXT tracker{XR_NULL_HANDLE};
        bool tracker_ready{false};
        bool active{false};
        bool aim_valid{false};
        XrHandTrackingAimFlagsFB aim_flags{0};
        float pinch_index{0.0f};
        float pinch_middle{0.0f};
        float pinch_ring{0.0f};
        float pinch_little{0.0f};
        XrPosef palm_pose{MakePose(0.0f, 0.0f, -1.0f)};
        XrPosef index_tip_pose{MakePose(0.0f, 0.0f, -1.0f)};
        XrPosef aim_pose{MakePose(0.0f, 0.0f, -1.0f)};
        XrHandTrackingDataSourceEXT data_source{XR_HAND_TRACKING_DATA_SOURCE_UNOBSTRUCTED_EXT};
        bool data_source_active{false};
        std::array<XrHandJointLocationEXT, XR_HAND_JOINT_COUNT_EXT> joints{};
    };

    struct RobotHandMetrics {
        bool valid{false};
        bool spatial_valid{false};
        bool thumb_side_valid{false};
        bool thumb_bend_valid{false};
        bool index_bend_valid{false};
        bool middle_bend_valid{false};
        bool ring_bend_valid{false};
        bool little_bend_valid{false};
        XrVector3f palm_position{0.0f, 0.0f, 0.0f};
        XrVector3f index_tip_position{0.0f, 0.0f, 0.0f};
        float thumb_side{0.0f};
        float thumb_bend{0.0f};
        float index_bend{0.0f};
        float middle_bend{0.0f};
        float ring_bend{0.0f};
        float little_bend{0.0f};
    };

    struct LanDeviceInfo {
        std::string ip;
        std::string mac;
    };

    struct LanInterfaceInfo {
        bool valid{false};
        uint32_t local_ip_host{0};
        uint32_t netmask_host{0};
        uint32_t broadcast_ip_host{0};
        std::string interface_name;
        std::string local_ip;
        std::string broadcast_ip;
    };

    struct NetworkStreamingState {
        bool udp_enabled{false};
        bool scan_in_progress{false};
        bool send_snapshot_requested{false};
        uint16_t udp_port{5005};
        int selected_target_index{0};
        int socket_fd{-1};
        uint64_t packet_sequence{0};
        std::chrono::steady_clock::time_point last_send_time{};
        std::string scan_status{"Idle"};
        LanInterfaceInfo interface_info{};
        std::vector<LanDeviceInfo> discovered_devices{};
        mutable std::mutex device_mutex;
        std::thread scan_thread;
    };

    void AddReferenceObjects();
    void AddControllerVisuals();
    void AddHandVisuals();
    void AddMainDashboard();
    void AddRuntimePanel();
    void AddRobotMappingPanel();
    void AddNetworkPanel();

    bool InitializeHandTracking();
    void DestroyHandTracking();
    void InitializeNetworkStreaming();
    void ShutdownNetworkStreaming();
    void UpdateControllerVisuals();
    void UpdateHandTracking();
    void UpdateHandVisuals();
    void UpdateRuntimeState();
    void UpdateNetworkStreaming();
    void DetectInputEvents();
    void StartLanScan();
    void AdvanceSelectedLanTarget(int delta);

    std::string BuildControllerText(int hand) const;
    std::string BuildHandText(int hand) const;
    std::string BuildRobotMappingText(int hand) const;
    std::string BuildNetworkPanelText();
    std::string BuildButtonSummary(int hand) const;
    std::string BuildHandGestureSummary(int hand) const;
    RobotHandMetrics ComputeRobotHandMetrics(int hand) const;
    std::string BuildUdpPayloadJson(uint64_t sequence) const;
    std::shared_ptr<Object> CreateControllerModel(int hand);
    LanInterfaceInfo QueryLanInterface() const;
    std::vector<LanDeviceInfo> ProbeLanDevices(const LanInterfaceInfo& interface_info, uint16_t port) const;
    std::string BuildSelectedTargetLabel() const;
    bool ResolveSelectedTarget(sockaddr_in* out_addr);

    void SpawnDebugCube();
    void ClearSpawnedCubes();
    void ResetPanelPose();
    void PulseController(int hand);

private:
    PFN_xrCreateHandTrackerEXT xrCreateHandTrackerEXT_{nullptr};
    PFN_xrDestroyHandTrackerEXT xrDestroyHandTrackerEXT_{nullptr};
    PFN_xrLocateHandJointsEXT xrLocateHandJointsEXT_{nullptr};

    bool hand_tracking_supported_{false};
    bool hand_tracking_aim_supported_{false};
    bool hand_tracking_aim_enabled_{false};
    bool hand_tracking_source_supported_{false};
    bool hand_tracking_ready_{false};
    std::string hand_tracking_create_error_{"not started"};

    std::array<HandTrackingState, Side::COUNT> hand_states_{
            HandTrackingState{XR_HAND_LEFT_EXT}, HandTrackingState{XR_HAND_RIGHT_EXT}};

    std::array<int64_t, Side::COUNT> controller_ids_{{-1, -1}};
    std::array<int64_t, Side::COUNT> aim_ids_{{-1, -1}};
    std::array<int64_t, Side::COUNT> hand_palm_ids_{{-1, -1}};
    std::array<int64_t, Side::COUNT> hand_tip_ids_{{-1, -1}};
    std::array<int64_t, Side::COUNT> hand_aim_ids_{{-1, -1}};

    std::array<std::array<int64_t, XR_HAND_JOINT_COUNT_EXT>, Side::COUNT> hand_joint_ids_{};
    std::array<std::array<int64_t, kHandBoneCount>, Side::COUNT> hand_bone_ids_{};

    std::shared_ptr<GuiWindow> dashboard_window_;
    std::shared_ptr<GuiWindow> controller_window_;
    std::shared_ptr<GuiWindow> hand_window_;
    std::shared_ptr<GuiWindow> action_window_;
    std::shared_ptr<GuiWindow> runtime_window_;
    std::shared_ptr<GuiWindow> robot_mapping_window_;
    std::shared_ptr<GuiWindow> network_window_;
    std::shared_ptr<GuiPlane> dashboard_plane_;
    std::shared_ptr<GuiPlane> controller_plane_;
    std::shared_ptr<GuiPlane> hand_plane_;
    std::shared_ptr<GuiPlane> action_plane_;
    std::shared_ptr<GuiPlane> runtime_plane_;
    std::shared_ptr<GuiPlane> robot_mapping_plane_;
    std::shared_ptr<GuiPlane> network_plane_;

    int hero_summary_text_id_{-1};
    int left_controller_text_id_{-1};
    int right_controller_text_id_{-1};
    int left_hand_text_id_{-1};
    int right_hand_text_id_{-1};
    int event_text_id_{-1};
    int runtime_text_id_{-1};
    int left_robot_mapping_text_id_{-1};
    int right_robot_mapping_text_id_{-1};
    int network_status_text_id_{-1};

    XrPosef dashboard_pose_{MakePose(0.0f, 0.24f, -1.18f)};
    XrPosef controller_pose_{MakePose(-0.52f, -0.18f, -1.26f)};
    XrPosef hand_pose_{MakePose(0.52f, -0.18f, -1.26f)};
    XrPosef action_pose_{MakePose(0.0f, -0.78f, -1.16f)};
    XrPosef runtime_pose_{MakePose(0.88f, 0.36f, -1.56f)};
    XrPosef robot_mapping_pose_{MakePose(-0.88f, 0.36f, -1.56f)};
    XrPosef network_pose_{MakePose(0.0f, -0.12f, -1.72f)};
    std::vector<int64_t> spawned_cube_ids_;
    std::string last_ui_event_{"Dashboard ready"};
    uint32_t previous_buttons_{0};
    uint32_t previous_touches_{0};
    std::array<bool, Side::COUNT> previous_index_pinch_{{false, false}};
    NetworkStreamingState network_state_{};
};

ControllerDiagnosticDemo::ControllerDiagnosticDemo(const std::shared_ptr<Configurations>& appConfig)
    : AndroidOpenXrProgram(appConfig) {
    for (auto& joints : hand_joint_ids_) {
        joints.fill(-1);
    }
    for (auto& bones : hand_bone_ids_) {
        bones.fill(-1);
    }
}

ControllerDiagnosticDemo::~ControllerDiagnosticDemo() {
    ShutdownNetworkStreaming();
    DestroyHandTracking();
}

std::string ControllerDiagnosticDemo::GetApplicationName() {
    return "OpenXRControllerDemo";
}

void ControllerDiagnosticDemo::CustomizedExtensionAndFeaturesInit() {
    AndroidOpenXrProgram::CustomizedExtensionAndFeaturesInit();

    hand_tracking_supported_ = IsExtensionSupported(XR_EXT_HAND_TRACKING_EXTENSION_NAME);
    hand_tracking_aim_supported_ = IsExtensionSupported(XR_FB_HAND_TRACKING_AIM_EXTENSION_NAME);
    hand_tracking_source_supported_ = false;
    hand_tracking_aim_enabled_ = false;

    if (hand_tracking_supported_) {
        non_plugin_extensions_.push_back(XR_EXT_HAND_TRACKING_EXTENSION_NAME);
    }
}

bool ControllerDiagnosticDemo::CustomizedSessionInit() {
    return InitializeHandTracking();
}

void ControllerDiagnosticDemo::CustomizedSessionDestroy() {
    ShutdownNetworkStreaming();
    DestroyHandTracking();
}

bool ControllerDiagnosticDemo::CustomizedAppPostInit() {
    if (!AndroidOpenXrProgram::CustomizedAppPostInit()) {
        return false;
    }

    AddReferenceObjects();
    AddControllerVisuals();
    AddHandVisuals();
    AddMainDashboard();
    AddRuntimePanel();
    AddRobotMappingPanel();
    AddNetworkPanel();
    InitializeNetworkStreaming();
    UpdateRuntimeState();
    return true;
}

bool ControllerDiagnosticDemo::CustomizedPreRenderFrame() {
    UpdateControllerVisuals();
    UpdateHandTracking();
    UpdateHandVisuals();
    DetectInputEvents();
    UpdateRuntimeState();
    UpdateNetworkStreaming();
    return true;
}

bool ControllerDiagnosticDemo::CustomizedRender() {
    return true;
}

void ControllerDiagnosticDemo::AddReferenceObjects() {
    auto& environment_scene = scenes_.at(SAMPLE_SCENE_TYPE_ENVIRONMENT);
    environment_scene.AddObject(
            std::make_shared<CartesianBranch>(MakePose(0.0f, -0.28f, -1.85f), XrVector3f{0.28f, 0.28f, 0.28f}));
}

void ControllerDiagnosticDemo::AddControllerVisuals() {
    auto& scene = scenes_.at(SAMPLE_SCENE_TYPE_CONTROLLER);

    for (int hand = 0; hand < Side::COUNT; ++hand) {
        auto controller_model = CreateControllerModel(hand);
        controller_model->SetVisible(false);
        controller_ids_[hand] = scene.AddObject(controller_model);
    }
}

void ControllerDiagnosticDemo::AddHandVisuals() {
    auto& scene = scenes_.at(SAMPLE_SCENE_TYPE_CUSTOM);

    for (int hand = 0; hand < Side::COUNT; ++hand) {
        for (size_t joint_index = 0; joint_index < XR_HAND_JOINT_COUNT_EXT; ++joint_index) {
            auto joint_marker =
                    std::make_shared<TruncatedCone>(MakePose(0.0f, 0.0f, -1.0f), XrVector3f{0.008f, 0.008f, 0.016f});
            joint_marker->GenerateMesh(1.0f, 1.0f, 1.0f, 20);
            joint_marker->SetVisible(false);
            hand_joint_ids_[hand][joint_index] = scene.AddObject(joint_marker);
        }

        for (size_t bone_index = 0; bone_index < kHandBoneConnections.size(); ++bone_index) {
            auto bone_segment =
                    std::make_shared<TruncatedCone>(MakePose(0.0f, 0.0f, -1.0f), XrVector3f{0.01f, 0.01f, 0.03f});
            bone_segment->GenerateMesh(1.0f, 1.0f, 1.0f, 20);
            bone_segment->SetVisible(false);
            hand_bone_ids_[hand][bone_index] = scene.AddObject(bone_segment);
        }
    }
}

void ControllerDiagnosticDemo::AddMainDashboard() {
    auto& gui_scene = scenes_.at(SAMPLE_SCENE_TYPE_GUI);

    GuiWindow::Builder hero_builder;
    dashboard_window_ = hero_builder.SetSize(1240, 230)
                                .SetBgColor(0.0f, 0.0f, 0.0f, 0.92f)
                                .SetText("")
                                .SetFontSize(24)
                                .SetTextColor(1.0f, 1.0f, 1.0f, 1.0f)
                                .NoScrollbar()
                                .Build();
    hero_summary_text_id_ = dashboard_window_->AddText("OpenXR Input Studio", 42, 34);
    dashboard_window_->SetComponentTextSize(hero_summary_text_id_, 36);
    dashboard_window_->SetComponentTextColor(hero_summary_text_id_, 1.0f, 1.0f, 1.0f, 1.0f);
    event_text_id_ = dashboard_window_->AddText("Recent event: waiting for first input", 42, 156);
    dashboard_window_->SetComponentTextSize(event_text_id_, 16);
    dashboard_window_->SetComponentTextColor(event_text_id_, 0.16f, 0.59f, 1.0f, 1.0f);
    dashboard_plane_ = std::make_shared<GuiPlane>(dashboard_pose_, XrVector3f{1.54f, 0.30f, 1.0f}, dashboard_window_);
    dashboard_plane_->SetRenderDepthable(false);
    gui_scene.AddObject(dashboard_plane_);

    GuiWindow::Builder controller_builder;
    controller_window_ = controller_builder.SetSize(560, 720)
                                 .SetBgColor(0.96f, 0.96f, 0.97f, 0.95f)
                                 .SetText("")
                                 .SetFontSize(20)
                                 .SetTextColor(0.11f, 0.11f, 0.12f, 1.0f)
                                 .NoScrollbar()
                                 .Build();
    const int controller_title = controller_window_->AddText("Controller Diagnostics", 26, 24);
    controller_window_->SetComponentTextSize(controller_title, 28);
    controller_window_->SetComponentTextColor(controller_title, 0.11f, 0.11f, 0.12f, 1.0f);
    const int controller_caption = controller_window_->AddText(
            "Grip + aim pose, analog input, battery, button and touch state.", 26, 74);
    controller_window_->SetComponentTextSize(controller_caption, 16);
    controller_window_->SetComponentTextColor(controller_caption, 0.0f, 0.0f, 0.0f, 0.80f);
    left_controller_text_id_ = controller_window_->AddText("Loading left controller...", 26, 132);
    controller_window_->SetComponentTextSize(left_controller_text_id_, 16);
    controller_window_->SetComponentTextColor(left_controller_text_id_, 0.11f, 0.11f, 0.12f, 1.0f);
    right_controller_text_id_ = controller_window_->AddText("Loading right controller...", 26, 408);
    controller_window_->SetComponentTextSize(right_controller_text_id_, 16);
    controller_window_->SetComponentTextColor(right_controller_text_id_, 0.11f, 0.11f, 0.12f, 1.0f);
    controller_plane_ =
            std::make_shared<GuiPlane>(controller_pose_, XrVector3f{0.78f, 0.98f, 1.0f}, controller_window_);
    controller_plane_->SetRenderDepthable(false);
    gui_scene.AddObject(controller_plane_);

    GuiWindow::Builder hand_builder;
    hand_window_ = hand_builder.SetSize(560, 720)
                           .SetBgColor(0.11f, 0.11f, 0.12f, 0.93f)
                           .SetText("")
                           .SetFontSize(20)
                           .SetTextColor(1.0f, 1.0f, 1.0f, 1.0f)
                           .NoScrollbar()
                           .Build();
    const int hand_title = hand_window_->AddText("Hand Tracking Studio", 26, 24);
    hand_window_->SetComponentTextSize(hand_title, 28);
    hand_window_->SetComponentTextColor(hand_title, 1.0f, 1.0f, 1.0f, 1.0f);
    const int hand_caption = hand_window_->AddText(
            "Runtime hand joints, aim gesture, pinch strength, and live skeletal proxy.", 26, 74);
    hand_window_->SetComponentTextSize(hand_caption, 16);
    hand_window_->SetComponentTextColor(hand_caption, 1.0f, 1.0f, 1.0f, 0.78f);
    left_hand_text_id_ = hand_window_->AddText("Loading left hand...", 26, 132);
    hand_window_->SetComponentTextSize(left_hand_text_id_, 16);
    hand_window_->SetComponentTextColor(left_hand_text_id_, 1.0f, 1.0f, 1.0f, 1.0f);
    right_hand_text_id_ = hand_window_->AddText("Loading right hand...", 26, 408);
    hand_window_->SetComponentTextSize(right_hand_text_id_, 16);
    hand_window_->SetComponentTextColor(right_hand_text_id_, 1.0f, 1.0f, 1.0f, 1.0f);
    hand_plane_ = std::make_shared<GuiPlane>(hand_pose_, XrVector3f{0.78f, 0.98f, 1.0f}, hand_window_);
    hand_plane_->SetRenderDepthable(false);
    gui_scene.AddObject(hand_plane_);

    GuiWindow::Builder action_builder;
    action_window_ = action_builder.SetSize(980, 118)
                             .SetBgColor(0.0f, 0.0f, 0.0f, 0.88f)
                             .SetText("")
                             .SetFontSize(16)
                             .SetTextColor(1.0f, 1.0f, 1.0f, 1.0f)
                             .NoScrollbar()
                             .Build();
    const int center_button = action_window_->AddButton("Center Panels", 130, 34, [&]() { ResetPanelPose(); });
    action_window_->SetComponentSize(center_button, 190, 48);
    action_window_->SetComponentBgColor(center_button, 0.0f, 0.44f, 0.89f, 1.0f);
    const int left_haptic = action_window_->AddButton("Left Haptic", 400, 34, [&]() { PulseController(Side::LEFT); });
    action_window_->SetComponentSize(left_haptic, 170, 48);
    action_window_->SetComponentBgColor(left_haptic, 0.0f, 0.44f, 0.89f, 1.0f);
    const int right_haptic =
            action_window_->AddButton("Right Haptic", 650, 34, [&]() { PulseController(Side::RIGHT); });
    action_window_->SetComponentSize(right_haptic, 170, 48);
    action_window_->SetComponentBgColor(right_haptic, 0.0f, 0.44f, 0.89f, 1.0f);
    action_plane_ = std::make_shared<GuiPlane>(action_pose_, XrVector3f{1.16f, 0.16f, 1.0f}, action_window_);
    action_plane_->SetRenderDepthable(false);
    gui_scene.AddObject(action_plane_);
}

void ControllerDiagnosticDemo::AddRuntimePanel() {
    GuiWindow::Builder builder;
    runtime_window_ = builder.SetSize(430, 330)
                              .SetBgColor(0.96f, 0.96f, 0.97f, 0.95f)
                              .SetText("")
                              .SetFontSize(20)
                              .SetTextColor(0.11f, 0.11f, 0.12f, 1.0f)
                              .NoScrollbar()
                              .Build();

    const int runtime_title = runtime_window_->AddText("Runtime Snapshot", 24, 22);
    runtime_window_->SetComponentTextSize(runtime_title, 24);
    runtime_window_->SetComponentTextColor(runtime_title, 0.11f, 0.11f, 0.12f, 1.0f);

    runtime_text_id_ = runtime_window_->AddText("Waiting for tracking data...", 24, 74);
    runtime_window_->SetComponentTextSize(runtime_text_id_, 16);
    runtime_window_->SetComponentTextColor(runtime_text_id_, 0.11f, 0.11f, 0.12f, 1.0f);

    const int note_id = runtime_window_->AddText(
            "OpenXR apps normally receive runtime-processed hand joints and gestures, not raw headset camera frames.",
            24, 248);
    runtime_window_->SetComponentTextSize(note_id, 16);
    runtime_window_->SetComponentTextColor(note_id, 0.0f, 0.0f, 0.0f, 0.80f);

    runtime_plane_ = std::make_shared<GuiPlane>(runtime_pose_, XrVector3f{0.52f, 0.40f, 1.0f}, runtime_window_);
    runtime_plane_->SetRenderDepthable(false);
    scenes_.at(SAMPLE_SCENE_TYPE_GUI).AddObject(runtime_plane_);
}

void ControllerDiagnosticDemo::AddRobotMappingPanel() {
    GuiWindow::Builder builder;
    robot_mapping_window_ = builder.SetSize(620, 760)
                                   .SetBgColor(0.08f, 0.09f, 0.12f, 0.96f)
                                   .SetText("")
                                   .SetFontSize(20)
                                   .SetTextColor(1.0f, 1.0f, 1.0f, 1.0f)
                                   .NoScrollbar()
                                   .Build();

    const int title_id = robot_mapping_window_->AddText("Robot Hand Mapping", 24, 22);
    robot_mapping_window_->SetComponentTextSize(title_id, 28);
    robot_mapping_window_->SetComponentTextColor(title_id, 1.0f, 0.88f, 0.22f, 1.0f);

    const int note_id = robot_mapping_window_->AddText(
            "Highlighted angles for dexterous-hand retargeting.\n"
            "Finger bend values use the proximal knuckle side near each MCP joint.", 24, 72);
    robot_mapping_window_->SetComponentTextSize(note_id, 16);
    robot_mapping_window_->SetComponentTextColor(note_id, 0.86f, 0.90f, 0.96f, 0.92f);

    left_robot_mapping_text_id_ = robot_mapping_window_->AddText("Waiting for left hand...", 24, 170);
    robot_mapping_window_->SetComponentTextSize(left_robot_mapping_text_id_, 20);
    robot_mapping_window_->SetComponentTextColor(left_robot_mapping_text_id_, 0.35f, 0.84f, 1.0f, 1.0f);

    right_robot_mapping_text_id_ = robot_mapping_window_->AddText("Waiting for right hand...", 24, 430);
    robot_mapping_window_->SetComponentTextSize(right_robot_mapping_text_id_, 20);
    robot_mapping_window_->SetComponentTextColor(right_robot_mapping_text_id_, 1.0f, 0.72f, 0.35f, 1.0f);

    robot_mapping_plane_ =
            std::make_shared<GuiPlane>(robot_mapping_pose_, XrVector3f{0.76f, 0.92f, 1.0f}, robot_mapping_window_);
    robot_mapping_plane_->SetRenderDepthable(false);
    scenes_.at(SAMPLE_SCENE_TYPE_GUI).AddObject(robot_mapping_plane_);
}

void ControllerDiagnosticDemo::AddNetworkPanel() {
    GuiWindow::Builder builder;
    network_window_ = builder.SetSize(760, 480)
                             .SetBgColor(0.05f, 0.07f, 0.10f, 0.96f)
                             .SetText("")
                             .SetFontSize(18)
                             .SetTextColor(1.0f, 1.0f, 1.0f, 1.0f)
                             .NoScrollbar()
                             .Build();

    const int title_id = network_window_->AddText("LAN UDP Streaming", 24, 22);
    network_window_->SetComponentTextSize(title_id, 28);
    network_window_->SetComponentTextColor(title_id, 0.38f, 0.90f, 0.96f, 1.0f);

    const int note_id = network_window_->AddText(
            "Send robot-hand mapping data as JSON over UDP.\n"
            "Scan the local subnet, pick a target, then stream without manual IP entry.", 24, 72);
    network_window_->SetComponentTextSize(note_id, 16);
    network_window_->SetComponentTextColor(note_id, 0.84f, 0.90f, 0.96f, 0.92f);

    const int scan_button = network_window_->AddButton("Scan LAN", 24, 142, [&]() { StartLanScan(); });
    network_window_->SetComponentSize(scan_button, 132, 46);
    network_window_->SetComponentBgColor(scan_button, 0.00f, 0.50f, 0.82f, 1.0f);

    const int prev_button =
            network_window_->AddButton("Prev Target", 176, 142, [&]() { AdvanceSelectedLanTarget(-1); });
    network_window_->SetComponentSize(prev_button, 142, 46);
    network_window_->SetComponentBgColor(prev_button, 0.16f, 0.32f, 0.56f, 1.0f);

    const int next_button =
            network_window_->AddButton("Next Target", 336, 142, [&]() { AdvanceSelectedLanTarget(1); });
    network_window_->SetComponentSize(next_button, 142, 46);
    network_window_->SetComponentBgColor(next_button, 0.16f, 0.32f, 0.56f, 1.0f);

    const int toggle_button = network_window_->AddButton("UDP On/Off", 496, 142, [&]() {
        network_state_.udp_enabled = !network_state_.udp_enabled;
        last_ui_event_ = Fmt("UDP streaming %s", network_state_.udp_enabled ? "enabled" : "disabled");
    });
    network_window_->SetComponentSize(toggle_button, 142, 46);
    network_window_->SetComponentBgColor(toggle_button, 0.02f, 0.62f, 0.44f, 1.0f);

    const int send_once_button = network_window_->AddButton("Send Once", 24, 204, [&]() {
        network_state_.send_snapshot_requested = true;
        last_ui_event_ = Fmt("Queued one UDP snapshot to %s", BuildSelectedTargetLabel().c_str());
    });
    network_window_->SetComponentSize(send_once_button, 132, 44);
    network_window_->SetComponentBgColor(send_once_button, 0.60f, 0.38f, 0.02f, 1.0f);

    const int port_down_button = network_window_->AddButton("Port -", 176, 204, [&]() {
        if (network_state_.udp_port > 1024) {
            --network_state_.udp_port;
            last_ui_event_ = Fmt("UDP port set to %u", static_cast<unsigned int>(network_state_.udp_port));
        }
    });
    network_window_->SetComponentSize(port_down_button, 100, 44);
    network_window_->SetComponentBgColor(port_down_button, 0.26f, 0.26f, 0.30f, 1.0f);

    const int port_up_button = network_window_->AddButton("Port +", 292, 204, [&]() {
        if (network_state_.udp_port < 65535) {
            ++network_state_.udp_port;
            last_ui_event_ = Fmt("UDP port set to %u", static_cast<unsigned int>(network_state_.udp_port));
        }
    });
    network_window_->SetComponentSize(port_up_button, 100, 44);
    network_window_->SetComponentBgColor(port_up_button, 0.26f, 0.26f, 0.30f, 1.0f);

    network_status_text_id_ = network_window_->AddText("Preparing network interface...", 24, 270);
    network_window_->SetComponentTextSize(network_status_text_id_, 16);
    network_window_->SetComponentTextColor(network_status_text_id_, 0.94f, 0.98f, 1.0f, 1.0f);

    network_plane_ = std::make_shared<GuiPlane>(network_pose_, XrVector3f{0.94f, 0.58f, 1.0f}, network_window_);
    network_plane_->SetRenderDepthable(false);
    scenes_.at(SAMPLE_SCENE_TYPE_GUI).AddObject(network_plane_);
}

void ControllerDiagnosticDemo::InitializeNetworkStreaming() {
    ShutdownNetworkStreaming();

    network_state_.packet_sequence = 0;
    network_state_.udp_enabled = false;
    network_state_.send_snapshot_requested = false;
    network_state_.selected_target_index = 0;
    network_state_.last_send_time = std::chrono::steady_clock::time_point{};
    network_state_.interface_info = QueryLanInterface();
    network_state_.scan_status =
            network_state_.interface_info.valid ? "Interface ready" : "No active IPv4 LAN interface found";

    network_state_.socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (network_state_.socket_fd >= 0) {
        const int enabled = 1;
        setsockopt(network_state_.socket_fd, SOL_SOCKET, SO_BROADCAST, &enabled, sizeof(enabled));
    } else {
        network_state_.scan_status = "Failed to create UDP socket";
    }

    if (network_state_.interface_info.valid) {
        StartLanScan();
    }
}

void ControllerDiagnosticDemo::ShutdownNetworkStreaming() {
    if (network_state_.scan_thread.joinable()) {
        network_state_.scan_thread.join();
    }

    if (network_state_.socket_fd >= 0) {
        close(network_state_.socket_fd);
        network_state_.socket_fd = -1;
    }

    network_state_.udp_enabled = false;
    network_state_.scan_in_progress = false;
}

void ControllerDiagnosticDemo::StartLanScan() {
    if (network_state_.scan_in_progress) {
        last_ui_event_ = "LAN scan already in progress";
        return;
    }

    if (network_state_.scan_thread.joinable()) {
        network_state_.scan_thread.join();
    }

    const LanInterfaceInfo interface_info = QueryLanInterface();
    network_state_.interface_info = interface_info;
    if (!interface_info.valid) {
        {
            std::lock_guard<std::mutex> lock(network_state_.device_mutex);
            network_state_.discovered_devices.clear();
        }
        network_state_.selected_target_index = 0;
        network_state_.scan_status = "No active IPv4 LAN interface found";
        last_ui_event_ = "LAN scan unavailable: no Wi-Fi or Ethernet IPv4 interface";
        return;
    }

    network_state_.scan_in_progress = true;
    network_state_.scan_status = Fmt("Scanning %s...", interface_info.interface_name.c_str());
    last_ui_event_ = Fmt("Scanning subnet from %s", interface_info.local_ip.c_str());
    const uint16_t scan_port = network_state_.udp_port;

    network_state_.scan_thread = std::thread([this, interface_info, scan_port]() {
        const auto devices = ProbeLanDevices(interface_info, scan_port);
        {
            std::lock_guard<std::mutex> lock(network_state_.device_mutex);
            network_state_.interface_info = interface_info;
            network_state_.discovered_devices = devices;
            network_state_.scan_in_progress = false;
            network_state_.scan_status =
                    Fmt("Scan complete: %d peer(s) on %s", static_cast<int>(devices.size()),
                        interface_info.interface_name.c_str());
            const int max_index = static_cast<int>(devices.size());
            network_state_.selected_target_index = std::clamp(network_state_.selected_target_index, 0, max_index);
        }
    });
}

void ControllerDiagnosticDemo::AdvanceSelectedLanTarget(int delta) {
    {
        std::lock_guard<std::mutex> lock(network_state_.device_mutex);
        const int total_targets = static_cast<int>(network_state_.discovered_devices.size()) + 1;
        if (total_targets <= 0) {
            network_state_.selected_target_index = 0;
        } else {
            int next_index = network_state_.selected_target_index + delta;
            while (next_index < 0) {
                next_index += total_targets;
            }
            next_index %= total_targets;
            network_state_.selected_target_index = next_index;
        }
    }
    last_ui_event_ = Fmt("UDP target switched to %s", BuildSelectedTargetLabel().c_str());
}

ControllerDiagnosticDemo::LanInterfaceInfo ControllerDiagnosticDemo::QueryLanInterface() const {
    LanInterfaceInfo best_interface;
    ifaddrs* interfaces = nullptr;
    if (getifaddrs(&interfaces) != 0 || interfaces == nullptr) {
        return best_interface;
    }

    auto candidate_score = [](const char* interface_name) {
        if (interface_name == nullptr) {
            return 0;
        }
        const std::string name = interface_name;
        if (name == "wlan0") {
            return 300;
        }
        if (name.rfind("wlan", 0) == 0 || name.rfind("wifi", 0) == 0) {
            return 250;
        }
        if (name.rfind("eth", 0) == 0) {
            return 200;
        }
        return 100;
    };

    int best_score = -1;
    for (ifaddrs* current = interfaces; current != nullptr; current = current->ifa_next) {
        if (current->ifa_addr == nullptr || current->ifa_netmask == nullptr || current->ifa_name == nullptr) {
            continue;
        }
        if (current->ifa_addr->sa_family != AF_INET) {
            continue;
        }
        const unsigned int flags = current->ifa_flags;
        if ((flags & IFF_UP) == 0 || (flags & IFF_LOOPBACK) != 0) {
            continue;
        }

        const auto* address = reinterpret_cast<const sockaddr_in*>(current->ifa_addr);
        const auto* netmask = reinterpret_cast<const sockaddr_in*>(current->ifa_netmask);
        const int score = candidate_score(current->ifa_name);
        if (score < best_score) {
            continue;
        }

        LanInterfaceInfo candidate;
        candidate.valid = true;
        candidate.interface_name = current->ifa_name;
        candidate.local_ip_host = ntohl(address->sin_addr.s_addr);
        candidate.netmask_host = ntohl(netmask->sin_addr.s_addr);
        candidate.broadcast_ip_host =
                (flags & IFF_BROADCAST) != 0 && current->ifa_broadaddr != nullptr
                        ? ntohl(reinterpret_cast<const sockaddr_in*>(current->ifa_broadaddr)->sin_addr.s_addr)
                        : ((candidate.local_ip_host & candidate.netmask_host) | (~candidate.netmask_host));
        candidate.local_ip = IpToString(candidate.local_ip_host);
        candidate.broadcast_ip = IpToString(candidate.broadcast_ip_host);

        best_interface = candidate;
        best_score = score;
    }

    freeifaddrs(interfaces);
    return best_interface;
}

std::vector<ControllerDiagnosticDemo::LanDeviceInfo> ControllerDiagnosticDemo::ProbeLanDevices(
        const LanInterfaceInfo& interface_info, uint16_t port) const {
    std::vector<LanDeviceInfo> devices;
    if (!interface_info.valid) {
        return devices;
    }

    const uint32_t network_prefix = interface_info.local_ip_host & interface_info.netmask_host;
    const uint32_t broadcast_ip = interface_info.broadcast_ip_host;
    if (broadcast_ip <= network_prefix + 1) {
        return devices;
    }

    const uint32_t host_count = broadcast_ip - network_prefix - 1;
    const uint32_t probe_count = std::min<uint32_t>(host_count, 254);

    const int probe_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (probe_socket >= 0) {
        const char* probe_payload = "PICO_OPENXR_DISCOVERY";
        for (uint32_t host_offset = 1; host_offset <= probe_count; ++host_offset) {
            const uint32_t target_ip_host = network_prefix + host_offset;
            if (target_ip_host == interface_info.local_ip_host || target_ip_host == broadcast_ip) {
                continue;
            }

            sockaddr_in target_addr{};
            target_addr.sin_family = AF_INET;
            target_addr.sin_port = htons(port);
            target_addr.sin_addr.s_addr = htonl(target_ip_host);
            sendto(probe_socket, probe_payload, static_cast<int>(std::strlen(probe_payload)), 0,
                   reinterpret_cast<const sockaddr*>(&target_addr), sizeof(target_addr));
        }
        close(probe_socket);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(350));

    std::ifstream arp_file("/proc/net/arp");
    if (!arp_file.is_open()) {
        return devices;
    }

    std::string line;
    std::getline(arp_file, line);
    while (std::getline(arp_file, line)) {
        std::istringstream row(line);
        std::string ip;
        std::string hw_type;
        std::string flags_hex;
        std::string mac;
        std::string mask;
        std::string device_name;
        if (!(row >> ip >> hw_type >> flags_hex >> mac >> mask >> device_name)) {
            continue;
        }
        if (device_name != interface_info.interface_name || mac == "00:00:00:00:00:00") {
            continue;
        }

        in_addr addr{};
        if (inet_pton(AF_INET, ip.c_str(), &addr) != 1) {
            continue;
        }
        const uint32_t ip_host = ntohl(addr.s_addr);
        if (ip_host == interface_info.local_ip_host || (ip_host & interface_info.netmask_host) !=
                                                            (interface_info.local_ip_host & interface_info.netmask_host)) {
            continue;
        }

        const unsigned long flags = std::strtoul(flags_hex.c_str(), nullptr, 16);
        if ((flags & 0x2u) == 0) {
            continue;
        }

        const auto duplicate = std::find_if(devices.begin(), devices.end(), [&](const LanDeviceInfo& device) {
            return device.ip == ip;
        });
        if (duplicate == devices.end()) {
            devices.push_back({ip, mac});
        }
    }

    return devices;
}

std::string ControllerDiagnosticDemo::BuildSelectedTargetLabel() const {
    std::lock_guard<std::mutex> lock(network_state_.device_mutex);
    if (network_state_.selected_target_index <= 0 ||
        network_state_.selected_target_index > static_cast<int>(network_state_.discovered_devices.size())) {
        return network_state_.interface_info.valid ? Fmt("Broadcast (%s)", network_state_.interface_info.broadcast_ip.c_str())
                                                   : "Broadcast";
    }

    const auto& device = network_state_.discovered_devices[network_state_.selected_target_index - 1];
    return Fmt("%s (%s)", device.ip.c_str(), device.mac.c_str());
}

bool ControllerDiagnosticDemo::ResolveSelectedTarget(sockaddr_in* out_addr) {
    if (out_addr == nullptr) {
        return false;
    }

    std::memset(out_addr, 0, sizeof(sockaddr_in));
    out_addr->sin_family = AF_INET;
    out_addr->sin_port = htons(network_state_.udp_port);

    std::lock_guard<std::mutex> lock(network_state_.device_mutex);
    if (!network_state_.interface_info.valid) {
        return false;
    }

    if (network_state_.selected_target_index <= 0 ||
        network_state_.selected_target_index > static_cast<int>(network_state_.discovered_devices.size())) {
        out_addr->sin_addr.s_addr = htonl(network_state_.interface_info.broadcast_ip_host);
        return true;
    }

    return inet_pton(AF_INET, network_state_.discovered_devices[network_state_.selected_target_index - 1].ip.c_str(),
                     &out_addr->sin_addr) == 1;
}

std::string ControllerDiagnosticDemo::BuildUdpPayloadJson(uint64_t sequence) const {
    const auto build_position_json = [](bool valid, const XrVector3f& position) {
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(4)
               << "{\"valid\":" << JsonBool(valid) << ",\"x\":" << position.x << ",\"y\":" << position.y
               << ",\"z\":" << position.z << "}";
        return stream.str();
    };

    const auto build_angle_json = [](bool valid, float angle_deg) {
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(3)
               << "{\"valid\":" << JsonBool(valid) << ",\"deg\":" << angle_deg << "}";
        return stream.str();
    };

    const auto build_hand_json = [&](int hand) {
        const RobotHandMetrics metrics = ComputeRobotHandMetrics(hand);
        const auto& hand_state = hand_states_[hand];
        const bool palm_valid = hand_state.active && IsJointPoseValid(hand_state.joints, XR_HAND_JOINT_PALM_EXT);
        const bool index_tip_valid = hand_state.active && IsJointPoseValid(hand_state.joints, XR_HAND_JOINT_INDEX_TIP_EXT);

        std::ostringstream stream;
        stream << "{\"hand\":\"" << (hand == Side::LEFT ? "left" : "right") << "\""
               << ",\"tracked\":" << JsonBool(hand_state.active)
               << ",\"palm_position_m\":" << build_position_json(palm_valid, metrics.palm_position)
               << ",\"index_tip_position_m\":" << build_position_json(index_tip_valid, metrics.index_tip_position)
               << ",\"joint_angles_deg\":{"
               << "\"thumb_side\":" << build_angle_json(metrics.thumb_side_valid, metrics.thumb_side) << ","
               << "\"thumb_bend\":" << build_angle_json(metrics.thumb_bend_valid, metrics.thumb_bend) << ","
               << "\"index_bend\":" << build_angle_json(metrics.index_bend_valid, metrics.index_bend) << ","
               << "\"middle_bend\":" << build_angle_json(metrics.middle_bend_valid, metrics.middle_bend) << ","
               << "\"ring_bend\":" << build_angle_json(metrics.ring_bend_valid, metrics.ring_bend) << ","
               << "\"little_bend\":" << build_angle_json(metrics.little_bend_valid, metrics.little_bend) << "}"
               << "}";
        return stream.str();
    };

    std::ostringstream stream;
    stream << std::fixed << std::setprecision(6)
           << "{"
           << "\"app\":\"OpenXRControllerDemo\","
           << "\"sequence\":" << sequence << ","
           << "\"frame\":" << static_cast<unsigned long long>(current_frame_in_.frame_number) << ","
           << "\"predicted_display_time_sec\":" << SecondsFromXrTime(current_frame_in_.predicted_display_time)
           << ","
           << "\"hands\":["
           << build_hand_json(Side::LEFT) << ","
           << build_hand_json(Side::RIGHT)
           << "]"
           << "}";
    return stream.str();
}

std::string ControllerDiagnosticDemo::BuildNetworkPanelText() {
    LanInterfaceInfo interface_info;
    std::vector<LanDeviceInfo> devices;
    bool udp_enabled = false;
    bool scan_in_progress = false;
    uint16_t udp_port = 0;
    int selected_target_index = 0;
    uint64_t packet_sequence = 0;
    std::string scan_status;
    {
        std::lock_guard<std::mutex> lock(network_state_.device_mutex);
        interface_info = network_state_.interface_info;
        devices = network_state_.discovered_devices;
        udp_enabled = network_state_.udp_enabled;
        scan_in_progress = network_state_.scan_in_progress;
        udp_port = network_state_.udp_port;
        selected_target_index = network_state_.selected_target_index;
        packet_sequence = network_state_.packet_sequence;
        scan_status = network_state_.scan_status;
    }

    std::string selected_target =
            selected_target_index <= 0 || selected_target_index > static_cast<int>(devices.size())
                    ? (interface_info.valid ? Fmt("Broadcast (%s)", interface_info.broadcast_ip.c_str()) : "Broadcast")
                    : Fmt("%s (%s)", devices[selected_target_index - 1].ip.c_str(),
                          devices[selected_target_index - 1].mac.c_str());

    std::ostringstream stream;
    stream << "UDP stream  " << (udp_enabled ? "ON" : "OFF") << "\n";
    stream << "Local interface  "
           << (interface_info.valid ? Fmt("%s  %s", interface_info.interface_name.c_str(), interface_info.local_ip.c_str())
                                    : "not found")
           << "\n";
    stream << "Broadcast  " << (interface_info.valid ? interface_info.broadcast_ip : "n/a") << "\n";
    stream << "Port  " << udp_port << "\n";
    stream << "Target  " << selected_target << "\n";
    stream << "Scan status  " << (scan_in_progress ? "running" : scan_status) << "\n";
    stream << "Packets sent  " << packet_sequence << "\n";
    stream << "Peers  " << devices.size() << "\n";
    if (!devices.empty()) {
        stream << "Available devices\n";
        const size_t max_devices_to_show = std::min<size_t>(devices.size(), 6);
        for (size_t index = 0; index < max_devices_to_show; ++index) {
            stream << "  " << (index + 1) << ". " << devices[index].ip << "  " << devices[index].mac << "\n";
        }
    }
    stream << "JSON payload\n";
    stream << "sequence, frame, predicted_display_time_sec,\n";
    stream << "hands[left/right].palm_position_m,\n";
    stream << "hands[left/right].index_tip_position_m,\n";
    stream << "six joint angles for robot mapping";
    return stream.str();
}

void ControllerDiagnosticDemo::UpdateNetworkStreaming() {
    if (network_state_.socket_fd < 0) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    uint64_t sequence_to_send = 0;
    bool should_send = false;
    {
        std::lock_guard<std::mutex> lock(network_state_.device_mutex);
        if (!network_state_.interface_info.valid) {
            return;
        }

        const bool send_due = network_state_.udp_enabled &&
                              (network_state_.packet_sequence == 0 ||
                               now - network_state_.last_send_time >= std::chrono::milliseconds(50));
        if (network_state_.send_snapshot_requested || send_due) {
            network_state_.send_snapshot_requested = false;
            network_state_.last_send_time = now;
            sequence_to_send = ++network_state_.packet_sequence;
            should_send = true;
        }
    }

    if (!should_send) {
        return;
    }

    sockaddr_in target_addr{};
    if (!ResolveSelectedTarget(&target_addr)) {
        std::lock_guard<std::mutex> lock(network_state_.device_mutex);
        network_state_.scan_status = "Target resolution failed";
        return;
    }

    const std::string payload = BuildUdpPayloadJson(sequence_to_send);
    const int send_result =
            sendto(network_state_.socket_fd, payload.c_str(), static_cast<int>(payload.size()), 0,
                   reinterpret_cast<const sockaddr*>(&target_addr), sizeof(target_addr));
    {
        std::lock_guard<std::mutex> lock(network_state_.device_mutex);
        network_state_.scan_status =
                send_result >= 0 ? Fmt("Last UDP send OK, %d bytes", send_result) : "UDP send failed";
    }
}

bool ControllerDiagnosticDemo::InitializeHandTracking() {
    hand_tracking_ready_ = false;
    hand_tracking_create_error_ = "XR_EXT_hand_tracking unavailable";

    if (!hand_tracking_supported_) {
        return true;
    }

    if (XR_FAILED(xrGetInstanceProcAddr(GetXrInstance(), "xrCreateHandTrackerEXT",
                                        reinterpret_cast<PFN_xrVoidFunction*>(&xrCreateHandTrackerEXT_))) ||
        XR_FAILED(xrGetInstanceProcAddr(GetXrInstance(), "xrDestroyHandTrackerEXT",
                                        reinterpret_cast<PFN_xrVoidFunction*>(&xrDestroyHandTrackerEXT_))) ||
        XR_FAILED(xrGetInstanceProcAddr(GetXrInstance(), "xrLocateHandJointsEXT",
                                        reinterpret_cast<PFN_xrVoidFunction*>(&xrLocateHandJointsEXT_))) ||
        xrCreateHandTrackerEXT_ == nullptr || xrDestroyHandTrackerEXT_ == nullptr || xrLocateHandJointsEXT_ == nullptr) {
        hand_tracking_create_error_ = "Failed to resolve hand tracking entry points";
        return true;
    }

    XrResult last_error = XR_SUCCESS;
    uint32_t created_trackers = 0;
    for (auto& hand_state : hand_states_) {
        XrHandTrackerCreateInfoEXT create_info{XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT};
        create_info.hand = hand_state.hand;
        create_info.handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT;

        const XrResult result = xrCreateHandTrackerEXT_(GetXrSession(), &create_info, &hand_state.tracker);
        if (XR_SUCCEEDED(result)) {
            hand_state.tracker_ready = true;
            ++created_trackers;
        } else {
            last_error = result;
            hand_state.tracker = XR_NULL_HANDLE;
            hand_state.tracker_ready = false;
            PLOGE("xrCreateHandTrackerEXT failed: %s", to_string(result));
        }
    }

    hand_tracking_ready_ = created_trackers > 0;
    hand_tracking_create_error_ =
            hand_tracking_ready_ ? "ready" : Fmt("Tracker creation failed: %s", to_string(last_error));
    return true;
}

void ControllerDiagnosticDemo::DestroyHandTracking() {
    if (xrDestroyHandTrackerEXT_ == nullptr) {
        return;
    }

    for (auto& hand_state : hand_states_) {
        if (hand_state.tracker != XR_NULL_HANDLE) {
            xrDestroyHandTrackerEXT_(hand_state.tracker);
            hand_state.tracker = XR_NULL_HANDLE;
        }
        hand_state.tracker_ready = false;
        hand_state.active = false;
    }

    hand_tracking_ready_ = false;
}

void ControllerDiagnosticDemo::UpdateControllerVisuals() {
    auto& scene = scenes_.at(SAMPLE_SCENE_TYPE_CONTROLLER);

    for (int hand = 0; hand < Side::COUNT; ++hand) {
        const bool active = current_frame_in_.controller_actives[hand] == XR_TRUE;
        if (const auto controller = scene.GetObject(controller_ids_[hand]); controller != nullptr) {
            controller->SetVisible(active);
            if (active) {
                controller->SetPose(current_frame_in_.controller_poses[hand]);
                controller->SetScale(XrVector3f{
                        0.030f,
                        0.028f * (1.0f + current_frame_in_.controller_grip_value[hand] * 0.08f),
                        0.145f * (1.0f + current_frame_in_.controller_trigger_value[hand] * 0.04f),
                });
            }
        }
    }
}

void ControllerDiagnosticDemo::UpdateHandTracking() {
    for (auto& hand_state : hand_states_) {
        hand_state.active = false;
        hand_state.aim_valid = false;
        hand_state.aim_flags = 0;
        hand_state.data_source_active = false;
        hand_state.pinch_index = 0.0f;
        hand_state.pinch_middle = 0.0f;
        hand_state.pinch_ring = 0.0f;
        hand_state.pinch_little = 0.0f;
    }

    if (!hand_tracking_ready_ || GetAppSpace() == XR_NULL_HANDLE || xrLocateHandJointsEXT_ == nullptr) {
        return;
    }

    for (auto& hand_state : hand_states_) {
        if (!hand_state.tracker_ready) {
            continue;
        }

        XrHandTrackingDataSourceStateEXT source_state{XR_TYPE_HAND_TRACKING_DATA_SOURCE_STATE_EXT};
        XrHandJointLocationsEXT joint_locations{XR_TYPE_HAND_JOINT_LOCATIONS_EXT};
        joint_locations.jointCount = XR_HAND_JOINT_COUNT_EXT;
        joint_locations.jointLocations = hand_state.joints.data();

        if (hand_tracking_source_supported_) {
            joint_locations.next = &source_state;
        }

        XrHandJointsLocateInfoEXT locate_info{XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT};
        locate_info.baseSpace = GetAppSpace();
        locate_info.time = current_frame_in_.predicted_display_time;

        const XrResult result = xrLocateHandJointsEXT_(hand_state.tracker, &locate_info, &joint_locations);
        if (XR_FAILED(result)) {
            PLOGE("xrLocateHandJointsEXT failed: %s", to_string(result));
            continue;
        }

        hand_state.active = joint_locations.isActive == XR_TRUE;
        if (!hand_state.active) {
            continue;
        }

        const auto& palm = hand_state.joints[XR_HAND_JOINT_PALM_EXT];
        const auto& index_tip = hand_state.joints[XR_HAND_JOINT_INDEX_TIP_EXT];
        const auto& thumb_tip = hand_state.joints[XR_HAND_JOINT_THUMB_TIP_EXT];

        if (IsPoseValid(palm)) {
            hand_state.palm_pose = palm.pose;
        }
        if (IsPoseValid(index_tip)) {
            hand_state.index_tip_pose = index_tip.pose;
            hand_state.aim_pose = index_tip.pose;
        }

        if (IsPoseValid(index_tip) && IsPoseValid(thumb_tip)) {
            const float dx = index_tip.pose.position.x - thumb_tip.pose.position.x;
            const float dy = index_tip.pose.position.y - thumb_tip.pose.position.y;
            const float dz = index_tip.pose.position.z - thumb_tip.pose.position.z;
            const float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
            const float pinch_estimate = std::clamp((0.045f - distance) / 0.030f, 0.0f, 1.0f);
            hand_state.pinch_index = std::max(hand_state.pinch_index, pinch_estimate);
        }

        if (hand_tracking_source_supported_) {
            hand_state.data_source_active = source_state.isActive == XR_TRUE;
            hand_state.data_source = source_state.dataSource;
        }
    }
}

void ControllerDiagnosticDemo::UpdateHandVisuals() {
    auto& scene = scenes_.at(SAMPLE_SCENE_TYPE_CUSTOM);

    for (int hand = 0; hand < Side::COUNT; ++hand) {
        const auto& hand_state = hand_states_[hand];

        for (size_t joint_index = 0; joint_index < XR_HAND_JOINT_COUNT_EXT; ++joint_index) {
            const auto& joint = hand_state.joints[joint_index];
            if (const auto joint_object = scene.GetObject(hand_joint_ids_[hand][joint_index]); joint_object != nullptr) {
                const bool visible =
                        hand_state.active && IsPoseValid(joint) && !IsFingerTipJoint(static_cast<XrHandJointEXT>(joint_index));
                joint_object->SetVisible(visible);
                if (visible) {
                    const float radius = std::max(joint.radius * 1.15f, kJointRadiusFloor);
                    joint_object->SetPose(joint.pose);
                    joint_object->SetScale(XrVector3f{radius, radius, radius * 2.0f});
                }
            }
        }

        for (size_t bone = 0; bone < kHandBoneConnections.size(); ++bone) {
            const auto& connection = kHandBoneConnections[bone];
            const auto& from_joint = hand_state.joints[connection.from];
            const auto& to_joint = hand_state.joints[connection.to];
            if (const auto bone_object = scene.GetObject(hand_bone_ids_[hand][bone]); bone_object != nullptr) {
                const bool visible = hand_state.active && IsPoseValid(from_joint) && IsPoseValid(to_joint);
                bone_object->SetVisible(visible);
                if (visible) {
                    XrVector3f delta{};
                    XrVector3f_Sub(&delta, &to_joint.pose.position, &from_joint.pose.position);
                    const float length = std::max(XrVector3f_Length(&delta), 0.01f);
                    const float thickness = std::max(connection.thickness,
                                                     std::max(from_joint.radius, to_joint.radius) * 1.10f);
                    bone_object->SetPose(MakeBonePose(from_joint.pose.position, to_joint.pose.position));
                    bone_object->SetScale(XrVector3f{thickness, thickness, length});
                }
            }
        }
    }
}

void ControllerDiagnosticDemo::UpdateRuntimeState() {
    if (dashboard_window_ != nullptr) {
        const int active_controllers =
                static_cast<int>(current_frame_in_.controller_actives[Side::LEFT] == XR_TRUE) +
                static_cast<int>(current_frame_in_.controller_actives[Side::RIGHT] == XR_TRUE);
        const int active_hands = static_cast<int>(hand_states_[Side::LEFT].active) +
                                 static_cast<int>(hand_states_[Side::RIGHT].active);
        dashboard_window_->UpdateText(
                hero_summary_text_id_,
                Fmt("PICO4 Input Studio\nPure pose capture mode with live controller telemetry and hand skeletal "
                    "diagnostics.\nActive controllers: %d  |  Active hands: %d  |  Hand tracking: %s",
                    active_controllers, active_hands, hand_tracking_ready_ ? "ready" : hand_tracking_create_error_.c_str())
                        .c_str());
        dashboard_window_->UpdateText(event_text_id_, Fmt("Recent event: %s", last_ui_event_.c_str()).c_str());
    }

    if (controller_window_ != nullptr) {
        controller_window_->UpdateText(left_controller_text_id_, BuildControllerText(Side::LEFT).c_str());
        controller_window_->UpdateText(right_controller_text_id_, BuildControllerText(Side::RIGHT).c_str());
    }

    if (hand_window_ != nullptr) {
        hand_window_->UpdateText(left_hand_text_id_, BuildHandText(Side::LEFT).c_str());
        hand_window_->UpdateText(right_hand_text_id_, BuildHandText(Side::RIGHT).c_str());
    }

    if (runtime_window_ != nullptr) {
        const int active_controllers =
                static_cast<int>(current_frame_in_.controller_actives[Side::LEFT] == XR_TRUE) +
                static_cast<int>(current_frame_in_.controller_actives[Side::RIGHT] == XR_TRUE);
        const int active_hands = static_cast<int>(hand_states_[Side::LEFT].active) +
                                 static_cast<int>(hand_states_[Side::RIGHT].active);

        runtime_window_->UpdateText(
                runtime_text_id_,
                Fmt("Frame: %lld\nPredicted display time: %.3f s\nControllers active: %d\nHands active: %d\n"
                    "Hand ext: %s\nAim ext: %s\nData source ext: %s\nTracker init: %s",
                    static_cast<long long>(current_frame_in_.frame_number),
                    SecondsFromXrTime(current_frame_in_.predicted_display_time), active_controllers, active_hands,
                    hand_tracking_supported_ ? "enabled" : "missing",
                    hand_tracking_aim_enabled_ ? "enabled" : "disabled for pose capture",
                    hand_tracking_source_supported_ ? "enabled" : "missing", hand_tracking_create_error_.c_str())
                        .c_str());
    }

    if (robot_mapping_window_ != nullptr) {
        robot_mapping_window_->UpdateText(left_robot_mapping_text_id_, BuildRobotMappingText(Side::LEFT).c_str());
        robot_mapping_window_->UpdateText(right_robot_mapping_text_id_, BuildRobotMappingText(Side::RIGHT).c_str());
    }

    if (network_window_ != nullptr) {
        network_window_->UpdateText(network_status_text_id_, BuildNetworkPanelText().c_str());
    }
}

void ControllerDiagnosticDemo::DetectInputEvents() {
    const uint32_t buttons = current_frame_in_.all_buttons_bitmask;
    const uint32_t touches = current_frame_in_.all_touches_bitmask;
    const uint32_t button_down = buttons & ~previous_buttons_;
    const uint32_t button_up = previous_buttons_ & ~buttons;
    const uint32_t touch_down = touches & ~previous_touches_;

    const auto describe_bits = [](uint32_t bits, const std::vector<std::pair<uint32_t, const char*>>& mapping) {
        std::string result;
        for (const auto& [mask, label] : mapping) {
            if ((bits & mask) == 0) {
                continue;
            }
            if (!result.empty()) {
                result += ", ";
            }
            result += label;
        }
        return result;
    };

    const std::vector<std::pair<uint32_t, const char*>> button_names = {
            {kButtonTriggerLeft, "L Trigger"},   {kButtonTriggerRight, "R Trigger"},
            {kButtonGripTriggerLeft, "L Grip"},  {kButtonGripTriggerRight, "R Grip"},
            {kButtonJoystickLeft, "L Stick"},    {kButtonJoystickRight, "R Stick"},
            {kButtonX, "X"},                     {kButtonY, "Y"},
            {kButtonA, "A"},                     {kButtonB, "B"},
            {kButtonMenu, "Menu"},               {kButtonHome, "Home"},
            {kButtonBackLeft, "L Back"},         {kButtonBackRight, "R Back"},
            {kButtonTouchpadLeft, "L Touchpad"}, {kButtonTouchpadRight, "R Touchpad"},
            {kButtonSideLeft, "L Side"},         {kButtonSideRight, "R Side"}};
    const std::vector<std::pair<uint32_t, const char*>> touch_names = {
            {kTouchJoystickLeft, "L Stick"},       {kTouchJoystickRight, "R Stick"},
            {kTouchTriggerLeft, "L Trigger"},      {kTouchTriggerRight, "R Trigger"},
            {kTouchThumbRestLeft, "L Thumbrest"},  {kTouchThumbRestRight, "R Thumbrest"},
            {kTouchRockerLeft, "L Rocker"},        {kTouchRockerRight, "R Rocker"},
            {kTouchX, "X"},                        {kTouchY, "Y"},
            {kTouchA, "A"},                        {kTouchB, "B"}};

    if (button_down != 0) {
        last_ui_event_ = "Button down: " + describe_bits(button_down, button_names);
    } else if (button_up != 0) {
        last_ui_event_ = "Button up: " + describe_bits(button_up, button_names);
    } else if (touch_down != 0) {
        last_ui_event_ = "Touch begin: " + describe_bits(touch_down, touch_names);
    }

    previous_buttons_ = buttons;
    previous_touches_ = touches;
}

std::string ControllerDiagnosticDemo::BuildControllerText(int hand) const {
    const char* label = hand == Side::LEFT ? "Left Controller" : "Right Controller";
    const auto& grip_pose = current_frame_in_.controller_poses[hand];
    const auto& aim_pose = current_frame_in_.controller_aim_poses[hand];
    const auto& stick = hand == Side::LEFT ? current_frame_in_.left_joystick_position
                                           : current_frame_in_.right_joystick_position;

    if (current_frame_in_.controller_actives[hand] != XR_TRUE) {
        return Fmt("%s\nStatus: Inactive\nAnalog\nTrigger %.2f  Grip %.2f  Battery %.1f / 5\nStick %.2f, %.2f\n%s",
                   label, current_frame_in_.controller_trigger_value[hand],
                   current_frame_in_.controller_grip_value[hand], current_frame_in_.controller_battery_value[hand],
                   stick.x, stick.y, BuildButtonSummary(hand).c_str());
    }

    return Fmt("%s\nStatus: Active\nGrip pose %.2f, %.2f, %.2f\nAim pose %.2f, %.2f, %.2f\n"
               "Analog\nTrigger %.2f  Grip %.2f  Battery %.1f / 5\nStick %.2f, %.2f\n%s",
               label, grip_pose.position.x, grip_pose.position.y, grip_pose.position.z, aim_pose.position.x,
               aim_pose.position.y, aim_pose.position.z, current_frame_in_.controller_trigger_value[hand],
               current_frame_in_.controller_grip_value[hand], current_frame_in_.controller_battery_value[hand], stick.x,
               stick.y, BuildButtonSummary(hand).c_str());
}

std::string ControllerDiagnosticDemo::BuildHandText(int hand) const {
    const char* label = HandLabel(hand);
    const auto& hand_state = hand_states_[hand];

    if (!hand_tracking_supported_) {
        return Fmt("%s\nStatus: Unsupported\nThis runtime does not expose XR_EXT_hand_tracking.", label);
    }

    if (!hand_state.tracker_ready) {
        return Fmt("%s\nStatus: Tracker unavailable\nReason\n%s", label, hand_tracking_create_error_.c_str());
    }

    if (!hand_state.active) {
        return Fmt("%s\nStatus: Inactive\nIf controllers are held, the runtime may pause the camera-driven hand "
                   "skeleton.",
                   label);
    }

    return Fmt("%s\nStatus: Active\nLive proxy: skeletal bones only\nPalm %.2f, %.2f, %.2f\n"
               "Index tip %.2f, %.2f, %.2f\nCommand gestures filtered in app\n%s",
               label, hand_state.palm_pose.position.x, hand_state.palm_pose.position.y, hand_state.palm_pose.position.z,
               hand_state.index_tip_pose.position.x, hand_state.index_tip_pose.position.y,
               hand_state.index_tip_pose.position.z,
               BuildHandGestureSummary(hand).c_str());
}

ControllerDiagnosticDemo::RobotHandMetrics ControllerDiagnosticDemo::ComputeRobotHandMetrics(int hand) const {
    RobotHandMetrics metrics;
    if (hand < 0 || hand >= Side::COUNT) {
        return metrics;
    }

    const auto& hand_state = hand_states_[hand];
    if (!hand_state.active) {
        return metrics;
    }

    const auto& joints = hand_state.joints;

    if (IsJointPoseValid(joints, XR_HAND_JOINT_PALM_EXT)) {
        metrics.spatial_valid = true;
        metrics.palm_position = joints[XR_HAND_JOINT_PALM_EXT].pose.position;
    }
    if (IsJointPoseValid(joints, XR_HAND_JOINT_INDEX_TIP_EXT)) {
        metrics.index_tip_position = joints[XR_HAND_JOINT_INDEX_TIP_EXT].pose.position;
    }

    if (IsJointPoseValid(joints, XR_HAND_JOINT_PALM_EXT) && IsJointPoseValid(joints, XR_HAND_JOINT_THUMB_METACARPAL_EXT) &&
        IsJointPoseValid(joints, XR_HAND_JOINT_THUMB_PROXIMAL_EXT)) {
        metrics.thumb_bend_valid = true;
        metrics.thumb_bend = AngleBetweenSegmentsDegrees(joints[XR_HAND_JOINT_PALM_EXT].pose.position,
                                                         joints[XR_HAND_JOINT_THUMB_METACARPAL_EXT].pose.position,
                                                         joints[XR_HAND_JOINT_THUMB_PROXIMAL_EXT].pose.position);
    }

    if (IsJointPoseValid(joints, XR_HAND_JOINT_INDEX_METACARPAL_EXT) &&
        IsJointPoseValid(joints, XR_HAND_JOINT_INDEX_PROXIMAL_EXT) &&
        IsJointPoseValid(joints, XR_HAND_JOINT_INDEX_INTERMEDIATE_EXT)) {
        metrics.index_bend_valid = true;
        metrics.index_bend = AngleBetweenSegmentsDegrees(joints[XR_HAND_JOINT_INDEX_METACARPAL_EXT].pose.position,
                                                         joints[XR_HAND_JOINT_INDEX_PROXIMAL_EXT].pose.position,
                                                         joints[XR_HAND_JOINT_INDEX_INTERMEDIATE_EXT].pose.position);
    }

    if (IsJointPoseValid(joints, XR_HAND_JOINT_MIDDLE_METACARPAL_EXT) &&
        IsJointPoseValid(joints, XR_HAND_JOINT_MIDDLE_PROXIMAL_EXT) &&
        IsJointPoseValid(joints, XR_HAND_JOINT_MIDDLE_INTERMEDIATE_EXT)) {
        metrics.middle_bend_valid = true;
        metrics.middle_bend = AngleBetweenSegmentsDegrees(joints[XR_HAND_JOINT_MIDDLE_METACARPAL_EXT].pose.position,
                                                          joints[XR_HAND_JOINT_MIDDLE_PROXIMAL_EXT].pose.position,
                                                          joints[XR_HAND_JOINT_MIDDLE_INTERMEDIATE_EXT].pose.position);
    }

    if (IsJointPoseValid(joints, XR_HAND_JOINT_RING_METACARPAL_EXT) &&
        IsJointPoseValid(joints, XR_HAND_JOINT_RING_PROXIMAL_EXT) &&
        IsJointPoseValid(joints, XR_HAND_JOINT_RING_INTERMEDIATE_EXT)) {
        metrics.ring_bend_valid = true;
        metrics.ring_bend = AngleBetweenSegmentsDegrees(joints[XR_HAND_JOINT_RING_METACARPAL_EXT].pose.position,
                                                        joints[XR_HAND_JOINT_RING_PROXIMAL_EXT].pose.position,
                                                        joints[XR_HAND_JOINT_RING_INTERMEDIATE_EXT].pose.position);
    }

    if (IsJointPoseValid(joints, XR_HAND_JOINT_LITTLE_METACARPAL_EXT) &&
        IsJointPoseValid(joints, XR_HAND_JOINT_LITTLE_PROXIMAL_EXT) &&
        IsJointPoseValid(joints, XR_HAND_JOINT_LITTLE_INTERMEDIATE_EXT)) {
        metrics.little_bend_valid = true;
        metrics.little_bend = AngleBetweenSegmentsDegrees(joints[XR_HAND_JOINT_LITTLE_METACARPAL_EXT].pose.position,
                                                          joints[XR_HAND_JOINT_LITTLE_PROXIMAL_EXT].pose.position,
                                                          joints[XR_HAND_JOINT_LITTLE_INTERMEDIATE_EXT].pose.position);
    }

    if (IsJointPoseValid(joints, XR_HAND_JOINT_PALM_EXT) && IsJointPoseValid(joints, XR_HAND_JOINT_INDEX_METACARPAL_EXT) &&
        IsJointPoseValid(joints, XR_HAND_JOINT_LITTLE_METACARPAL_EXT) &&
        IsJointPoseValid(joints, XR_HAND_JOINT_THUMB_METACARPAL_EXT) &&
        IsJointPoseValid(joints, XR_HAND_JOINT_THUMB_PROXIMAL_EXT)) {
        XrVector3f palm_to_index =
                SubtractVector(joints[XR_HAND_JOINT_INDEX_METACARPAL_EXT].pose.position,
                               joints[XR_HAND_JOINT_PALM_EXT].pose.position);
        XrVector3f palm_to_little =
                SubtractVector(joints[XR_HAND_JOINT_LITTLE_METACARPAL_EXT].pose.position,
                               joints[XR_HAND_JOINT_PALM_EXT].pose.position);
        XrVector3f thumb_direction =
                SubtractVector(joints[XR_HAND_JOINT_THUMB_PROXIMAL_EXT].pose.position,
                               joints[XR_HAND_JOINT_THUMB_METACARPAL_EXT].pose.position);
        XrVector3f palm_normal{};
        XrVector3f_Cross(&palm_normal, &palm_to_index, &palm_to_little);
        if (NormalizeVectorSafe(&palm_normal)) {
            const XrVector3f thumb_on_palm = ProjectVectorOntoPlane(thumb_direction, palm_normal);
            const XrVector3f index_on_palm = ProjectVectorOntoPlane(palm_to_index, palm_normal);
            metrics.thumb_side_valid = true;
            metrics.thumb_side = AngleBetweenVectorsDegrees(thumb_on_palm, index_on_palm);
        }
    }

    metrics.valid = metrics.spatial_valid || metrics.thumb_side_valid || metrics.thumb_bend_valid ||
                    metrics.index_bend_valid || metrics.middle_bend_valid || metrics.ring_bend_valid ||
                    metrics.little_bend_valid;
    return metrics;
}

std::string ControllerDiagnosticDemo::BuildRobotMappingText(int hand) const {
    const char* label = HandLabel(hand);
    const auto& hand_state = hand_states_[hand];

    if (!hand_tracking_supported_) {
        return Fmt("%s\nStatus: Unsupported\nHand tracking extension is not available.", label);
    }

    if (!hand_state.tracker_ready) {
        return Fmt("%s\nStatus: Tracker unavailable\n%s", label, hand_tracking_create_error_.c_str());
    }

    if (!hand_state.active) {
        return Fmt("%s\nStatus: Inactive\nAwaiting a live tracked hand skeleton.", label);
    }

    const RobotHandMetrics metrics = ComputeRobotHandMetrics(hand);
    if (!metrics.valid) {
        return Fmt("%s\nStatus: Active\nRobot mapping metrics are not ready yet.", label);
    }

    const auto angle_line = [](const char* name, bool valid, float value) {
        return valid ? Fmt("%s  %.1f deg", name, value) : Fmt("%s  n/a", name);
    };

    return Fmt("%s\nStatus: Active\nHand root XYZ  %.3f, %.3f, %.3f m\nIndex tip XYZ  %.3f, %.3f, %.3f m\n"
               "%s\n%s\n%s\n%s\n%s\n%s",
               label, metrics.palm_position.x, metrics.palm_position.y, metrics.palm_position.z,
               metrics.index_tip_position.x, metrics.index_tip_position.y, metrics.index_tip_position.z,
               angle_line("Thumb side", metrics.thumb_side_valid, metrics.thumb_side).c_str(),
               angle_line("Thumb bend", metrics.thumb_bend_valid, metrics.thumb_bend).c_str(),
               angle_line("Index bend", metrics.index_bend_valid, metrics.index_bend).c_str(),
               angle_line("Middle bend", metrics.middle_bend_valid, metrics.middle_bend).c_str(),
               angle_line("Ring bend", metrics.ring_bend_valid, metrics.ring_bend).c_str(),
               angle_line("Little bend", metrics.little_bend_valid, metrics.little_bend).c_str());
}

std::string ControllerDiagnosticDemo::BuildButtonSummary(int hand) const {
    const uint32_t left_or_right_trigger =
            hand == Side::LEFT ? kButtonTriggerLeft : kButtonTriggerRight;
    const uint32_t left_or_right_grip =
            hand == Side::LEFT ? kButtonGripTriggerLeft : kButtonGripTriggerRight;
    const uint32_t left_or_right_stick =
            hand == Side::LEFT ? kButtonJoystickLeft : kButtonJoystickRight;
    const uint32_t left_or_right_back = hand == Side::LEFT ? kButtonBackLeft : kButtonBackRight;
    const uint32_t left_or_right_pad =
            hand == Side::LEFT ? kButtonTouchpadLeft : kButtonTouchpadRight;
    const uint32_t left_or_right_side = hand == Side::LEFT ? kButtonSideLeft : kButtonSideRight;
    const uint32_t left_or_right_touch_trigger =
            hand == Side::LEFT ? kTouchTriggerLeft : kTouchTriggerRight;
    const uint32_t left_or_right_touch_stick =
            hand == Side::LEFT ? kTouchJoystickLeft : kTouchJoystickRight;
    const uint32_t left_or_right_thumb =
            hand == Side::LEFT ? kTouchThumbRestLeft : kTouchThumbRestRight;
    const uint32_t left_or_right_rocker =
            hand == Side::LEFT ? kTouchRockerLeft : kTouchRockerRight;

    std::string button_text;
    const auto append = [&](std::string* text, bool enabled, const char* name) {
        if (!enabled) {
            return;
        }
        if (!text->empty()) {
            *text += ", ";
        }
        *text += name;
    };

    append(&button_text, (current_frame_in_.all_buttons_bitmask & left_or_right_trigger) != 0, "Trigger");
    append(&button_text, (current_frame_in_.all_buttons_bitmask & left_or_right_grip) != 0, "Grip");
    append(&button_text, (current_frame_in_.all_buttons_bitmask & left_or_right_stick) != 0, "Stick");
    append(&button_text, (current_frame_in_.all_buttons_bitmask & left_or_right_back) != 0, "Back");
    append(&button_text, (current_frame_in_.all_buttons_bitmask & left_or_right_pad) != 0, "Touchpad");
    append(&button_text, (current_frame_in_.all_buttons_bitmask & left_or_right_side) != 0, "Side");

    if (hand == Side::LEFT) {
        append(&button_text, (current_frame_in_.all_buttons_bitmask & kButtonX) != 0, "X");
        append(&button_text, (current_frame_in_.all_buttons_bitmask & kButtonY) != 0, "Y");
        append(&button_text, (current_frame_in_.all_buttons_bitmask & kButtonMenu) != 0, "Menu");
    } else {
        append(&button_text, (current_frame_in_.all_buttons_bitmask & kButtonA) != 0, "A");
        append(&button_text, (current_frame_in_.all_buttons_bitmask & kButtonB) != 0, "B");
        append(&button_text, (current_frame_in_.all_buttons_bitmask & kButtonHome) != 0, "Home");
    }

    std::string touch_text;
    append(&touch_text, (current_frame_in_.all_touches_bitmask & left_or_right_touch_trigger) != 0, "Trigger");
    append(&touch_text, (current_frame_in_.all_touches_bitmask & left_or_right_touch_stick) != 0, "Stick");
    append(&touch_text, (current_frame_in_.all_touches_bitmask & left_or_right_thumb) != 0, "Thumbrest");
    append(&touch_text, (current_frame_in_.all_touches_bitmask & left_or_right_rocker) != 0, "Rocker");
    if (hand == Side::LEFT) {
        append(&touch_text, (current_frame_in_.all_touches_bitmask & kTouchX) != 0, "X");
        append(&touch_text, (current_frame_in_.all_touches_bitmask & kTouchY) != 0, "Y");
    } else {
        append(&touch_text, (current_frame_in_.all_touches_bitmask & kTouchA) != 0, "A");
        append(&touch_text, (current_frame_in_.all_touches_bitmask & kTouchB) != 0, "B");
    }

    return Fmt("Buttons\n%s\nTouches\n%s", button_text.empty() ? "none" : button_text.c_str(),
               touch_text.empty() ? "none" : touch_text.c_str());
}

std::string ControllerDiagnosticDemo::BuildHandGestureSummary(int hand) const {
    const auto& hand_state = hand_states_[hand];

    if (!hand_state.active) {
        return "Joint angles unavailable";
    }

    const char* data_source = "unknown";
    if (hand_tracking_source_supported_ && hand_state.data_source_active) {
        data_source = hand_state.data_source == XR_HAND_TRACKING_DATA_SOURCE_UNOBSTRUCTED_EXT ? "camera/unobstructed"
                                                                                               : "controller-assisted";
    }

    std::string angle_lines = Fmt("Source\n%s", data_source);
    for (const auto& chain : kFingerAngleChains) {
        bool valid = true;
        for (const auto joint : chain.joints) {
            valid &= IsPoseValid(hand_state.joints[joint]);
        }
        if (!valid) {
            angle_lines += Fmt("\n%s\nn/a", chain.name);
            continue;
        }

        const float a0 = AngleBetweenSegmentsDegrees(hand_state.joints[chain.joints[0]].pose.position,
                                                     hand_state.joints[chain.joints[1]].pose.position,
                                                     hand_state.joints[chain.joints[2]].pose.position);
        const float a1 = AngleBetweenSegmentsDegrees(hand_state.joints[chain.joints[1]].pose.position,
                                                     hand_state.joints[chain.joints[2]].pose.position,
                                                     hand_state.joints[chain.joints[3]].pose.position);
        const float a2 = AngleBetweenSegmentsDegrees(hand_state.joints[chain.joints[2]].pose.position,
                                                     hand_state.joints[chain.joints[3]].pose.position,
                                                     hand_state.joints[chain.joints[4]].pose.position);
        angle_lines += Fmt("\n%s\n%s %.0f  %s %.0f  %s %.0f", chain.name, chain.labels[0], a0, chain.labels[1], a1,
                           chain.labels[2], a2);
    }

    return angle_lines;
}

void ControllerDiagnosticDemo::SpawnDebugCube() {
    auto& scene = scenes_.at(SAMPLE_SCENE_TYPE_CUSTOM);
    const size_t index = spawned_cube_ids_.size();
    const float column = static_cast<float>(index % 5);
    const float row = static_cast<float>(index / 5);
    const float x = -0.36f + column * 0.18f;
    const float y = -0.46f + row * 0.14f;
    const float z = -1.95f - row * 0.10f;
    auto cube = std::make_shared<Cube>(MakePose(x, y, z), MakeUniformScale(0.08f));
    spawned_cube_ids_.push_back(scene.AddObject(cube));
    last_ui_event_ = Fmt("Spawned debug cube #%d", static_cast<int>(spawned_cube_ids_.size()));
}

void ControllerDiagnosticDemo::ClearSpawnedCubes() {
    auto& scene = scenes_.at(SAMPLE_SCENE_TYPE_CUSTOM);
    for (const int64_t id : spawned_cube_ids_) {
        scene.RemoveObject(id);
    }
    spawned_cube_ids_.clear();
    last_ui_event_ = "Cleared debug cubes";
}

void ControllerDiagnosticDemo::ResetPanelPose() {
    dashboard_pose_ = MakePose(0.0f, 0.24f, -1.18f);
    controller_pose_ = MakePose(-0.52f, -0.18f, -1.26f);
    hand_pose_ = MakePose(0.52f, -0.18f, -1.26f);
    action_pose_ = MakePose(0.0f, -0.78f, -1.16f);
    runtime_pose_ = MakePose(0.88f, 0.36f, -1.56f);
    robot_mapping_pose_ = MakePose(-0.88f, 0.36f, -1.56f);
    network_pose_ = MakePose(0.0f, -0.12f, -1.72f);

    if (dashboard_plane_ != nullptr) {
        dashboard_plane_->SetPose(dashboard_pose_);
    }
    if (controller_plane_ != nullptr) {
        controller_plane_->SetPose(controller_pose_);
    }
    if (hand_plane_ != nullptr) {
        hand_plane_->SetPose(hand_pose_);
    }
    if (action_plane_ != nullptr) {
        action_plane_->SetPose(action_pose_);
    }
    if (runtime_plane_ != nullptr) {
        runtime_plane_->SetPose(runtime_pose_);
    }
    if (robot_mapping_plane_ != nullptr) {
        robot_mapping_plane_->SetPose(robot_mapping_pose_);
    }
    if (network_plane_ != nullptr) {
        network_plane_->SetPose(network_pose_);
    }
    last_ui_event_ = "Recentered the full panel layout in front of the headset";
}

void ControllerDiagnosticDemo::PulseController(int hand) {
    if (hand < 0 || hand >= Side::COUNT || GetXrSession() == XR_NULL_HANDLE) {
        return;
    }

    XrHapticVibration vibration{XR_TYPE_HAPTIC_VIBRATION};
    vibration.amplitude = 0.75f;
    vibration.duration = XR_MIN_HAPTIC_DURATION;
    vibration.frequency = XR_FREQUENCY_UNSPECIFIED;

    XrHapticActionInfo info{XR_TYPE_HAPTIC_ACTION_INFO};
    info.action = input_.vibrate_action;
    info.subactionPath = input_.controller_subaction_paths[hand];

    const XrResult result =
            xrApplyHapticFeedback(GetXrSession(), &info, reinterpret_cast<const XrHapticBaseHeader*>(&vibration));
    last_ui_event_ = XR_SUCCEEDED(result)
                             ? Fmt("%s controller haptic pulse", hand == Side::LEFT ? "Left" : "Right")
                             : Fmt("%s haptic failed: %s", hand == Side::LEFT ? "Left" : "Right", to_string(result));
}

std::shared_ptr<Object> ControllerDiagnosticDemo::CreateControllerModel(int hand) {
    auto body = std::make_shared<TruncatedCone>(MakePose(0.0f, 0.0f, 0.0f), XrVector3f{0.030f, 0.028f, 0.145f});
    body->GenerateMesh(0.70f, 0.44f, 1.0f, 28);

    auto crown = std::make_shared<TruncatedCone>(MakePose(0.0f, 0.024f, 0.030f), XrVector3f{0.044f, 0.034f, 0.060f});
    crown->GenerateMesh(0.92f, 0.58f, 1.0f, 24);
    body->AddChild(crown);

    const float side = hand == Side::LEFT ? -1.0f : 1.0f;
    auto tracking_arc =
            std::make_shared<TruncatedCone>(MakePose(0.020f * side, 0.050f, 0.070f), XrVector3f{0.020f, 0.046f, 0.080f});
    tracking_arc->GenerateMesh(0.50f, 0.28f, 1.0f, 20);
    body->AddChild(tracking_arc);

    auto trigger = std::make_shared<TruncatedCone>(MakePose(0.0f, -0.010f, 0.034f), XrVector3f{0.016f, 0.014f, 0.050f});
    trigger->GenerateMesh(0.55f, 0.36f, 1.0f, 18);
    body->AddChild(trigger);

    auto grip = std::make_shared<TruncatedCone>(MakePose(0.020f * side, -0.010f, -0.006f),
                                                XrVector3f{0.014f, 0.022f, 0.060f});
    grip->GenerateMesh(0.48f, 0.32f, 1.0f, 18);
    body->AddChild(grip);

    return body;
}

void android_main(struct android_app* app) {
    auto config = std::make_shared<Configurations>();
    ControllerDiagnosticDemo program(config);
    program.Run(app);
}
