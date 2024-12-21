#include <memory>

#include "beefast_filter_mask/virtual_wall.hpp"
#include "rclcpp/rclcpp.hpp"

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<beefast_filter_mask::VirtualWall>();  
  rclcpp::spin(node->get_node_base_interface());
  rclcpp::shutdown();
  return 0;
}
