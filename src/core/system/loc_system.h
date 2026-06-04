//
// Created by xiang on 25-9-8.
//

#ifndef LIGHTNING_LOC_SYSTEM_H
#define LIGHTNING_LOC_SYSTEM_H

#include <array>
#include <atomic>
#include <limits>
#include <string>

#include <builtin_interfaces/msg/time.hpp>
#include <diagnostic_msgs/msg/diagnostic_array.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <std_msgs/msg/string.hpp>
#include <tf2_ros/transform_broadcaster.h>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>

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

    void LoadRosOutputOptions(const std::string& yaml_path);
    void CreateRosOutputPublishers();
    void PublishLocalizationTopics(const loc::LocalizationResult& result);
    geometry_msgs::msg::PoseStamped MakePoseStamped(const loc::LocalizationResult& result) const;
    std_msgs::msg::String MakeStatusString(const loc::LocalizationResult& result) const;
    diagnostic_msgs::msg::DiagnosticArray MakeDiagnosticArray(const loc::LocalizationResult& result) const;
    nav_msgs::msg::Odometry MakeOdometry(const loc::LocalizationResult& result) const;
    builtin_interfaces::msg::Time ToRosTime(double timestamp) const;
    std::string StatusToString(loc::LocalizationStatus status) const;
    bool ShouldPublishResult(const loc::LocalizationResult& result) const;
    bool ShouldPublishByPeriod(double timestamp, double min_period_sec, double& last_pub_time) const;

    Options options_;
    RosOutputOptions ros_output_options_;

    std::shared_ptr<loc::Localization> loc_ = nullptr;  // 定位接口

    std::atomic_bool loc_started_ = false;  // 是否开启定位
    std::atomic_bool map_loaded_ = false;   // 地图是否已载入

    /// 实时模式下的ros2 node, subscribers
    rclcpp::Node::SharedPtr node_;
    std::shared_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster_ = nullptr;

    std::string imu_topic_;
    std::string cloud_topic_;
    std::string livox_topic_;
    std::string map_path_;

    double last_status_pub_time_ = -std::numeric_limits<double>::infinity();
    double last_diagnostics_pub_time_ = -std::numeric_limits<double>::infinity();

    rclcpp::Subscription<sensor_msgs::msg::Imu>::SharedPtr imu_sub_ = nullptr;
    rclcpp::Subscription<sensor_msgs::msg::PointCloud2>::SharedPtr cloud_sub_ = nullptr;
    rclcpp::Subscription<livox_ros_driver2::msg::CustomMsg>::SharedPtr livox_sub_ = nullptr;

    rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr pose_pub_ = nullptr;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr status_pub_ = nullptr;
    rclcpp::Publisher<diagnostic_msgs::msg::DiagnosticArray>::SharedPtr diagnostics_pub_ = nullptr;
    rclcpp::Publisher<nav_msgs::msg::Odometry>::SharedPtr odometry_pub_ = nullptr;
};

};  // namespace lightning

#endif  // LIGHTNING_LOC_SYSTEM_H

