//
// Created by xiang on 25-9-12.
//

#include "core/system/loc_system.h"
#include "core/localization/localization.h"
#include "io/yaml_io.h"
#include "wrapper/ros_utils.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <utility>

#include <pcl/common/transforms.h>

namespace lightning {

namespace {

template <typename T>
T ReadYamlValueOr(const YAML::Node& node, const std::string& key, const T& fallback) {
    if (!node || !node[key]) {
        return fallback;
    }
    return node[key].as<T>();
}

std::array<double, 3> ReadYamlArray3Or(const YAML::Node& node, const std::string& key,
                                       const std::array<double, 3>& fallback) {
    if (!node || !node[key] || !node[key].IsSequence() || node[key].size() != 3) {
        return fallback;
    }

    return {node[key][0].as<double>(), node[key][1].as<double>(), node[key][2].as<double>()};
}

std::array<double, 7> ReadYamlArray7Or(const YAML::Node& node, const std::string& key,
                                       const std::array<double, 7>& fallback) {
    if (!node || !node[key] || !node[key].IsSequence() || node[key].size() != 7) {
        return fallback;
    }

    return {node[key][0].as<double>(), node[key][1].as<double>(), node[key][2].as<double>(),
            node[key][3].as<double>(), node[key][4].as<double>(), node[key][5].as<double>(),
            node[key][6].as<double>()};
}

void AddDiagnosticValue(diagnostic_msgs::msg::DiagnosticStatus& status, const std::string& key,
                        const std::string& value) {
    diagnostic_msgs::msg::KeyValue pair;
    pair.key = key;
    pair.value = value;
    status.values.push_back(std::move(pair));
}

std::string BoolString(bool value) { return value ? "true" : "false"; }

std::string LowerString(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return std::tolower(c); });
    return value;
}

std::string JsonEscape(const std::string& value) {
    std::ostringstream out;
    for (unsigned char c : value) {
        switch (c) {
            case '"':
                out << "\\\"";
                break;
            case '\\':
                out << "\\\\";
                break;
            case '\b':
                out << "\\b";
                break;
            case '\f':
                out << "\\f";
                break;
            case '\n':
                out << "\\n";
                break;
            case '\r':
                out << "\\r";
                break;
            case '\t':
                out << "\\t";
                break;
            default:
                if (c < 0x20) {
                    out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c)
                        << std::dec << std::setfill(' ');
                } else {
                    out << static_cast<char>(c);
                }
                break;
        }
    }
    return out.str();
}

}  // namespace

LocSystem::LocSystem(LocSystem::Options options) : options_(options) {
    /// handle ctrl-c
    signal(SIGINT, lightning::debug::SigHandle);
}

LocSystem::~LocSystem() {
    if (loc_) {
        loc_->Finish();
    }
}

bool LocSystem::Init(const std::string &yaml_path) {
    core_initialized_ = false;
    loc_started_ = false;

    loc::Localization::Options opt;
    opt.online_mode_ = true;
    loc_ = std::make_shared<loc::Localization>(opt);

    YAML_IO yaml(yaml_path);

    map_path_ = yaml.GetValue<std::string>("system", "map_path");

    LOG(INFO) << "online mode, creating ros2 node ... ";

    /// subscribers
    node_ = std::make_shared<rclcpp::Node>("lightning_slam");
    LoadRosOutputOptions(yaml_path);
    LoadInitializationOptions(yaml_path);
    ApplyInitializationParameterOverrides();
    CreateRosOutputPublishers();

    imu_topic_ = yaml.GetValue<std::string>("common", "imu_topic");
    cloud_topic_ = yaml.GetValue<std::string>("common", "lidar_topic");
    livox_topic_ = yaml.GetValue<std::string>("common", "livox_lidar_topic");

    rclcpp::QoS qos(10);

    imu_sub_ = node_->create_subscription<sensor_msgs::msg::Imu>(
        imu_topic_, qos, [this](sensor_msgs::msg::Imu::SharedPtr msg) {
            IMUPtr imu = std::make_shared<IMU>();
            imu->timestamp = ToSec(msg->header.stamp);
            imu->linear_acceleration =
                Vec3d(msg->linear_acceleration.x, msg->linear_acceleration.y, msg->linear_acceleration.z);
            imu->angular_velocity = Vec3d(msg->angular_velocity.x, msg->angular_velocity.y, msg->angular_velocity.z);

            ProcessIMU(imu);
        });

    cloud_sub_ = node_->create_subscription<sensor_msgs::msg::PointCloud2>(
        cloud_topic_, qos, [this](sensor_msgs::msg::PointCloud2::SharedPtr cloud) {
            Timer::Evaluate([&]() { ProcessLidar(cloud); }, "Proc Lidar", true);
        });

    livox_sub_ = node_->create_subscription<livox_ros_driver2::msg::CustomMsg>(
        livox_topic_, qos, [this](livox_ros_driver2::msg::CustomMsg ::SharedPtr cloud) {
            Timer::Evaluate([&]() { ProcessLidar(cloud); }, "Proc Lidar", true);
        });

    if (options_.pub_tf_) {
        tf_broadcaster_ = std::make_shared<tf2_ros::TransformBroadcaster>(node_);
        loc_->SetTFCallback(
            [this](const geometry_msgs::msg::TransformStamped &pose) { tf_broadcaster_->sendTransform(pose); });
    }

    loc_->SetResultCallback([this](const loc::LocalizationResult &result) { PublishLocalizationTopics(result); });
    loc_->SetScanCloudCallback(
        [this](CloudPtr scan, const SE3& pose, double timestamp) { PublishScanCloud(scan, pose, timestamp); });
    loc_->SetMapCloudCallback([this](CloudPtr map) { PublishMapCloud(map); });

    bool ret = loc_->Init(yaml_path, map_path_);
    if (!ret) {
        core_initialized_ = false;
        loc_started_ = false;
        LOG(ERROR) << "failed to initialize localization core; initial pose interfaces are disabled.";
        return false;
    }

    core_initialized_ = true;
    CreateInitializationInterfaces();
    LOG(INFO) << "online loc node has been created.";
    return true;
}

void LocSystem::LoadRosOutputOptions(const std::string& yaml_path) {
    YAML::Node root = YAML::LoadFile(yaml_path);
    YAML::Node output = root["ros_output"];
    if (!output) {
        LOG(WARNING) << "ros_output config not found; using built-in ROS output defaults.";
        return;
    }

    ros_output_options_.publish_pose_ =
        ReadYamlValueOr<bool>(output, "publish_pose", ros_output_options_.publish_pose_);
    ros_output_options_.pose_topic_ =
        ReadYamlValueOr<std::string>(output, "pose_topic", ros_output_options_.pose_topic_);

    ros_output_options_.publish_status_ =
        ReadYamlValueOr<bool>(output, "publish_status", ros_output_options_.publish_status_);
    ros_output_options_.status_topic_ =
        ReadYamlValueOr<std::string>(output, "status_topic", ros_output_options_.status_topic_);

    ros_output_options_.publish_diagnostics_ =
        ReadYamlValueOr<bool>(output, "publish_diagnostics", ros_output_options_.publish_diagnostics_);
    ros_output_options_.diagnostics_topic_ =
        ReadYamlValueOr<std::string>(output, "diagnostics_topic", ros_output_options_.diagnostics_topic_);

    ros_output_options_.publish_odometry_ =
        ReadYamlValueOr<bool>(output, "publish_odometry", ros_output_options_.publish_odometry_);
    ros_output_options_.odometry_topic_ =
        ReadYamlValueOr<std::string>(output, "odometry_topic", ros_output_options_.odometry_topic_);

    ros_output_options_.publish_scan_ =
        ReadYamlValueOr<bool>(output, "publish_scan", ros_output_options_.publish_scan_);
    ros_output_options_.scan_topic_ =
        ReadYamlValueOr<std::string>(output, "scan_topic", ros_output_options_.scan_topic_);

    ros_output_options_.publish_map_ =
        ReadYamlValueOr<bool>(output, "publish_map", ros_output_options_.publish_map_);
    ros_output_options_.map_topic_ =
        ReadYamlValueOr<std::string>(output, "map_topic", ros_output_options_.map_topic_);

    ros_output_options_.publish_invalid_result_ =
        ReadYamlValueOr<bool>(output, "publish_invalid_result", ros_output_options_.publish_invalid_result_);
    ros_output_options_.map_frame_ =
        ReadYamlValueOr<std::string>(output, "map_frame", ros_output_options_.map_frame_);
    ros_output_options_.base_frame_ =
        ReadYamlValueOr<std::string>(output, "base_frame", ros_output_options_.base_frame_);

    ros_output_options_.diagnostics_min_period_sec_ =
        ReadYamlValueOr<double>(output, "diagnostics_min_period_sec",
                                ros_output_options_.diagnostics_min_period_sec_);
    ros_output_options_.status_min_period_sec_ =
        ReadYamlValueOr<double>(output, "status_min_period_sec", ros_output_options_.status_min_period_sec_);
    ros_output_options_.scan_min_period_sec_ =
        ReadYamlValueOr<double>(output, "scan_min_period_sec", ros_output_options_.scan_min_period_sec_);
    ros_output_options_.map_min_period_sec_ =
        ReadYamlValueOr<double>(output, "map_min_period_sec", ros_output_options_.map_min_period_sec_);

    ros_output_options_.odometry_covariance_source_ =
        ReadYamlValueOr<std::string>(output, "odometry_covariance_source",
                                     ros_output_options_.odometry_covariance_source_);
    ros_output_options_.odometry_position_covariance_ =
        ReadYamlArray3Or(output, "odometry_position_covariance",
                         ros_output_options_.odometry_position_covariance_);
    ros_output_options_.odometry_orientation_covariance_ =
        ReadYamlArray3Or(output, "odometry_orientation_covariance",
                         ros_output_options_.odometry_orientation_covariance_);
}

void LocSystem::LoadInitializationOptions(const std::string& yaml_path) {
    YAML::Node root = YAML::LoadFile(yaml_path);
    YAML::Node init = root["initialization"];
    if (!init) {
        LOG(WARNING) << "initialization config not found; using built-in initialization defaults.";
        return;
    }

    initialization_options_.source_ =
        NormalizeInitializationSource(ReadYamlValueOr<std::string>(init, "source", initialization_options_.source_));

    YAML::Node default_pose = init["default_pose"];
    initialization_options_.default_pose_enabled_ =
        ReadYamlValueOr<bool>(default_pose, "enabled", initialization_options_.default_pose_enabled_);
    initialization_options_.default_pose_ =
        ReadYamlArray7Or(default_pose, "pose", initialization_options_.default_pose_);

    YAML::Node external_pose = init["external_pose"];
    initialization_options_.external_pose_enabled_ =
        ReadYamlValueOr<bool>(external_pose, "enabled", initialization_options_.external_pose_enabled_);
    initialization_options_.external_pose_service_name_ =
        ReadYamlValueOr<std::string>(external_pose, "service_name",
                                     initialization_options_.external_pose_service_name_);
    initialization_options_.external_pose_accept_frame_id_ =
        ReadYamlValueOr<std::string>(external_pose, "accept_frame_id",
                                     initialization_options_.external_pose_accept_frame_id_);
    initialization_options_.external_pose_require_valid_quaternion_ =
        ReadYamlValueOr<bool>(external_pose, "require_valid_quaternion",
                              initialization_options_.external_pose_require_valid_quaternion_);
    initialization_options_.external_pose_apply_immediately_ =
        ReadYamlValueOr<bool>(external_pose, "apply_immediately",
                              initialization_options_.external_pose_apply_immediately_);

    YAML::Node rviz_initialpose = init["rviz_initialpose"];
    initialization_options_.rviz_initialpose_enabled_ =
        ReadYamlValueOr<bool>(rviz_initialpose, "enabled", initialization_options_.rviz_initialpose_enabled_);
    initialization_options_.rviz_initialpose_topic_ =
        ReadYamlValueOr<std::string>(rviz_initialpose, "topic", initialization_options_.rviz_initialpose_topic_);
    initialization_options_.rviz_initialpose_accept_frame_id_ =
        ReadYamlValueOr<std::string>(rviz_initialpose, "accept_frame_id",
                                     initialization_options_.rviz_initialpose_accept_frame_id_);
    initialization_options_.rviz_initialpose_require_valid_quaternion_ =
        ReadYamlValueOr<bool>(rviz_initialpose, "require_valid_quaternion",
                              initialization_options_.rviz_initialpose_require_valid_quaternion_);
    initialization_options_.rviz_initialpose_apply_immediately_ =
        ReadYamlValueOr<bool>(rviz_initialpose, "apply_immediately",
                              initialization_options_.rviz_initialpose_apply_immediately_);
    initialization_options_.rviz_initialpose_preserve_default_behavior_ =
        ReadYamlValueOr<bool>(rviz_initialpose, "preserve_default_behavior",
                              initialization_options_.rviz_initialpose_preserve_default_behavior_);

    YAML::Node status = init["status"];
    initialization_options_.publish_initialization_status_ =
        ReadYamlValueOr<bool>(status, "publish_initialization_status",
                              initialization_options_.publish_initialization_status_);
    initialization_options_.initialization_status_topic_ =
        ReadYamlValueOr<std::string>(status, "topic", initialization_options_.initialization_status_topic_);
}

void LocSystem::ApplyInitializationParameterOverrides() {
    const auto source =
        node_->declare_parameter<std::string>("initial_pose_source", "");
    const auto initialpose_topic =
        node_->declare_parameter<std::string>("initialpose_topic", "");

    if (!source.empty()) {
        initialization_options_.source_ = NormalizeInitializationSource(source);
    }
    if (!initialpose_topic.empty()) {
        initialization_options_.rviz_initialpose_topic_ = initialpose_topic;
    }
}

void LocSystem::CreateRosOutputPublishers() {
    rclcpp::QoS qos(10);

    if (ros_output_options_.publish_pose_) {
        pose_pub_ = node_->create_publisher<geometry_msgs::msg::PoseStamped>(ros_output_options_.pose_topic_, qos);
    }

    if (ros_output_options_.publish_status_) {
        status_pub_ = node_->create_publisher<std_msgs::msg::String>(ros_output_options_.status_topic_, qos);
    }

    if (ros_output_options_.publish_diagnostics_) {
        diagnostics_pub_ =
            node_->create_publisher<diagnostic_msgs::msg::DiagnosticArray>(ros_output_options_.diagnostics_topic_, qos);
    }

    if (ros_output_options_.publish_odometry_) {
        odometry_pub_ = node_->create_publisher<nav_msgs::msg::Odometry>(ros_output_options_.odometry_topic_, qos);
    }

    if (ros_output_options_.publish_scan_) {
        scan_pub_ =
            node_->create_publisher<sensor_msgs::msg::PointCloud2>(ros_output_options_.scan_topic_,
                                                                   rclcpp::SensorDataQoS());
    }

    if (ros_output_options_.publish_map_) {
        map_pub_ = node_->create_publisher<sensor_msgs::msg::PointCloud2>(
            ros_output_options_.map_topic_, rclcpp::QoS(1).reliable().transient_local());
    }
}

void LocSystem::CreateInitializationInterfaces() {
    rclcpp::QoS qos(10);

    if (initialization_options_.publish_initialization_status_) {
        initialization_status_pub_ =
            node_->create_publisher<std_msgs::msg::String>(initialization_options_.initialization_status_topic_, qos);
    }

    if (initialization_options_.rviz_initialpose_enabled_) {
        initialpose_sub_ = node_->create_subscription<geometry_msgs::msg::PoseWithCovarianceStamped>(
            initialization_options_.rviz_initialpose_topic_, qos,
            [this](geometry_msgs::msg::PoseWithCovarianceStamped::SharedPtr msg) { HandleRvizInitialPose(msg); });
    }

    if (initialization_options_.external_pose_enabled_) {
        set_initial_pose_service_ = node_->create_service<lightning_localization::srv::SetInitialPose>(
            initialization_options_.external_pose_service_name_,
            [this](const std::shared_ptr<lightning_localization::srv::SetInitialPose::Request> request,
                   std::shared_ptr<lightning_localization::srv::SetInitialPose::Response> response) {
                HandleSetInitialPoseService(request, response);
            });
    }
}

void LocSystem::PublishLocalizationTopics(const loc::LocalizationResult& result) {
    const bool publish_pose_like_output = ShouldPublishResult(result);
    {
        std::lock_guard<std::mutex> lock(initialization_mutex_);
        latest_localization_status_ = StatusToString(result.status_);
    }

    if (pose_pub_ && publish_pose_like_output) {
        pose_pub_->publish(MakePoseStamped(result));
    }

    if (odometry_pub_ && publish_pose_like_output) {
        odometry_pub_->publish(MakeOdometry(result));
    }

    if (status_pub_ &&
        ShouldPublishByPeriod(result.timestamp_, ros_output_options_.status_min_period_sec_, last_status_pub_time_)) {
        status_pub_->publish(MakeStatusString(result));
    }

    if (diagnostics_pub_ && ShouldPublishByPeriod(result.timestamp_, ros_output_options_.diagnostics_min_period_sec_,
                                                  last_diagnostics_pub_time_)) {
        diagnostics_pub_->publish(MakeDiagnosticArray(result));
    }

    if (initialization_status_pub_ &&
        ShouldPublishByPeriod(result.timestamp_, ros_output_options_.status_min_period_sec_,
                              last_initialization_status_pub_time_)) {
        PublishInitializationStatus(result.timestamp_);
    }
}

geometry_msgs::msg::PoseStamped LocSystem::MakePoseStamped(const loc::LocalizationResult& result) const {
    geometry_msgs::msg::PoseStamped msg;
    msg.header.stamp = ToRosTime(result.timestamp_);
    msg.header.frame_id = ros_output_options_.map_frame_;

    const auto q = result.pose_.so3().unit_quaternion();
    msg.pose.position.x = result.pose_.translation().x();
    msg.pose.position.y = result.pose_.translation().y();
    msg.pose.position.z = result.pose_.translation().z();
    msg.pose.orientation.x = q.x();
    msg.pose.orientation.y = q.y();
    msg.pose.orientation.z = q.z();
    msg.pose.orientation.w = q.w();
    return msg;
}

std_msgs::msg::String LocSystem::MakeStatusString(const loc::LocalizationResult& result) const {
    std_msgs::msg::String msg;
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(6);
    ss << "status=" << StatusToString(result.status_) << " valid=" << BoolString(result.valid_)
       << " lidar_loc_valid=" << BoolString(result.lidar_loc_valid_) << " confidence=" << result.confidence_
       << " timestamp=" << result.timestamp_;

    if (result.status_ == loc::LocalizationStatus::GOOD && result.valid_) {
        ss << " health=localization_good";
    } else if (result.status_ == loc::LocalizationStatus::INITIALIZING) {
        ss << " health=initializing";
    } else if (result.status_ == loc::LocalizationStatus::FOLLOWING_DR) {
        ss << " health=following_dead_reckoning";
    } else if (result.status_ == loc::LocalizationStatus::FAIL) {
        ss << " health=localization_failed";
    } else if (!result.valid_) {
        ss << " health=invalid_result_not_published_to_pose_by_default";
    } else {
        ss << " health=idle";
    }

    msg.data = ss.str();
    return msg;
}

diagnostic_msgs::msg::DiagnosticArray LocSystem::MakeDiagnosticArray(const loc::LocalizationResult& result) const {
    diagnostic_msgs::msg::DiagnosticArray array;
    array.header.stamp = ToRosTime(result.timestamp_);

    diagnostic_msgs::msg::DiagnosticStatus status;
    status.name = "lightning_localization/localization";
    status.hardware_id = "lightning_localization";

    if (result.status_ == loc::LocalizationStatus::GOOD && result.valid_) {
        status.level = diagnostic_msgs::msg::DiagnosticStatus::OK;
        status.message = "localization good";
    } else if (result.status_ == loc::LocalizationStatus::FAIL) {
        status.level = diagnostic_msgs::msg::DiagnosticStatus::ERROR;
        status.message = "localization failed";
    } else {
        status.level = diagnostic_msgs::msg::DiagnosticStatus::WARN;
        status.message = result.valid_ ? "localization degraded" : "localization result invalid";
    }

    AddDiagnosticValue(status, "status", StatusToString(result.status_));
    AddDiagnosticValue(status, "valid", BoolString(result.valid_));
    AddDiagnosticValue(status, "lidar_loc_valid", BoolString(result.lidar_loc_valid_));
    AddDiagnosticValue(status, "confidence", std::to_string(result.confidence_));
    AddDiagnosticValue(status, "lidar_loc_odom_delta", std::to_string(result.lidar_loc_odom_delta_));
    AddDiagnosticValue(status, "lidar_loc_odom_error_normal", BoolString(result.lidar_loc_odom_error_normal_));
    AddDiagnosticValue(status, "lidar_loc_smooth_flag", BoolString(result.lidar_loc_smooth_flag_));
    AddDiagnosticValue(status, "is_parking", std::to_string(result.is_parking_));
    AddDiagnosticValue(status, "timestamp", std::to_string(result.timestamp_));
    AddDiagnosticValue(status, "frame_id", ros_output_options_.map_frame_);
    AddDiagnosticValue(status, "child_frame_id", ros_output_options_.base_frame_);
    AddDiagnosticValue(status, "map_path", map_path_);
    AddDiagnosticValue(status, "odometry_covariance_source", ros_output_options_.odometry_covariance_source_);
    AddDiagnosticValue(status, "note",
                       "confidence is inherited from NDT transformation probability, not a unified industrial score");

    {
        std::lock_guard<std::mutex> lock(initialization_mutex_);
        AddDiagnosticValue(status, "initialization_source", initialization_state_.current_source_);
        AddDiagnosticValue(status, "last_initialization_source", initialization_state_.last_request_source_);
        AddDiagnosticValue(status, "last_initialization_accepted",
                           BoolString(initialization_state_.last_request_accepted_));
        AddDiagnosticValue(status, "last_initialization_applied",
                           BoolString(initialization_state_.last_request_applied_));
        AddDiagnosticValue(status, "last_initialization_message", initialization_state_.last_message_);
        AddDiagnosticValue(status, "waiting_for_initial_pose",
                           BoolString(initialization_state_.waiting_for_initial_pose_));
        AddDiagnosticValue(status, "initialpose_topic", initialization_options_.rviz_initialpose_topic_);
        AddDiagnosticValue(status, "set_initial_pose_service",
                           initialization_options_.external_pose_service_name_);
    }

    array.status.push_back(std::move(status));
    return array;
}

nav_msgs::msg::Odometry LocSystem::MakeOdometry(const loc::LocalizationResult& result) const {
    nav_msgs::msg::Odometry msg;
    msg.header.stamp = ToRosTime(result.timestamp_);
    msg.header.frame_id = ros_output_options_.map_frame_;
    msg.child_frame_id = ros_output_options_.base_frame_;

    msg.pose.pose = MakePoseStamped(result).pose;
    msg.pose.covariance.fill(0.0);
    msg.pose.covariance[0] = ros_output_options_.odometry_position_covariance_[0];
    msg.pose.covariance[7] = ros_output_options_.odometry_position_covariance_[1];
    msg.pose.covariance[14] = ros_output_options_.odometry_position_covariance_[2];
    msg.pose.covariance[21] = ros_output_options_.odometry_orientation_covariance_[0];
    msg.pose.covariance[28] = ros_output_options_.odometry_orientation_covariance_[1];
    msg.pose.covariance[35] = ros_output_options_.odometry_orientation_covariance_[2];

    return msg;
}

sensor_msgs::msg::PointCloud2 LocSystem::MakePointCloud2(const CloudPtr& cloud, const std::string& frame_id,
                                                         double timestamp) const {
    sensor_msgs::msg::PointCloud2 msg;
    if (cloud) {
        pcl::toROSMsg(*cloud, msg);
    }
    msg.header.frame_id = frame_id;
    if (timestamp > 0.0) {
        msg.header.stamp = ToRosTime(timestamp);
    } else if (node_) {
        msg.header.stamp = node_->now();
    }
    return msg;
}

void LocSystem::PublishScanCloud(const CloudPtr& scan, const SE3& pose, double timestamp) {
    if (!scan_pub_ || !scan || scan->empty()) {
        return;
    }

    if (!ShouldPublishByPeriod(timestamp, ros_output_options_.scan_min_period_sec_, last_scan_pub_time_)) {
        return;
    }

    CloudPtr scan_world(new PointCloudType);
    pcl::transformPointCloud(*scan, *scan_world, pose.matrix().cast<float>());
    scan_pub_->publish(MakePointCloud2(scan_world, ros_output_options_.map_frame_, timestamp));
}

void LocSystem::PublishMapCloud(const CloudPtr& map) {
    if (!map_pub_ || !map || map->empty()) {
        return;
    }

    const double timestamp = node_ ? node_->now().seconds() : 0.0;
    if (!ShouldPublishByPeriod(timestamp, ros_output_options_.map_min_period_sec_, last_map_pub_time_)) {
        return;
    }

    map_pub_->publish(MakePointCloud2(map, ros_output_options_.map_frame_, timestamp));
}

builtin_interfaces::msg::Time LocSystem::ToRosTime(double timestamp) const {
    builtin_interfaces::msg::Time stamp;
    if (timestamp <= 0.0) {
        stamp.sec = 0;
        stamp.nanosec = 0;
        return stamp;
    }

    const double sec_floor = std::floor(timestamp);
    stamp.sec = static_cast<int32_t>(sec_floor);
    stamp.nanosec = static_cast<uint32_t>(std::llround((timestamp - sec_floor) * 1e9));
    if (stamp.nanosec >= 1000000000U) {
        stamp.sec += 1;
        stamp.nanosec -= 1000000000U;
    }
    return stamp;
}

std::string LocSystem::StatusToString(loc::LocalizationStatus status) const {
    switch (status) {
        case loc::LocalizationStatus::IDLE:
            return "IDLE";
        case loc::LocalizationStatus::INITIALIZING:
            return "INITIALIZING";
        case loc::LocalizationStatus::GOOD:
            return "GOOD";
        case loc::LocalizationStatus::FOLLOWING_DR:
            return "FOLLOWING_DR";
        case loc::LocalizationStatus::FAIL:
            return "FAIL";
    }
    return "UNKNOWN";
}

bool LocSystem::ShouldPublishResult(const loc::LocalizationResult& result) const {
    return ros_output_options_.publish_invalid_result_ || result.valid_;
}

bool LocSystem::ShouldPublishByPeriod(double timestamp, double min_period_sec, double& last_pub_time) const {
    const double now = timestamp > 0.0 ? timestamp : node_->now().seconds();
    if (min_period_sec <= 0.0 || !std::isfinite(last_pub_time) || now - last_pub_time >= min_period_sec) {
        last_pub_time = now;
        return true;
    }
    return false;
}

void LocSystem::ApplyConfiguredInitialPose() {
    if (!core_initialized_.load()) {
        const double timestamp = node_ ? node_->now().seconds() : 0.0;
        {
            std::lock_guard<std::mutex> lock(initialization_mutex_);
            initialization_state_.last_request_source_ = initialization_options_.source_;
            initialization_state_.last_request_time_ = timestamp;
            initialization_state_.last_request_frame_ = ros_output_options_.map_frame_;
            initialization_state_.last_request_accepted_ = false;
            initialization_state_.last_request_applied_ = false;
            initialization_state_.last_message_ =
                "localization core is not initialized; startup initial pose was skipped";
            initialization_state_.waiting_for_initial_pose_ = true;
        }
        PublishInitializationStatus(timestamp);
        return;
    }

    const std::string source = NormalizeInitializationSource(initialization_options_.source_);

    if (source == "default") {
        if (initialization_options_.default_pose_enabled_) {
            std::string message;
            ApplyInitialPose(PoseArrayToSE3(initialization_options_.default_pose_), "default",
                             ros_output_options_.map_frame_, true, &message);
        } else {
            {
                std::lock_guard<std::mutex> lock(initialization_mutex_);
                initialization_state_.current_source_ = "default";
                initialization_state_.waiting_for_initial_pose_ = true;
                initialization_state_.last_message_ =
                    "default initialization source selected but default_pose is disabled";
            }
            PublishInitializationStatus(node_ ? node_->now().seconds() : 0.0);
        }
        return;
    }

    if (source == "rviz_initialpose") {
        if (initialization_options_.rviz_initialpose_preserve_default_behavior_ &&
            initialization_options_.default_pose_enabled_) {
            std::string message;
            ApplyInitialPose(PoseArrayToSE3(initialization_options_.default_pose_),
                             "rviz_initialpose_default_fallback", ros_output_options_.map_frame_, true, &message);
            return;
        }

        {
            std::lock_guard<std::mutex> lock(initialization_mutex_);
            loc_started_ = false;
            initialization_state_.current_source_ = "rviz_initialpose";
            initialization_state_.waiting_for_initial_pose_ = true;
            initialization_state_.last_message_ = "waiting for RViz /initialpose";
        }
        PublishInitializationStatus(node_ ? node_->now().seconds() : 0.0);
        return;
    }

    if (source == "external_pose") {
        {
            std::lock_guard<std::mutex> lock(initialization_mutex_);
            loc_started_ = false;
            initialization_state_.current_source_ = "external_pose";
            initialization_state_.waiting_for_initial_pose_ = true;
            initialization_state_.last_message_ = "waiting for external set initial pose service request";
        }
        PublishInitializationStatus(node_ ? node_->now().seconds() : 0.0);
        return;
    }

    if (source == "functional_point") {
        {
            std::lock_guard<std::mutex> lock(initialization_mutex_);
            loc_started_ = true;
            initialization_state_.current_source_ = "functional_point";
            initialization_state_.waiting_for_initial_pose_ = false;
            initialization_state_.last_message_ =
                "functional point initialization selected; success depends on map functional points and later matching";
        }
        PublishInitializationStatus(node_ ? node_->now().seconds() : 0.0);
        return;
    }

    LOG(WARNING) << "unknown initialization.source '" << initialization_options_.source_
                 << "', falling back to default initial pose";
    std::string message;
    ApplyInitialPose(PoseArrayToSE3(initialization_options_.default_pose_), "default", ros_output_options_.map_frame_,
                     true, &message);
}

void LocSystem::SetInitPose(const SE3 &pose) {
    std::string message;
    ApplyInitialPose(pose, "default", ros_output_options_.map_frame_, true, &message);
}

void LocSystem::PublishInitializationStatus(double timestamp) {
    if (!initialization_status_pub_) {
        return;
    }

    initialization_status_pub_->publish(MakeInitializationStatusString(timestamp));
}

std_msgs::msg::String LocSystem::MakeInitializationStatusString(double timestamp) const {
    std_msgs::msg::String msg;
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(6);

    std::lock_guard<std::mutex> lock(initialization_mutex_);
    ss << "{"
       << "\"current_initialization_source\":\"" << JsonEscape(initialization_state_.current_source_) << "\","
       << "\"last_request_source\":\"" << JsonEscape(initialization_state_.last_request_source_) << "\","
       << "\"last_request_time\":" << initialization_state_.last_request_time_ << ","
       << "\"last_request_frame\":\"" << JsonEscape(initialization_state_.last_request_frame_) << "\","
       << "\"last_request_accepted\":" << BoolString(initialization_state_.last_request_accepted_) << ","
       << "\"last_request_applied\":" << BoolString(initialization_state_.last_request_applied_) << ","
       << "\"last_message\":\"" << JsonEscape(initialization_state_.last_message_) << "\","
       << "\"localization_status\":\"" << JsonEscape(latest_localization_status_) << "\","
       << "\"waiting_for_initial_pose\":" << BoolString(initialization_state_.waiting_for_initial_pose_) << ","
       << "\"external_pose_service_enabled\":" << BoolString(initialization_options_.external_pose_enabled_) << ","
       << "\"rviz_initialpose_enabled\":" << BoolString(initialization_options_.rviz_initialpose_enabled_) << ","
       << "\"initialpose_topic\":\"" << JsonEscape(initialization_options_.rviz_initialpose_topic_) << "\","
       << "\"set_initial_pose_service\":\"" << JsonEscape(initialization_options_.external_pose_service_name_) << "\","
       << "\"core_initialized\":" << BoolString(core_initialized_.load()) << ","
       << "\"timestamp\":" << timestamp << "}";

    msg.data = ss.str();
    return msg;
}

std::string LocSystem::NormalizeInitializationSource(const std::string& source) const {
    const std::string normalized = LowerString(source);
    if (normalized == "default" || normalized == "external_pose" || normalized == "rviz_initialpose" ||
        normalized == "functional_point") {
        return normalized;
    }
    return source;
}

SE3 LocSystem::PoseArrayToSE3(const std::array<double, 7>& pose) const {
    Eigen::Quaterniond q(pose[6], pose[3], pose[4], pose[5]);
    if (q.norm() <= 1e-9 || !std::isfinite(q.norm())) {
        q = Eigen::Quaterniond::Identity();
    } else {
        q.normalize();
    }
    return SE3(q, Eigen::Vector3d(pose[0], pose[1], pose[2]));
}

bool LocSystem::ApplyInitialPose(const SE3& pose, const std::string& source, const std::string& frame_id,
                                 bool apply_immediately, std::string* message) {
    std::string local_message;
    const double timestamp = node_ ? node_->now().seconds() : 0.0;

    if (!core_initialized_.load()) {
        local_message = "localization core is not initialized; initial pose was rejected";
        {
            std::lock_guard<std::mutex> lock(initialization_mutex_);
            initialization_state_.last_request_source_ = source;
            initialization_state_.last_request_time_ = timestamp;
            initialization_state_.last_request_frame_ = frame_id;
            initialization_state_.last_request_accepted_ = false;
            initialization_state_.last_request_applied_ = false;
            initialization_state_.last_message_ = local_message;
            initialization_state_.waiting_for_initial_pose_ = true;
        }
        if (message) {
            *message = local_message;
        }
        PublishInitializationStatus(timestamp);
        return false;
    }

    if (!apply_immediately) {
        local_message =
            "initial pose accepted but not applied because apply_immediately is false; no deferred queue is executed";
        {
            std::lock_guard<std::mutex> lock(initialization_mutex_);
            initialization_state_.current_source_ = source;
            initialization_state_.last_request_source_ = source;
            initialization_state_.last_request_time_ = timestamp;
            initialization_state_.last_request_frame_ = frame_id;
            initialization_state_.last_request_accepted_ = true;
            initialization_state_.last_request_applied_ = false;
            initialization_state_.last_message_ = local_message;
            initialization_state_.waiting_for_initial_pose_ = true;
        }
        if (message) {
            *message = local_message;
        }
        PublishInitializationStatus(timestamp);
        return true;
    }

    if (!loc_) {
        local_message = "localization object is not initialized";
        {
            std::lock_guard<std::mutex> lock(initialization_mutex_);
            initialization_state_.last_request_source_ = source;
            initialization_state_.last_request_time_ = timestamp;
            initialization_state_.last_request_frame_ = frame_id;
            initialization_state_.last_request_accepted_ = false;
            initialization_state_.last_request_applied_ = false;
            initialization_state_.last_message_ = local_message;
        }
        if (message) {
            *message = local_message;
        }
        PublishInitializationStatus(timestamp);
        return false;
    }

    LOG(INFO) << "set init pose from " << source << ": " << pose.translation().transpose() << ", "
              << pose.unit_quaternion().coeffs().transpose();

    loc_->SetExternalPose(pose.unit_quaternion(), pose.translation());
    loc_started_ = true;

    local_message =
        "initial pose accepted and applied as initial guess; localization success still depends on later matching";
    {
        std::lock_guard<std::mutex> lock(initialization_mutex_);
        initialization_state_.current_source_ = source;
        initialization_state_.last_request_source_ = source;
        initialization_state_.last_request_time_ = timestamp;
        initialization_state_.last_request_frame_ = frame_id;
        initialization_state_.last_request_accepted_ = true;
        initialization_state_.last_request_applied_ = true;
        initialization_state_.last_message_ = local_message;
        initialization_state_.waiting_for_initial_pose_ = false;
    }

    if (message) {
        *message = local_message;
    }
    PublishInitializationStatus(timestamp);
    return true;
}

bool LocSystem::ValidateInitialPose(const geometry_msgs::msg::Pose& pose, const std::string& frame_id,
                                    const std::string& accept_frame_id, bool require_unit_quaternion,
                                    Eigen::Quaterniond* q, Eigen::Vector3d* t, std::string* message) const {
    if (frame_id.empty()) {
        if (message) {
            *message = "initial pose frame_id is empty";
        }
        return false;
    }

    if (!accept_frame_id.empty() && frame_id != accept_frame_id) {
        if (message) {
            *message = "initial pose frame_id '" + frame_id + "' does not match required frame '" + accept_frame_id +
                       "'";
        }
        return false;
    }

    const std::array<double, 7> values = {pose.position.x,    pose.position.y,    pose.position.z,
                                         pose.orientation.x, pose.orientation.y, pose.orientation.z,
                                         pose.orientation.w};
    for (double value : values) {
        if (!std::isfinite(value)) {
            if (message) {
                *message = "initial pose contains NaN or Inf";
            }
            return false;
        }
    }

    Eigen::Quaterniond quat(pose.orientation.w, pose.orientation.x, pose.orientation.y, pose.orientation.z);
    const double norm = quat.norm();
    if (norm <= 1e-9) {
        if (message) {
            *message = "initial pose quaternion is zero";
        }
        return false;
    }

    if (require_unit_quaternion && std::abs(norm - 1.0) > 1e-3) {
        if (message) {
            *message = "initial pose quaternion is not unit length";
        }
        return false;
    }

    quat.normalize();
    if (q) {
        *q = quat;
    }
    if (t) {
        *t = Eigen::Vector3d(pose.position.x, pose.position.y, pose.position.z);
    }
    if (message) {
        *message = "initial pose is valid";
    }
    return true;
}

void LocSystem::HandleRvizInitialPose(const geometry_msgs::msg::PoseWithCovarianceStamped::SharedPtr& msg) {
    Eigen::Quaterniond q;
    Eigen::Vector3d t;
    std::string message;

    const bool valid = ValidateInitialPose(msg->pose.pose, msg->header.frame_id,
                                           initialization_options_.rviz_initialpose_accept_frame_id_,
                                           initialization_options_.rviz_initialpose_require_valid_quaternion_, &q, &t,
                                           &message);
    if (!valid) {
        const double timestamp = node_ ? node_->now().seconds() : 0.0;
        {
            std::lock_guard<std::mutex> lock(initialization_mutex_);
            initialization_state_.last_request_source_ = "rviz_initialpose_topic";
            initialization_state_.last_request_time_ = timestamp;
            initialization_state_.last_request_frame_ = msg->header.frame_id;
            initialization_state_.last_request_accepted_ = false;
            initialization_state_.last_request_applied_ = false;
            initialization_state_.last_message_ = message;
        }
        PublishInitializationStatus(timestamp);
        return;
    }

    ApplyInitialPose(SE3(q, t), "rviz_initialpose_topic", msg->header.frame_id,
                     initialization_options_.rviz_initialpose_apply_immediately_, &message);
}

void LocSystem::HandleSetInitialPoseService(
    const std::shared_ptr<lightning_localization::srv::SetInitialPose::Request> request,
    std::shared_ptr<lightning_localization::srv::SetInitialPose::Response> response) {
    Eigen::Quaterniond q;
    Eigen::Vector3d t;
    std::string message;

    const bool valid = ValidateInitialPose(request->pose, request->header.frame_id,
                                           initialization_options_.external_pose_accept_frame_id_,
                                           initialization_options_.external_pose_require_valid_quaternion_, &q, &t,
                                           &message);
    if (!valid) {
        const double timestamp = node_ ? node_->now().seconds() : 0.0;
        {
            std::lock_guard<std::mutex> lock(initialization_mutex_);
            initialization_state_.last_request_source_ =
                request->source.empty() ? "external_pose_service" : request->source;
            initialization_state_.last_request_time_ = timestamp;
            initialization_state_.last_request_frame_ = request->header.frame_id;
            initialization_state_.last_request_accepted_ = false;
            initialization_state_.last_request_applied_ = false;
            initialization_state_.last_message_ = message;
        }
        response->success = false;
        response->message = message;
        response->status = 0;
        PublishInitializationStatus(timestamp);
        return;
    }

    const std::string source = request->source.empty() ? "external_pose_service" : request->source;
    const bool apply_immediately = request->apply_immediately && initialization_options_.external_pose_apply_immediately_;
    const bool applied = ApplyInitialPose(SE3(q, t), source, request->header.frame_id, apply_immediately, &message);

    response->success = applied;
    response->message = message;
    response->status = applied ? (apply_immediately ? 1 : 2) : 0;
}

void LocSystem::ProcessIMU(const IMUPtr &imu) {
    if (loc_started_) {
        loc_->ProcessIMUMsg(imu);
    }
}

void LocSystem::ProcessLidar(const sensor_msgs::msg::PointCloud2::SharedPtr &cloud) {
    if (loc_started_) {
        loc_->ProcessLidarMsg(cloud);
    }
}

void LocSystem::ProcessLidar(const livox_ros_driver2::msg::CustomMsg::SharedPtr &cloud) {
    if (loc_started_) {
        loc_->ProcessLivoxLidarMsg(cloud);
    }
}

void LocSystem::Spin() {
    if (node_ != nullptr) {
        spin(node_);
    }
}

}  // namespace lightning
