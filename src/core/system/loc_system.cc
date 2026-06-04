//
// Created by xiang on 25-9-12.
//

#include "core/system/loc_system.h"
#include "core/localization/localization.h"
#include "io/yaml_io.h"
#include "wrapper/ros_utils.h"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <utility>

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

void AddDiagnosticValue(diagnostic_msgs::msg::DiagnosticStatus& status, const std::string& key,
                        const std::string& value) {
    diagnostic_msgs::msg::KeyValue pair;
    pair.key = key;
    pair.value = value;
    status.values.push_back(std::move(pair));
}

std::string BoolString(bool value) { return value ? "true" : "false"; }

}  // namespace

LocSystem::LocSystem(LocSystem::Options options) : options_(options) {
    /// handle ctrl-c
    signal(SIGINT, lightning::debug::SigHandle);
}

LocSystem::~LocSystem() { loc_->Finish(); }

bool LocSystem::Init(const std::string &yaml_path) {
    loc::Localization::Options opt;
    opt.online_mode_ = true;
    loc_ = std::make_shared<loc::Localization>(opt);

    YAML_IO yaml(yaml_path);

    map_path_ = yaml.GetValue<std::string>("system", "map_path");

    LOG(INFO) << "online mode, creating ros2 node ... ";

    /// subscribers
    node_ = std::make_shared<rclcpp::Node>("lightning_slam");
    LoadRosOutputOptions(yaml_path);
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

    bool ret = loc_->Init(yaml_path, map_path_);
    if (ret) {
        LOG(INFO) << "online loc node has been created.";
    }

    return ret;
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
}

void LocSystem::PublishLocalizationTopics(const loc::LocalizationResult& result) {
    const bool publish_pose_like_output = ShouldPublishResult(result);

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

void LocSystem::SetInitPose(const SE3 &pose) {
    LOG(INFO) << "set init pose: " << pose.translation().transpose() << ", "
              << pose.unit_quaternion().coeffs().transpose();

    loc_->SetExternalPose(pose.unit_quaternion(), pose.translation());
    loc_started_ = true;
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
