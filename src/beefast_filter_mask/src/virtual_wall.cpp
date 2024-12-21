#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <random>
#include "beefast_filter_mask/virtual_wall.hpp"

using std::placeholders::_1;

namespace beefast_filter_mask
{

  VirtualWall::VirtualWall() : LifecycleNode("virtual_wall", "", rclcpp::NodeOptions().allow_undeclared_parameters(true)
                        .automatically_declare_parameters_from_overrides(true))
  {
    RCLCPP_INFO(rclcpp::get_logger("VirtualWall"), "Creating virtual_wall");
    declare_parameter("topic_name", "/map1");
    declare_parameter("mask_topic", "/keepout_filter_mask");    
  }  

  nav2_util::CallbackReturn VirtualWall::on_configure(const rclcpp_lifecycle::State & /*state*/)
  {
    RCLCPP_INFO(rclcpp::get_logger("VirtualWall"), "VirtualWall Configuring...");  

    std::string topic_name = get_parameter("topic_name").as_string();
    std::string mask_topic = get_parameter("mask_topic").as_string();
    RCLCPP_INFO(rclcpp::get_logger("VirtualWall"), "topic_name : %s.",topic_name.c_str());
    costmap_sub_ = create_subscription<nav_msgs::msg::OccupancyGrid>(
        "map", 10, std::bind(&VirtualWall::mapReceived, this, _1));
    occupancy_grid_publisher_  = create_publisher<nav_msgs::msg::OccupancyGrid>(
      mask_topic,rclcpp::QoS(rclcpp::KeepLast(1)).transient_local().reliable());

    return nav2_util::CallbackReturn::SUCCESS;
  }

  nav2_util::CallbackReturn VirtualWall::on_activate(const rclcpp_lifecycle::State & /*state*/)
  {
      RCLCPP_INFO(rclcpp::get_logger("VirtualWall"), "VirtualWall Activating...");
      occupancy_grid_publisher_->on_activate();
      return nav2_util::CallbackReturn::SUCCESS;
  }

  nav2_util::CallbackReturn VirtualWall::on_deactivate(const rclcpp_lifecycle::State & /*state*/)
  {
      RCLCPP_INFO(rclcpp::get_logger("VirtualWall"), "VirtualWall Deactivating...");
      occupancy_grid_publisher_->on_deactivate();
      return nav2_util::CallbackReturn::SUCCESS;
  }

  nav2_util::CallbackReturn VirtualWall::on_cleanup(const rclcpp_lifecycle::State & /*state*/)
  {
        RCLCPP_INFO(rclcpp::get_logger("VirtualWall"), "VirtualWall Cleaning up...");
        occupancy_grid_publisher_.reset();
        return nav2_util::CallbackReturn::SUCCESS;
  }

  nav2_util::CallbackReturn VirtualWall::on_shutdown(const rclcpp_lifecycle::State & /*state*/)
  {
        RCLCPP_INFO(rclcpp::get_logger("VirtualWall"), "VirtualWall shutting...");
        return nav2_util::CallbackReturn::SUCCESS;
  } 

  void VirtualWall::UpdateVirtualWall(std::vector<beefast_filter_mask::ZoneData> prohibit_polys)
  {
    prohibition_polygons_.clear();

    //解析多边形 : 接收到的禁行区域信息,并遍历prohibit_polys
    for(const auto& poly : prohibit_polys) 
    {
      std::vector<geometry_msgs::msg::Point> points;
      for(const auto& xy : poly.coordinates) 
      {
        geometry_msgs::msg::Point point;
        point.x = xy.x;
        point.y = xy.y;
        points.push_back(point);
      }
      prohibition_polygons_.push_back(points);
    }    
  }
  
  void VirtualWall::AddVirtualWall()
  {
    RCLCPP_INFO(this->get_logger(), "generate /keepout_filter_mask");
    //将多边形加载到成本地图中
    RCLCPP_INFO(rclcpp::get_logger("VirtualWall"), "prohibition_polygons_.size() : %d.",prohibition_polygons_.size());
    costmap_.updateProhibitionCells(prohibition_polygons_,false);

    costmap_.prepareGrid(); 

    occupancy_grid_publisher_->publish(std::move(costmap_.grid_));
    return ;
  }

  void VirtualWall::mapReceived(const nav_msgs::msg::OccupancyGrid::SharedPtr msg)
  {
    RCLCPP_INFO(rclcpp::get_logger("VirtualWall"), "Got new map");
    costmap_.updateFullMap(msg);

    AddVirtualWall();
  }

} // namespace beefast_filter_mask