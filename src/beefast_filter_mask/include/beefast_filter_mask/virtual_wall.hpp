#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <geometry_msgs/msg/point.hpp>
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "nav2_util/lifecycle_node.hpp"
#include "nav2_util/node_thread.hpp"
#include "nav_msgs/msg/occupancy_grid.hpp"
#include "tf2_ros/transform_listener.h"
#include "visualization_msgs/msg/marker_array.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include <tf2_ros/buffer.h>
#include <mutex>

#include "std_msgs/msg/string.hpp"
#include "beefast_filter_mask/costmap.hpp"

namespace beefast_filter_mask
{
    // 坐标结构体
    struct Coordinate {
        double x;
        double y;
    };
    /*禁行区域定义
    */
    struct ZoneData
    {
        int map_id;
        int zone_id;
        int zone_type; // 1-线段；2-三角形；3-长方形；4-多边形
        std::vector<Coordinate> coordinates;
    };

    class VirtualWall : public nav2_util::LifecycleNode
    {
    public:
        VirtualWall();        
    protected:
        /**
         * @brief Sets up required params and services. Loads map and its parameters from the file
         * @param state Lifecycle Node's state
         * @return Success or Failure
         */
        nav2_util::CallbackReturn on_configure(const rclcpp_lifecycle::State & state) override;
        /**
         * @brief Start publishing the map using the latched topic
         * @param state Lifecycle Node's state
         * @return Success or Failure
         */
        nav2_util::CallbackReturn on_activate(const rclcpp_lifecycle::State & state) override;
        /**
         * @brief Stops publishing the latched topic
         * @param state Lifecycle Node's state
         * @return Success or Failure
         */
        nav2_util::CallbackReturn on_deactivate(const rclcpp_lifecycle::State & state) override;
        /**
         * @brief Resets the member variables
         * @param state Lifecycle Node's state
         * @return Success or Failure
         */
        nav2_util::CallbackReturn on_cleanup(const rclcpp_lifecycle::State & state) override;
        /**
         * @brief Called when in Shutdown state
         * @param state Lifecycle Node's state
         * @return Success or Failure
         */
        nav2_util::CallbackReturn on_shutdown(const rclcpp_lifecycle::State & state) override;

    private:
        void mapReceived(const nav_msgs::msg::OccupancyGrid::SharedPtr msg);        

        rclcpp::Subscription<nav_msgs::msg::OccupancyGrid>::ConstSharedPtr costmap_sub_;

        rclcpp_lifecycle::LifecyclePublisher<nav_msgs::msg::OccupancyGrid>::SharedPtr occupancy_grid_publisher_;
        
        // custom classes
        Costmap costmap_;

        std::vector<std::vector<geometry_msgs::msg::Point>> prohibition_polygons_;
        // map坐标系下的点集
        std::vector<PointInt> prohibited_cells_;      

    public:
        void AddVirtualWall();

        void UpdateVirtualWall(std::vector<beefast_filter_mask::ZoneData> prohibit_polys);   
    };

} // namespace beefast_filter_mask
