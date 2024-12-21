#include "beefast_database/database_tool.hpp"

int main(int argc, char **argv)
{  
  //对数据库路径进行配置
  rclcpp::init(argc, argv);
  std::string db_path = "/opt/beefast/db/beefast.db";
  auto database = std::make_shared<beefast_database::DatabaseTool>(db_path);
  /*std::vector<beefast_database::IndexMapData> index_map_data;
  database->QueryAllIndexMap(index_map_data);
  // 打印 vector 中所有的数据
  for (const auto& data : index_map_data) {
        std::cout << "id: " << data.id << "\n"
                  << "map_id: " << data.map_id << "\n"
                  << "map_name: " << data.map_name << "\n"
                  << "origin_file_name: " << data.origin_file_name << "\n"
                  << "create_at: " << data.create_at << "\n"
                  << "update_at: " << data.update_at << "\n"
                  << "version: " << data.version << "\n"
                  << "start_point: " << data.start_point << "\n"
                  << "---------------------------------" << std::endl;
  }*/
  
  //添加地图数据
   beefast_database::MapData map_data;
   map_data.map_name = "mingliang_20241129_map11";
   map_data.origin_map_name = "mingliang_20241129_map11";
   map_data.create_at = "20241129";
   map_data.update_at = "20241129";
   map_data.version = "1.0";
   map_data.start_point = "-1.2,1.3";
   database->WriteMapData(map_data);

  // beefast_database::MapZone zone;
  // zone.map_id = 1;
  // zone.zone_type = 2;
  // zone.zone_mode = 1;
  // zone.zone_coordinates = "(-0.364 1.1)(0.615 1.17)(0.653 -0.75)(-0.26 -0.136)";
  // database->WriteZoneData(zone);

  // //删除指定虚拟墙
  // database->RemoveByZoneId(6);
  // //坐标解析
  // std::vector<std::pair<double, double>> coordinates;
  // std::string zone_coordinates = "(-0.364 1.1)(0.615 1.17)(0.653 -0.75)(-0.26 -0.136)";
  // database->GetCoordinate(zone_coordinates,coordinates);
  // for (const auto &coord : coordinates)
  // {
  //   std::cout << "Received coordinate: (" << coord.first << "," << coord.second << ")" << std::endl;
  // }  
  rclcpp::spin(database);
  rclcpp::shutdown();
  return 0; 
}