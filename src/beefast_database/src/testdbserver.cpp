#include "beefast_database/database_server.hpp"

int main(int argc, char **argv) {
  rclcpp::init(argc, argv);
  std::shared_ptr<rclcpp::Executor> executor 
    = std::make_shared<rclcpp::executors::SingleThreadedExecutor>();
  std::string db_path = "/opt/beefast/db/beefast.db";
  std::string node_name="beefast_db_server";
  auto database = std::make_shared<beefast_database::DatabaseServer>(executor, db_path, node_name);
  rclcpp::spin(database);
  rclcpp::shutdown();
  return 0;
}