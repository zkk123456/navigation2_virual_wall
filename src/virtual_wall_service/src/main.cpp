#include "virtual_wall_service/virtual_wall_server.hpp"

int main(int argc, char **argv)
{  
    rclcpp::init(argc, argv);
    // 赋值配置参数
    std::string node_name = "virtual_wall_service";
    auto application_ = std::make_shared<virtual_wall_service::VirtualWallServer>(node_name);   
    auto synchronous_client = std::make_shared<rclcpp::SyncParametersClient>(application_);        
    auto load_future = synchronous_client->load_parameters("/opt/beefast/config/beefast.yaml");
    RCLCPP_INFO(rclcpp::get_logger("virtual_wall_service"), "virtual_wall_service loaded the parameters successfully");
    application_->Startup();   
    // std::this_thread::sleep_for(std::chrono::seconds(3));
    // RCLCPP_INFO(rclcpp::get_logger("virtual_wall_service"), "reflush virtual wall");  
    // application_->RefreshVirtualWall(1);

     // JSON 数据字符串
    std::string json_str = R"({
        "func": "virtual_wall_service",
        "op": "addzone",
        "data": [
          {
            "map_id": 1,
            "type": 2,
            "count": 3,
            "coordinates": [
                {"x": 0.1, "y": 0.2},
                {"x": 1.5, "y": 0.9},
                {"x": 0.6, "y": 5}
            ]
          },
          {
            "map_id": 1,
            "type": 2,
            "count": 4,
            "coordinates": [
                {"x": 0.0, "y": 0.0},
                {"x": 1.0, "y": 0.0},
                {"x": 1.0, "y": 1.0},
                {"x": 0.0, "y": 1.0}
            ]
           }
        ]
    })";

    application_->AddVirtualWall(json_str);
    // std::this_thread::sleep_for(std::chrono::seconds(3));

/*
{
  "func": "virtual_wall_service",
  "op": "deletezone",
  "data": {
  "map_id": 1,
  "zone_id": 1
  }
}
*/
    // nlohmann::json request_json;
    //   // 构造 JSON 数据
    // request_json["op"] = "deletezones";
    // request_json["name"] = map_name;
    // request_json["file_name"] = map_name;
    // request_json["data"]["map_id"] = 1;

/*
{
  "func": "virtual_wall_service",
  "op": "deletezone",
  "data": {
  "map_id": 1,
  "zone_id": 1
  }
}
*/
      // nlohmann::json request_json;
      //   // 构造 JSON 数据
      // request_json["op"] = "deletezone";
      // request_json["data"]["map_id"] = 1;
      // request_json["data"]["zone_id"] = 15;

      // // 将 JSON 数据序列化为字符串
      // std::string request_string = request_json.dump();  // dump() 默认没有缩进
      // application_->DeleteVirtualWall(request_string);


    rclcpp::spin(application_->get_node_base_interface());
    // 关闭解析命令行标志
    rclcpp::shutdown(); 
    return 0;
}
