#include "virtual_wall_service/virtual_wall_server.hpp"
#include <iomanip>  
#include <sstream>


namespace virtual_wall_service
{
    VirtualWallServer::VirtualWallServer(const std::string &node_name, const std::string &ns,
             const rclcpp::NodeOptions &options) : rclcpp::Node(node_name, ns, options)
    {
        std::cout << "VirtualWallServer Constructor." << std::endl; 
        //获取导航参数
        declare_parameter("param_file", "/opt/beefast/config/keepout_params.yaml"); 
        get_parameter("param_file", param_file_);

        virtual_wall_mask_node_ = std::make_shared<beefast_filter_mask::VirtualWall>();  
        auto synchronous_client = std::make_shared<rclcpp::SyncParametersClient>(virtual_wall_mask_node_);        
        auto load_future = synchronous_client->load_parameters(param_file_);
        RCLCPP_INFO(rclcpp::get_logger("VirtualWallServer"), "virtual_wall_mask_node_ loaded the parameters successfully");
       
        costmap_filter_info_node_ = std::make_shared<nav2_map_server::CostmapFilterInfoServer>();
        synchronous_client = std::make_shared<rclcpp::SyncParametersClient>(costmap_filter_info_node_);        
        load_future = synchronous_client->load_parameters(param_file_);
        RCLCPP_INFO(rclcpp::get_logger("VirtualWallServer"), "costmap_filter_info_node_ loaded the parameters successfully");

        node_map_.push_back(virtual_wall_mask_node_);     
        node_map_.push_back(costmap_filter_info_node_);          
        rclcpp_thread_1 = std::make_unique<nav2_util::NodeThread>(virtual_wall_mask_node_);
        rclcpp_thread_2 = std::make_unique<nav2_util::NodeThread>(costmap_filter_info_node_);
        
        map_pub_ =  this->create_publisher<std_msgs::msg::String>("/generate_map", 1);
        update_zone_pub_ =  this->create_publisher<std_msgs::msg::String>("/update_zone", 1);
        shutdown_pub_ =  this->create_publisher<std_msgs::msg::String>("beefast/shutdown", 1);
        //创建数据库客户端
        database_client_ = create_client<DatabaseService>("/database/service");
        while (!database_client_->wait_for_service(std::chrono::seconds(2))) 
        {
            if (!rclcpp::ok()) 
            {
                RCLCPP_ERROR(get_logger(), "Interrupted while waiting for the service.");
            }
            RCLCPP_INFO(get_logger(), "Service not available.");
        }
        current_state_ = State::PRIMARY_STATE_UNCONFIGURED;
    }

    VirtualWallServer::~VirtualWallServer()
    {
      std::cout << "VirtualWallServer destructor." << std::endl;
    }

    bool VirtualWallServer::Startup()
    {
      RCLCPP_INFO(rclcpp::get_logger("VirtualWallServer"), "Startup...");
      bool task_staus = Configure();
      if(!task_staus)
        return false;
      task_staus = Activate();
      if(!task_staus)
        return false;
      current_state_ = State::PRIMARY_STATE_ACTIVE;
      return true;
    } 

    //配置
    bool VirtualWallServer::Configure()
    {
        RCLCPP_INFO(rclcpp::get_logger("VirtualWallServer"), "Startup...");
        for (auto& node: node_map_) 
        {
            node->configure(ret);
            uint8_t state = node->get_current_state().id();
            if (!(state == State::PRIMARY_STATE_INACTIVE))
            {
                RCLCPP_ERROR(rclcpp::get_logger("VirtualWallServer"), "Failed to change configure state for node");
                return false;
            }
        }
        return true;
    }

    bool VirtualWallServer::Activate()
    {
      RCLCPP_INFO(rclcpp::get_logger("VirtualWallServer"), "activate...");
      for (auto& node: node_map_) 
      {
        node->activate(ret);
        uint8_t state = node->get_current_state().id();
        if (!(state == State::PRIMARY_STATE_ACTIVE))
        {
          RCLCPP_ERROR(rclcpp::get_logger("VirtualWallServer"), "Failed to change activate state for node.");
          return false;
        }
      }
      current_state_ = State::PRIMARY_STATE_ACTIVE;
      return true;
    }

    /**  @brief 去激活
     * 
     */
    bool VirtualWallServer::Deactivate()
    {
      RCLCPP_INFO(rclcpp::get_logger("VirtualWallServer"), "deactivate...");
      for (auto& node: node_map_) 
      {
        node->deactivate(ret);
        uint8_t state = node->get_current_state().id();
        if (!(state == State::PRIMARY_STATE_INACTIVE))
        {
          RCLCPP_ERROR(rclcpp::get_logger("VirtualWallServer"), "Failed to change deactivate state for node.");
          return false;
        }
      }
      current_state_ = State::PRIMARY_STATE_INACTIVE;
      return true;   
    }

        /**  @brief 清理
         * 
         */
    bool VirtualWallServer::Cleanup()
    {
      RCLCPP_INFO(rclcpp::get_logger("VirtualWallServer"), "cleanup...");
      for (auto& node: node_map_) 
      {
        node->cleanup(ret);
        uint8_t state = node->get_current_state().id();
        if (!(state == State::PRIMARY_STATE_UNCONFIGURED))
        {
          RCLCPP_ERROR(rclcpp::get_logger("VirtualWallServer"), "Failed to change cleanup state for node.");
          return false;
        }
      }
      current_state_ = State::PRIMARY_STATE_UNCONFIGURED;
      return true;    
    }

        /**  @brief 关闭
         * 
         */
    bool VirtualWallServer::Shutdown()
    {
      RCLCPP_INFO(rclcpp::get_logger("VirtualWallServer"), "shutdown...");
      for (auto& node: node_map_) 
      {  
        node->shutdown(ret);
        uint8_t state = node->get_current_state().id();
        if (!(state == State::PRIMARY_STATE_FINALIZED))
        {
          RCLCPP_ERROR(rclcpp::get_logger("VirtualWallServer"), "Failed to change shutdown state for node.");
          return false;
        }
      }
      auto message = std::make_shared<std_msgs::msg::String>();
      message->data = "virtual_wall_service";
      shutdown_pub_->publish(*message);
      current_state_ = State::PRIMARY_STATE_UNCONFIGURED;
      return true;    
    }

    uint8_t VirtualWallServer::GetNodeState()
    {
      return current_state_;
    }  


/* belief: 动态刷新虚拟墙
*/
    void VirtualWallServer::RefreshVirtualWall(int map_id)
    {   
      RCLCPP_INFO(rclcpp::get_logger("VirtualWallServer"), "RefreshVirtualWall %d...", map_id);
      std::string zone_datas;
      auto request = std::make_shared<DatabaseService::Request>();
      nlohmann::json request_json;
        // 构造 JSON 数据
      request_json["op"] = "getzone";
      request_json["data"]["map_id"] = map_id;
      request->request_data = request_json.dump();
      auto result_future = database_client_->async_send_request(request);
      RCLCPP_INFO(rclcpp::get_logger("VirtualWallServer"), "request server start.");
      // Wait for the result.
      if (rclcpp::spin_until_future_complete(shared_from_this(), result_future) ==
        rclcpp::FutureReturnCode::SUCCESS)
      {
        auto result = result_future.get();
        zone_datas = result->response_data;    
        RCLCPP_INFO(rclcpp::get_logger("VirtualWallServer"), "Result: %s", zone_datas.c_str());
      } else {
        RCLCPP_ERROR(rclcpp::get_logger("rclcpp"), "Failed to call service RefreshVirtualWall");
      }   
      
      //判断虚拟墙数据非空，就加载虚拟墙到地图中
      if (zone_datas.empty()) 
        return ;
      //对map_id对应的虚拟墙数据进行解析
      nlohmann::json parsed_json = nlohmann::json::parse(zone_datas);
      std::vector<beefast_filter_mask::ZoneData> zones;
      for (const auto& json_zone : parsed_json["data"]) 
      {
        beefast_filter_mask::ZoneData zone_data;
        zone_data.map_id = std::stoi(json_zone["map_id"].get<std::string>());
        zone_data.zone_id = std::stoi(json_zone["zone_id"].get<std::string>());
        zone_data.zone_type = std::stoi(json_zone["zone_type"].get<std::string>());
        // 转换 zone_coordinates
        auto coordinates = nlohmann::json::parse(json_zone["zone_coordinates"].get<std::string>());
        for (const auto& coord : coordinates) 
        {
            zone_data.coordinates.emplace_back(beefast_filter_mask::Coordinate{coord["x"], coord["y"]});
        }
        zones.push_back(zone_data);
      }
      // 分割字符串
      RCLCPP_INFO(rclcpp::get_logger("VirtualWallServer"), "zones.size() : %d.",zones.size());
      //获取虚拟墙信息
      //提醒cartographer生成/map话题
      auto message = std::make_shared<std_msgs::msg::String>();
      message->data = "virtual_wall";
      map_pub_->publish(*message);
      //手动添加多边形禁止层
      virtual_wall_mask_node_->UpdateVirtualWall(zones);
      //发布/update_zone话题，提醒 costmap_filter_info_lifecycle
      update_zone_pub_->publish(*message);   
    }

    void VirtualWallServer::AddVirtualWall(std::string param)
    {
      RCLCPP_INFO(rclcpp::get_logger("VirtualWallServer"), "AddVirtualWall %s...", param.c_str());
      auto request = std::make_shared<DatabaseService::Request>();      
      request->request_data = param;
      auto result_future = database_client_->async_send_request(request);
      RCLCPP_INFO(rclcpp::get_logger("VirtualWallServer"), "request server start.");
      if (rclcpp::spin_until_future_complete(shared_from_this(), result_future) ==
        rclcpp::FutureReturnCode::SUCCESS)
      {
        RCLCPP_INFO(rclcpp::get_logger("VirtualWallServer"), "AddVirtualWall,add virtual wall successfully.");
      } else {
        RCLCPP_ERROR(rclcpp::get_logger("VirtualWallServer"), "AddVirtualWall,Invalid future");
      }
      // 使用 nlohmann::json 解析 JSON 字符串
      nlohmann::json parsed_json = nlohmann::json::parse(param);
      int first_map_id = 1;
      // 安全检查和提取数据
      if (parsed_json.contains("data") && parsed_json["data"].is_array() && !parsed_json["data"].empty()) {
          if (parsed_json["data"][0].contains("map_id") && parsed_json["data"][0]["map_id"].is_number()) {
              first_map_id = parsed_json["data"][0]["map_id"];
              std::cout << "First map_id: " << first_map_id << std::endl;
          } else {
              std::cerr << "Error: 'map_id' is missing or not a number in the first element of 'data'." << std::endl;
          }
      } else {
          std::cerr << "Error: 'data' is missing, not an array, or empty." << std::endl;
      }

      RefreshVirtualWall(first_map_id);
      return ;
    }

    void VirtualWallServer::DeleteVirtualWall(std::string param) 
    {
      RCLCPP_INFO(rclcpp::get_logger("VirtualWallServer"), "DeleteVirtualWall %s...", param.c_str());
      
      auto request = std::make_shared<DatabaseService::Request>();
      request->request_data = param;
      auto result_future = database_client_->async_send_request(request);
      if (rclcpp::spin_until_future_complete(shared_from_this(), result_future) ==
        rclcpp::FutureReturnCode::SUCCESS)
      {
        RCLCPP_INFO(rclcpp::get_logger("VirtualWallServer"), "DeleteVirtualWall,delete virtual wall successfully.");
      } else {
        RCLCPP_ERROR(rclcpp::get_logger("VirtualWallServer"), "DeleteVirtualWall,Invalid future");
      }
      // 使用 nlohmann::json 解析 JSON 字符串
      nlohmann::json parsed_json = nlohmann::json::parse(param);
      int first_map_id = 1;
      // 安全检查和提取数据
      if (parsed_json.contains("data")) {
          if (parsed_json["data"].contains("map_id") && parsed_json["data"]["map_id"].is_number()) 
          {
              first_map_id = parsed_json["data"]["map_id"];
              std::cout << "First map_id: " << first_map_id << std::endl;
          } else {
              std::cerr << "Error: 'map_id' is missing or not a number in the first element of 'data'." << std::endl;
          }
      } else {
          std::cerr << "Error: 'data' is missing, not an array, or empty." << std::endl;
      }
      
      RefreshVirtualWall(first_map_id);
      return ;
    }
}