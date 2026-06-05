//
// Created by xiang on 25-9-8.
//

#ifndef LIGHTNING_LOC_SYSTEM_H
#define LIGHTNING_LOC_SYSTEM_H

#include <array>
#include <atomic>
#include <limits>
#include <memory>
#include <mutex>
#include <string>

#include <builtin_interfaces/msg/time.hpp>
#include <diagnostic_msgs/msg/diagnostic_array.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <geometry_msgs/msg/pose_with_covariance_stamped.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <std_msgs/msg/string.hpp>
#include <tf2_ros/transform_broadcaster.h>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>

#include "lightning_localization/srv/set_initial_pose.hpp"
#include "livox_ros_driver2/msg/custom_msg.hpp"

#include "common/eigen_types.h"
#include "common/imu.h"
#include "common/keyframe.h"

namespace lightning {

namespace loc {
class Localization;
struct LocalizationResult;
enum class LocalizationStatus;
}

class LocSystem {
   public:
    struct Options {
        bool pub_tf_ = true;  // 是否发布tf
    };

    explicit LocSystem(Options options);
    ~LocSystem();

    /// 初始化，地图路径在yaml里配置
    bool Init(const std::string& yaml_path);

    /// 设置初始化位姿
    void SetInitPose(const SE3& pose);

    /// 根据配置选择启动初始化来源
    void ApplyConfiguredInitialPose();

    /// 处理IMU
    void ProcessIMU(const lightning::IMUPtr& imu);

    /// 处理点云
    void ProcessLidar(const sensor_msgs::msg::PointCloud2::SharedPtr& cloud);
    void ProcessLidar(const livox_ros_driver2::msg::CustomMsg::SharedPtr& cloud);

    /// 实时模式下的spin
    void Spin();

   private:
    struct RosOutputOptions {
        bool publish_pose_ = true;
        std::string pose_topic_ = "/localization/pose";

        bool publish_status_ = true;
        std::string status_topic_ = "/localization/status";

        bool publish_diagnostics_ = true;
        std::string diagnostics_topic_ = "/localization/diagnostics";

        bool publish_odometry_ = false;
        std::string odometry_topic_ = "/localization/odometry";

        bool publish_invalid_result_ = false;
        std::string map_frame_ = "map";
        std::string base_frame_ = "base_link";

        double diagnostics_min_period_sec_ = 0.1;
        double status_min_period_sec_ = 0.1;

        std::string odometry_covariance_source_ = "static_config_placeholder";
        std::array<double, 3> odometry_position_covariance_ = {1.0, 1.0, 1.0};
        std::array<double, 3> odometry_orientation_covariance_ = {0.5, 0.5, 0.5};
    };

    struct InitializationOptions {
        std::string source_ = "default";

        bool default_pose_enabled_ = true;
        std::array<double, 7> default_pose_ = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0};

        bool external_pose_enabled_ = true;
        std::string external_pose_service_name_ = "/lightning_localization/set_initial_pose";
        std::string external_pose_accept_frame_id_ = "map";
        bool external_pose_require_valid_quaternion_ = true;
        bool external_pose_apply_immediately_ = true;

        bool rviz_initialpose_enabled_ = true;
        std::string rviz_initialpose_topic_ = "/initialpose";
        std::string rviz_initialpose_accept_frame_id_ = "map";
        bool rviz_initialpose_require_valid_quaternion_ = true;
        bool rviz_initialpose_apply_immediately_ = true;
        bool rviz_initialpose_preserve_default_behavior_ = true;

        bool publish_initialization_status_ = true;
        std::string initialization_status_topic_ = "/localization/initialization_status";
    };

    struct InitializationRequestState {
        std::string current_source_ = "default";
        std::string last_request_source_ = "none";
        double last_request_time_ = 0.0;
        std::string last_request_frame_ = "";
        bool last_request_accepted_ = false;
        bool last_request_applied_ = false;
        std::string last_message_ = "no initialization request received";
        bool waiting_for_initial_pose_ = false;
    };

    void LoadRosOutputOptions(const std::string& yaml_path);
    void LoadInitializationOptions(const std::string& yaml_path);
    void ApplyInitializationParameterOverrides();
    void CreateRosOutputPublishers();
    void CreateInitializationInterfaces();
    void PublishLocalizationTopics(const loc::LocalizationResult& result);
    void PublishInitializationStatus(double timestamp);
    geometry_msgs::msg::PoseStamped MakePoseStamped(const loc::LocalizationResult& result) const;
    std_msgs::msg::String MakeStatusString(const loc::LocalizationResult& result) const;
    std_msgs::msg::String MakeInitializationStatusString(double timestamp) const;
    diagnostic_msgs::msg::DiagnosticArray MakeDiagnosticArray(const loc::LocalizationResult& result) const;
    nav_msgs::msg::Odometry MakeOdometry(const loc::LocalizationResult& result) const;
    builtin_interfaces::msg::Time ToRosTime(double timestamp) const;
    std::string StatusToString(loc::LocalizationStatus status) const;
    std::string NormalizeInitializationSource(const std::string& source) const;
    SE3 PoseArrayToSE3(const std::array<double, 7>& pose) const;
    bool ApplyInitialPose(const SE3& pose, const std::string& source, const std::string& frame_id,
                          bool apply_immediately, std::string* message);
    bool ValidateInitialPose(const geometry_msgs::msg::Pose& pose, const std::string& frame_id,
                             const std::string& accept_frame_id, bool require_unit_quaternion,
                             Eigen::Quaterniond* q, Eigen::Vector3d* t, std::string* message) const;
    void HandleRvizInitialPose(const geometry_msgs::msg::PoseWithCovarianceStamped::SharedPtr& msg);
    void HandleSetInitialPoseService(
        const std::shared_ptr<lightning_localization::srv::SetInitialPose::Request> request,
        std::shared_ptr<lightning_localization::srv::SetInitialPose::Response> response);
    bool ShouldPublishResult(const loc::LocalizationResult& result) const;
    bool ShouldPublishByPeriod(double timestamp, double min_period_sec, double& last_pub_time) const;

    Options options_;
    RosOutputOptions ros_output_options_;
    InitializationOptions initialization_options_;
    InitializationRequestState initialization_state_;
    std::string latest_localization_status_ = "IDLE";
    mutable std::mutex initialization_mutex_;

    std::shared_ptr<loc::Localization> loc_ = nullptr;  // 定位接口

    std::atomic_bool loc_started_ = false;          // 是否开启定位
    std::atomic_bool core_initialized_ = false;     // 定位核心是否初始化成功
    std::atomic_bool map_loaded_ = false;           // 地图是否已载入

    /// 实时模式下的ros2 node, subscribers
    rclcpp::Node::SharedPtr node_;
    std::shared_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster_ = nullptr;

    std::string imu_topic_;
    std::string cloud_topic_;
    std::string livox_topic_;
    std::string map_path_;

    double last_status_pub_time_ = -std::numeric_limits<double>::infinity();
    double last_diagnostics_pub_time_ = -std::numeric_limits<double>::infinity();
    double last_initialization_status_pub_time_ = -std::numeric_limits<double>::infinity();

    rclcpp::Subscription<sensor_msgs::msg::Imu>::SharedPtr imu_sub_ = nullptr;
    rclcpp::Subscription<sensor_msgs::msg::PointCloud2>::SharedPtr cloud_sub_ = nullptr;
    rclcpp::Subscription<livox_ros_driver2::msg::CustomMsg>::SharedPtr livox_sub_ = nullptr;
    rclcpp::Subscription<geometry_msgs::msg::PoseWithCovarianceStamped>::SharedPtr initialpose_sub_ = nullptr;

    rclcpp::Service<lightning_localization::srv::SetInitialPose>::SharedPtr set_initial_pose_service_ = nullptr;

    rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr pose_pub_ = nullptr;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr status_pub_ = nullptr;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr initialization_status_pub_ = nullptr;
    rclcpp::Publisher<diagnostic_msgs::msg::DiagnosticArray>::SharedPtr diagnostics_pub_ = nullptr;
    rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odometry_pub_ = nullptr;
};

};  // namespace lightning

#endif  // LIGHTNING_LOC_SYSTEM_H

