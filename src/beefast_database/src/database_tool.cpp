#include "beefast_database/database_tool.hpp"

namespace beefast_database
{
    DatabaseTool::DatabaseTool(std::string database_file_path) : Node("database_node"),database_file_path_(database_file_path)
    {
        // 对数据库路径进行配置
        int rc = sqlite3_open(database_file_path_.c_str(), &db);
        if (rc)
        {
            RCLCPP_INFO(rclcpp::get_logger("DatabaseTool"), "%s cannot be opened.", database_file_path_.c_str());
        } else {
            RCLCPP_INFO(rclcpp::get_logger("DatabaseTool"), "open db successfully.");
        }
        database_server_ = create_service<DatabaseService>("/database_service",
            std::bind(&DatabaseTool::GetDatabaseData, this, std::placeholders::_1, std::placeholders::_2));
    }

    DatabaseTool::~DatabaseTool()
    {
        RCLCPP_INFO(rclcpp::get_logger("DatabaseTool"), "DatabaseTool destructor.");
    }

     // 自定义函数，将当前时间转换为指定格式的字符串  
    std::string DatabaseTool::GetCurrentDateTimeString() 
    {  
        // 获取当前时间点  
        auto now = std::chrono::system_clock::now();  
        // 转换为time_t，以便使用std::put_time  
        auto tt = std::chrono::system_clock::to_time_t(now);        
        // 创建本地时间的tm结构  
        std::tm* local_time = std::localtime(&tt);        
        // 创建一个输出字符串流  
        std::ostringstream oss;  
        // 设置填充和格式  
        oss << std::put_time(local_time, "%Y-%m-%d %H:%M:%S");        
        // 返回格式化的日期时间字符串  
        return oss.str();  
    }  
    

    void DatabaseTool::GetCoordinate(std::string coordinate_data,std::vector<std::pair<double, double>> &coordinates)
    {
        size_t start_pos = 0;
        while ((start_pos = coordinate_data.find("(", start_pos)) != std::string::npos)
        {
            size_t end_pos = coordinate_data.find(")", start_pos);
            if (end_pos != std::string::npos)
            {
                std::string coordinate_str = coordinate_data.substr(start_pos + 1, end_pos - start_pos - 1);
                std::istringstream iss(coordinate_str);
                double x, y;
                iss >> x >> y;
                coordinates.push_back(std::make_pair(x, y));
            }
            start_pos = end_pos;
        }
        return ;
    }

    std::unordered_map<std::string, std::string> DatabaseTool::ParseKeyword(const std::string &input)
    {
        std::unordered_map<std::string, std::string> result;
        std::istringstream iss(input);
        std::string token;
        while (std::getline(iss, token, ','))
        {
            std::istringstream tokenStream(token);
            std::string key, value;
            std::getline(tokenStream, key, '=');
            std::getline(tokenStream, value, '=');
            result[key] = value;
        }
        return result;
    }

    void DatabaseTool::GetDatabaseData(
        const std::shared_ptr<DatabaseService::Request> request,
        std::shared_ptr<DatabaseService::Response> response)
    {
        
        RCLCPP_INFO(rclcpp::get_logger("DatabaseTool"), "进入数据库工具箱 ");
        /*
        实例1：
            根据map_id查询所有的虚拟墙信息：
                uint8 command = 3
                string type = "virtual_wall_service"
                string request_data="map_id=1"
                ---
                string status = success 或 failed
                string response_data //所有虚拟墙数据，用";"分割不同记录，","分割不同字段  
        */
        if(request->command == DatabaseService::Request::QUERY && request->type == "virtual_wall_service")
        {
            std::string map_id = request->request_data;
            RCLCPP_INFO(rclcpp::get_logger("DatabaseTool"), "query virual wall,map_id : %s",map_id.c_str());
            std::vector<MapZone> zones_data;            
            QueryZoneByMapId(std::stoi(map_id),zones_data);

            RCLCPP_INFO(rclcpp::get_logger("DatabaseTool"), "query virual wall size : %d",zones_data.size());
            std::string response_data = "";
            for(int i = 0;i < zones_data.size();i++)
            {
                std::string zone_id = std::to_string(zones_data[i].zone_id);
                std::string map_id = std::to_string(zones_data[i].map_id);                
                std::string zone_mode = std::to_string(zones_data[i].zone_mode);
                std::string zone_type = std::to_string(zones_data[i].zone_type);
                std::string zone_coordinates = zones_data[i].zone_coordinates;
                std::cout << "zone_mode :" <<  zone_mode << ",zone_type : " << zone_type << ",zone_coordinates : " << zone_coordinates << std::endl;
                std::string zone_record = "";
                zone_record += map_id;
                zone_record += ",";
                zone_record += zone_id;
                zone_record += ",";
                zone_record += zone_mode;
                zone_record += ",";
                zone_record += zone_type;
                zone_record += ",";
                zone_record += zone_coordinates;
                zone_record += ";";
                response_data += zone_record ;
            } 
            response->status = "success";
            response->response_data = response_data;
        } 

        /*
        实例2：
            添加虚拟墙信息：
            uint8 command = 0
            string type = "virtual_wall_service"
            string request_data="map_id=1,zone_mode=1,zone_type=3,zone_coordinates=(1.0 0)(0 0)(0 0)(0 1.0)"
            ---
            string status = success 或 failed
            string response_data //无
        */
        if(request->command == DatabaseService::Request::WRITE && request->type == "virtual_wall_service")
        {
            std::string request_data = request->request_data;
            RCLCPP_INFO(rclcpp::get_logger("DatabaseTool"), "add virual wall,request_data : %s",request_data.c_str());

            std::unordered_map<std::string, std::string> keyValuePairs = ParseKeyword(request_data);

            //判断keyValuePairs中是否包含map_id和new_name
            if(keyValuePairs.find("map_id") == keyValuePairs.end() || keyValuePairs.find("zone_mode") == keyValuePairs.end()
                || keyValuePairs.find("zone_type") == keyValuePairs.end() || keyValuePairs.find("zone_coordinates") == keyValuePairs.end())
            {
                response->status = "failed";
                return ;
            }           

//map_id=2,zone_mode=1,zone_type=3,zone_coordinates=(2.87 2.81)(2.91 1.99)(0.949 1.97)(1.07 2.85)
            std::string map_id = keyValuePairs["map_id"];
            std::string zone_mode = keyValuePairs["zone_mode"];
            std::string zone_type = keyValuePairs["zone_type"];            

            beefast_database::MapZone zone;
            zone.map_id = std::atoi(map_id.c_str());            
            zone.zone_mode = std::atoi(zone_mode.c_str());
            zone.zone_type = std::atoi(zone_type.c_str());
            zone.zone_coordinates = keyValuePairs["zone_coordinates"];
            WriteZoneData(zone);
            response->status = "success";
        } 

        /*
        实例3：
            删除虚拟墙信息
            uint8 command = 1
            string type = "virtual_wall_service"
            string request_data="zone_id=1"
            ---
            string status = success 或 failed
            string response_data //无
        */
        if(request->command == DatabaseService::Request::DELETE && request->type == "virtual_wall_service")
        {
            std::string request_data = request->request_data;
            RCLCPP_INFO(rclcpp::get_logger("DatabaseTool"), "delete virual wall,request_data : %s",request_data.c_str());
            std::unordered_map<std::string, std::string> keyValuePairs = ParseKeyword(request_data);
            //判断keyValuePairs中是否包含map_id和new_name
            if(keyValuePairs.find("zone_id") == keyValuePairs.end())
            {
                response->status = "failed";
                return ;
            }  
            int zone_id = std::atoi(keyValuePairs["zone_id"].c_str());
            RemoveByZoneId(zone_id);
            response->status = "success";
        } 

        /*
        实例4：
            修改地图名称，
                uint8 command = 2
                string type = "map_service"
                string request_data="map_id=1,old_name=,new_name="
                ---
                string status = success 或 failed
                string response_data //无
        */
       if(request->command == DatabaseService::Request::UPDATE && request->type == "map_service")
       {
            std::string request_data = request->request_data;
            std::unordered_map<std::string, std::string> keyValuePairs = ParseKeyword(request_data);

            //判断keyValuePairs中是否包含map_id和new_name
            if(keyValuePairs.find("map_id") == keyValuePairs.end() || keyValuePairs.find("new_name") == keyValuePairs.end())
            {
                response->status = "failed";
                return ;
            } 
             // 打印解析结果
            for (const auto &pair : keyValuePairs)
            {
                std::cout << "key : " << pair.first.c_str() << ",value : " << pair.second.c_str() << std::endl;
            }
            int map_id = std::atoi(keyValuePairs["map_id"].c_str());
            std::string old_name = keyValuePairs["old_name"];
            std::string new_name = keyValuePairs["new_name"];
            UpateMapName(map_id,old_name,new_name);
            response->status = "success";
       }

       /*
       实例5：
        删除地图
            uint8 command = 1
            string type = "map_service"
            string request_data="map_id=1"
            ---
            string status = success 或 failed
            string response_data //无
       */
       if(request->command == DatabaseService::Request::DELETE && request->type == "map_service")
       {
            std::string request_data = request->request_data;
            std::unordered_map<std::string, std::string> keyValuePairs = ParseKeyword(request_data);
            //判断keyValuePairs中是否包含map_id和new_name
            if(keyValuePairs.find("map_id") == keyValuePairs.end())
            {
                response->status = "failed";
                RCLCPP_WARN(rclcpp::get_logger("DatabaseTool"), "The map_id field does not exist.");
                return ;
            } 
            int map_id = std::atoi(keyValuePairs["map_id"].c_str());
            // RemoveByMapId(map_id);
            RemoveMapDataById(map_id);
            response->status = "success";   
            response->response_data = map_name_;         
       }
        /*
        实例6：
        保存地图
            uint8 command = 0
            string type = "map_service"
            string request_data="map_name=xxx,origin_map_name=xxx"
            ---
            string status = success 或 failed
            string response_data //无    
        */
        if(request->command == DatabaseService::Request::WRITE && request->type == "map_service")
       {
            std::string request_data = request->request_data;
            std::unordered_map<std::string, std::string> keyValuePairs = ParseKeyword(request_data);
            //判断keyValuePairs中是否包含map_id和new_name
            if(keyValuePairs.find("map_name") == keyValuePairs.end() || keyValuePairs.find("origin_map_name") == keyValuePairs.end())
            {
                response->status = "failed";
                RCLCPP_WARN(rclcpp::get_logger("DatabaseTool"), "The map_name and origin_map_name field does not exist.");
                return ;
            } 
            std::string map_name = keyValuePairs["map_name"];
            std::string origin_map_name = keyValuePairs["origin_map_name"];
            std::string start_point = keyValuePairs["start_point"];
            SaveMapWriteData(map_name,origin_map_name,start_point);
            response->status = "success";      
       }
        /*
        实例7：
        根据map_id查询查询对应索引地图信息：
        uint8 command = 3
        string type = "index_map_service"
        string request_data="map_id=1"
        ---
        string status = success 或 failed
        string response_data //索引地图数据，用","分割不同字段  
        */
        if(request->command == DatabaseService::Request::QUERY && request->type == "index_map_service")
        {
            std::string map_id = request->request_data;
            RCLCPP_INFO(rclcpp::get_logger("DatabaseTool"), "query index map,map_id : %s",map_id.c_str());
            IndexMapData index_map_data;            
            QueryIndexMapByMapId(std::stoi(map_id),index_map_data);

            std::string response_data = "";

            std::string id = std::to_string(index_map_data.id);
            map_id = std::to_string(index_map_data.map_id);                
            std::string map_name = index_map_data.map_name;
            std::string origin_file_name = index_map_data.origin_file_name;
            std::string create_at = index_map_data.create_at;
            std::string update_at = index_map_data.update_at;
            std::string version = index_map_data.version;
            std::string start_point = index_map_data.start_point;
            std::cout << "map_name :" <<  map_name << ",origin_file_name : " << origin_file_name << ",create_at : " << create_at << std::endl;
            std::string index_map_record = "";
            index_map_record += id;
            index_map_record += ",";
            index_map_record += map_id;
            index_map_record += ",";
            index_map_record += map_name;
            index_map_record += ",";
            index_map_record += origin_file_name;
            index_map_record += ",";
            index_map_record += create_at;
            index_map_record += ",";
            index_map_record += update_at;
            index_map_record += ",";
            index_map_record += version;
            index_map_record += ",";
            index_map_record += start_point;
            index_map_record += ";";
            response_data += index_map_record;
            
            response->status = "success";
            response->response_data = response_data;
        } 
        
        /*
        实例8：
        查询所有的索引地图信息：
            uint8 command = 3
            string type = "all_index_map_service"
            ---
            string status = success 或 failed
            string response_data //所有索引地图信息数据，用";"分割不同记录，","分割不同字段  
        */
        if(request->command == DatabaseService::Request::QUERY && request->type == "all_index_map_service")
        {
            //std::string map_id = request->request_data;
            std::vector<IndexMapData> all_index_map_data;            
            QueryAllIndexMap(all_index_map_data);

            RCLCPP_INFO(rclcpp::get_logger("DatabaseTool"), "query virual wall size : %d",all_index_map_data.size());
            std::string response_data = "";
            for(int i = 0;i < all_index_map_data.size();i++)
            {
                std::string id = std::to_string(all_index_map_data[i].id);
                std::string map_id = std::to_string(all_index_map_data[i].map_id);                
                std::string map_name = all_index_map_data[i].map_name;
                std::string origin_file_name = all_index_map_data[i].origin_file_name;
                std::string create_at = all_index_map_data[i].create_at;
                std::string update_at = all_index_map_data[i].update_at;
                std::string version = all_index_map_data[i].version;
                std::string start_point = all_index_map_data[i].start_point;
                std::cout << "map_name :" <<  map_name << ",origin_file_name : " << origin_file_name << ",create_at : " << create_at << std::endl;
                std::string index_map_record = "";
                index_map_record += id;
                index_map_record += ",";
                index_map_record += map_id;
                index_map_record += ",";
                index_map_record += map_name;
                index_map_record += ",";
                index_map_record += origin_file_name;
                index_map_record += ",";
                index_map_record += create_at;
                index_map_record += ",";
                index_map_record += update_at;
                index_map_record += ",";
                index_map_record += version;
                index_map_record += ",";
                index_map_record += start_point;
                index_map_record += ";";
                response_data += index_map_record;
            } 
            response->status = "success";
            response->response_data = response_data;
        } 

       return ;
    }

    /**
     * @brief 读数据
     *
     * @param[in] db 数据库对象
     * @param[in] str_sql 查询语句
     * @param[out] resultdata 查询的数据集合
     * @return int  查询成功返回SQLITE_OK，否则返回 1
     */
    int DatabaseTool::SelectData(sqlite3 *db, const std::string str_sql, std::vector<std::vector<std::string>> &resultdata)
    {
        const char *sql = str_sql.data();
        char *zErrMsg = 0;
        char **dbResult;
        int nRow = 0, nColumn = 0;
        int rc = sqlite3_get_table(db, sql, &dbResult, &nRow, &nColumn, &zErrMsg);
        if (SQLITE_OK != rc)
        {
            RCLCPP_ERROR(rclcpp::get_logger("DatabaseTool"), "error: %s.", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        else
        {
            int index = nColumn; // 去掉表头
            for (int i = 0; i < nRow; i++)
            {
                std::vector<std::string> tmp;
                for (int j = 0; j < nColumn; j++)
                {
                    char *tet1 = dbResult[index++];
                    std::string subtmp = "NULL";
                    if (tet1 != 0x0)
                        subtmp = tet1;
                    tmp.push_back(subtmp);
                }
                resultdata.push_back(tmp);
            }
            sqlite3_free_table(dbResult);
            return SQLITE_OK;
        }
        return SQLITE_ERROR;
    }

    /**
     * @brief 删除表中的所有记录
     *
     * @param[in] db 数据库对象
     * @param[in] tablename 表名
     * @param[in] writedata 待删除的记录id
     * @return int  删除数据表中记录成功返回SQLITE_OK，否则返回 0
     */
    int DatabaseTool::DeleteData(sqlite3 *db, const std::string tablename, const std::vector<std::string> writedata)
    {
        std::string str_tmp = "";
        int len = writedata.size();
        for (int i = 0; i < len; i++)
        {
            str_tmp += writedata.at(i);
        }
        RCLCPP_INFO(rclcpp::get_logger("DatabaseTool"), "%s\n", str_tmp.c_str());
        const char *sql = str_tmp.data();
        char *zErrMsg = 0;
        int rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
        if (SQLITE_OK != rc)
        {
            RCLCPP_ERROR(rclcpp::get_logger("DatabaseTool"), "error: %s.", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        else
        {
            return SQLITE_OK;
        }
        return SQLITE_ERROR;
    }

    /**
     * @brief 写入数据
     *
     * @param[in] db 数据库对象
     * @param[in] tablename 表名
     * @param[in] writedata 要写入的数据集合
     * @return int  写入数据成功返回SQLITE_OK，否则返回0
     */
    int DatabaseTool::InsertData(sqlite3 *db, const std::string tablename, const std::vector<std::string> writedata)
    {
        std::string str_tmp;
        std::string str_head = "insert or replace into ";
        int len = writedata.size();
        for (int i = 0; i < len; i++)
        {
            str_tmp += str_head;
            str_tmp += tablename;
            str_tmp += writedata.at(i);
        }
        RCLCPP_INFO(rclcpp::get_logger("DatabaseTool"), "%s\n", str_tmp.c_str());
        const char *sql = str_tmp.data();
        char *zErrMsg = 0;
        int rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
        if (SQLITE_OK != rc)
        {
            RCLCPP_ERROR(rclcpp::get_logger("DatabaseTool"), "insert data failed %s.", zErrMsg);
            sqlite3_free(zErrMsg);
        }
        else
        {
            RCLCPP_INFO(rclcpp::get_logger("DatabaseTool"), "insert data successfully.");
            return SQLITE_OK;
        }
        return SQLITE_ERROR;
    }

    /**@brief 写入地图数据
     * 
     * @param[in] map_datas 地图数据
     * @return int 写入成功返回0，否则返回1
     */
    int DatabaseTool::WriteMapData(std::vector<MapData> map_datas)
    {
        int size_r = map_datas.size();
        std::vector<std::string> insert_datas;
        for (int i = 0; i < size_r; i++)
        {
            std::string str_tmp = "(map_name,origin_file_name,create_at,update_at,version) values('";
            std::string map_name = map_datas[i].map_name;
            std::string origin_map_name = map_datas[i].origin_map_name;
            std::string create_at = map_datas[i].create_at;
            std::string update_at = map_datas[i].update_at;
            std::string version = map_datas[i].version;
            str_tmp += map_name;
            str_tmp += "','";
            str_tmp += origin_map_name;
            str_tmp += "','";
            str_tmp += create_at;
            str_tmp += "','";
            str_tmp += update_at;
            str_tmp += "','";
            str_tmp += version;
            str_tmp += "');";
            insert_datas.push_back(str_tmp);
        }
        if (insert_datas.size() > 0)
        {
            int res = InsertData(db, "maps", insert_datas);
            return res;
        }
        return 1;
    }

    /**@brief 写入禁止区域数据
     * 
     * @param[in] zone_datas 禁止区域数据
     * @return int 写入成功返回0，否则返回1
     */
    int DatabaseTool::WriteZoneData( std::vector<MapZone> zone_datas)
    {
        int size_r = zone_datas.size();
        std::vector<std::string> insert_datas;
        for (int i = 0; i < size_r; i++)
        {
            std::string str_tmp = "(map_id,zone_mode,zone_type,zone_coordinates) values(";
            std::string map_id = std::to_string(zone_datas[i].map_id);
            std::string zone_mode = std::to_string(zone_datas[i].zone_mode);
            std::string zone_type = std::to_string(zone_datas[i].zone_type);
            std::string zone_coordinates = zone_datas[i].zone_coordinates;
            str_tmp += map_id;
            str_tmp += ",";
            str_tmp += zone_mode;
            str_tmp += ",";
            str_tmp += zone_type;
            str_tmp += ",'";
            str_tmp += zone_coordinates;
            str_tmp += "');";
            RCLCPP_INFO(rclcpp::get_logger("DatabaseTool"), "%s", str_tmp.c_str());
            insert_datas.push_back(str_tmp);
        } 
        if (insert_datas.size() > 0)
        {
            int res = InsertData(db, "map_zones", insert_datas);
            return res;
        }
        return 1;
    }

           /**@brief 写入地图数据
         * 
         * @param[in] map_datas 地图数据
         * @return int 写入成功返回0，否则返回1
         */
    int DatabaseTool::WriteMapData(MapData map_data)
    {
        std::vector<std::string> insert_datas;
        std::string str_tmp = "(map_name,origin_file_name,create_at,update_at,version) values('";
        std::string map_name = map_data.map_name;
        std::string origin_map_name = map_data.origin_map_name;
        std::string create_at = map_data.create_at;
        std::string update_at = map_data.update_at;
        std::string version = map_data.version;
        std::string start_point = map_data.start_point;
        std::replace(start_point.begin(), start_point.end(), '!', ',');
        
        
        str_tmp += map_name;
        str_tmp += "','";
        str_tmp += origin_map_name;
        str_tmp += "','";
        str_tmp += create_at;
        str_tmp += "','";
        str_tmp += update_at;
        str_tmp += "','";
        str_tmp += version;
        str_tmp += "');";
        insert_datas.push_back(str_tmp);
        // 向 maps 表插入数据
        int res = InsertData(db, "maps", insert_datas);
        
        if (res != SQLITE_OK) {
            return res;  // 如果 maps 插入失败，直接返回
        }
        insert_datas.clear();
        // 为 frontend_map 表构建插入语句，插入的数据与 maps 表相同
        str_tmp = "(map_id, map_name, origin_file_name, create_at, update_at, version, start_point) values((SELECT MAX(map_id) FROM maps), '";
        str_tmp += map_name;
        str_tmp += "','";
        str_tmp += origin_map_name;
        str_tmp += "','";
        str_tmp += create_at;
        str_tmp += "','";
        str_tmp += update_at;
        str_tmp += "','";
        str_tmp += version;
        str_tmp += "','";
        str_tmp += start_point;
        str_tmp += "');";
        insert_datas.push_back(str_tmp);

        // 向 frontend_map 表插入数据
        res = InsertData(db, "frontend_map", insert_datas);

        return res;
    }

        /**@brief 写入禁止区域数据
         * 
         * @param[in] zone_datas 禁止区域数据
         * @return int 写入成功返回0，否则返回1
         */
    int DatabaseTool::WriteZoneData(MapZone zone_data)
    {
        std::vector<std::string> insert_datas;
        std::string str_tmp = "(map_id,zone_mode,zone_type,zone_coordinates) values(";
        std::string map_id = std::to_string(zone_data.map_id);
        std::string zone_mode = std::to_string(zone_data.zone_mode);
        std::string zone_type = std::to_string(zone_data.zone_type);
        std::string zone_coordinates = zone_data.zone_coordinates;
        str_tmp += map_id;
        str_tmp += ",";
        str_tmp += zone_mode;
        str_tmp += ",";
        str_tmp += zone_type;
        str_tmp += ",'";
        str_tmp += zone_coordinates;
        str_tmp += "');";
        RCLCPP_INFO(rclcpp::get_logger("DatabaseTool"), "%s", str_tmp.c_str());
        insert_datas.push_back(str_tmp);
        if (insert_datas.size() > 0)
        {
            int res = InsertData(db, "map_zones", insert_datas);
            return res;
        }
        return 1;
    }

    /**
     * @brief 根据zoon_id删除表map_zones 中的记录
     *
     * @param[in] zone_id 删除记录的id
     * @return int 写入成功返回0，否则返回1
     */
    int DatabaseTool::RemoveByZoneId(int zone_id)
    {
        std::vector<std::string> delete_datas;
        std::string str_tmp = "delete from map_zones WHERE zone_id = " + std::to_string(zone_id) + ";";
        delete_datas.push_back(str_tmp);
        if (delete_datas.size() > 0)
        {
            int res = DeleteData(db, "map_zones", delete_datas);
            return res;
        }
        return 0;
    }

    /**@brief  根据zoon_id删除表map_zones 中的记录
     * 
     * @param[in] zone_ids 删除记录的id集合
     * @return int 写入成功返回0，否则返回1
     */
    int DatabaseTool::RemoveByZoneId(std::vector<int> zone_ids)
    {
        int size_r = zone_ids.size();
        std::vector<std::string> delete_datas;
        for (int i = 0; i < size_r; i++)
        {
            std::string str_tmp = "delete from map_zones WHERE zone_id = " + std::to_string(zone_ids[i]) + ";";
            delete_datas.push_back(str_tmp);
        }
        if (delete_datas.size() > 0)
        {
            int res = DeleteData(db, "map_zones", delete_datas);
            return res;
        }
        return 0;
    }

    /**@brief 根据map_id删除表maps中的记录
     * 
     * @param[in] map_id 删除记录的id
     * @return int 写入成功返回0，否则返回1
     */
    int DatabaseTool::RemoveByMapId(int map_id)
    {        
        std::vector<std::string> delete_datas;
        // 先从map_zones中删除 map_id 为 id
        std::string str_tmp = "delete from map_zones WHERE map_id = " + std::to_string(map_id) + ";";
        delete_datas.push_back(str_tmp);
        // 在从maps中删除map_id 为id的
        str_tmp = "delete from maps WHERE map_id = " + std::to_string(map_id) + ";";
        delete_datas.push_back(str_tmp);
        if (delete_datas.size() > 0)
        {
            int res = DeleteData(db, "maps", delete_datas);
            return res;
        }
        return 0;
    }

    /**@brief 根据map_id删除表maps中的记录
     * 
     * @param[in] map_id 删除记录的id
     * @return int 写入成功返回0，否则返回1
     */
    int DatabaseTool::RemoveByMapId(std::vector<int> map_ids)
    {
        int size_r = map_ids.size();
        std::vector<std::string> delete_datas;
        for (int i = 0; i < size_r; i++)
        {
            // 先从map_zones中删除 map_id 为 id
            std::string str_tmp = "delete from map_zones WHERE map_id = " + std::to_string(map_ids[i]) + ";";
            delete_datas.push_back(str_tmp);
            // 在从maps中删除map_id 为id的
            str_tmp = "delete from maps WHERE map_id = " + std::to_string(map_ids[i]) + ";";
            delete_datas.push_back(str_tmp);
        }
        if (delete_datas.size() > 0)
        {
            int res = DeleteData(db, "maps", delete_datas);
            return res;
        }
        return 0;
    }


    /**@brief 查所有的地图
     * 
     * @param[out] maps_data maps表中所有的记录
     * @return int 查询成功返回0，否则返回1
     */
    int DatabaseTool::QueryMapsData(std::vector<MapData> &maps_data)
    {
        std::string str_sql = "select * from  maps";
        std::vector<std::vector<std::string>> query_data;
        int ret = SelectData(db, str_sql, query_data);
        //将其封装成maps_data
        for (int i = 0; i < query_data.size(); i++)
        {
            std::string map_id = query_data[i][0];
            std::string map_name = query_data[i][1];
            std::string origin_map_name = query_data[i][2];
            std::string create_at = query_data[i][3];
            std::string update_at = query_data[i][4];
            std::string version = query_data[i][5];
            MapData my_data;
            my_data.map_id = std::stoi(map_id);
            my_data.map_name = map_name;
            my_data.origin_map_name = origin_map_name;
            my_data.create_at = create_at;
            my_data.update_at = update_at;
            my_data.version = version;
            maps_data.push_back(my_data);
        }
        return ret;
    }

    int DatabaseTool::QueryMapById(int map_id,std::vector<MapData> &maps_data)
    {
        std::string str_sql = "select * from  maps WHERE map_id = " + std::to_string(map_id) + ";";
        std::vector<std::vector<std::string>> query_data;
        int ret = SelectData(db, str_sql, query_data);
        //将其封装成maps_data
        for (int i = 0; i < query_data.size(); i++)
        {
            std::string map_id = query_data[i][0];
            std::string map_name = query_data[i][1];
            std::string origin_file_name = query_data[i][2];
            std::string create_at = query_data[i][3];
            std::string update_at = query_data[i][4];
            std::string version = query_data[i][5];
            
            MapData my_data;
            my_data.map_id = std::stoi(map_id);
            my_data.map_name = map_name;
            my_data.origin_map_name = origin_file_name;
            my_data.create_at = create_at;
            my_data.update_at = update_at;
            my_data.version = version;
            maps_data.push_back(my_data);
        }
        return ret;
    }

        /**@brief 根据map_id,删除数据库中地图信息
     * 
     * @param[in] map_name 地图名
     * @return int 写入成功返回0，否则返回1
     */
    int DatabaseTool::RemoveMapDataById(int map_id)
    {
        //查找map_id 对应的map_name
        std::vector<MapData> maps_data;        
        QueryMapById(map_id,maps_data);
        if(maps_data.size() > 0) 
        {
            //1）删除地图名文件（文件操作）；
            RCLCPP_INFO(rclcpp::get_logger("MapManageApplication"), "Delete the map name is '%s'",maps_data[0].map_name.c_str());
            map_name_ = maps_data[0].map_name;
        }    
        std::vector<std::string> delete_datas;
        // 先从map_zones中删除 map_id 为 id
        std::string str_tmp = "delete from map_zones WHERE map_id = " + std::to_string(map_id) + ";";
        delete_datas.push_back(str_tmp);
        // 从 frontend_map 中删除 map_id 为 id 的记录
        str_tmp = "delete from frontend_map WHERE map_id = " + std::to_string(map_id) + ";";
        delete_datas.push_back(str_tmp);
        // 在从maps中删除map_id 为id的
        str_tmp = "delete from maps WHERE map_id = " + std::to_string(map_id) + ";";
        delete_datas.push_back(str_tmp);
        if (delete_datas.size() > 0)
        {
            int res = DeleteData(db, "maps", delete_datas);
            return res;
        }
        return 0;
    }
    
    /**@brief 查指定map_id的禁行区域
     * 
     * @param[in] map_id 地图id
     * @param[out] zones_data map_id中所有的禁止区域
     * @return int 查询成功返回0，否则返回1
     */
    int DatabaseTool::QueryZoneByMapId(int map_id,std::vector<MapZone> &zones_data)
    {
        std::string str_sql = "select * from  map_zones WHERE map_id = " + std::to_string(map_id) + ";";
        std::vector<std::vector<std::string>> query_data;
        int ret = SelectData(db, str_sql, query_data);
        for (int i = 0; i < query_data.size(); i++)
        {
            std::string zone_id = query_data[i][0];
            std::string map_id = query_data[i][1];
            std::string zone_mode = query_data[i][2];
            std::string zone_type = query_data[i][3];
            std::string zone_coordinates = query_data[i][4];
            MapZone my_data;   
            my_data.zone_id = std::stoi(zone_id);
            my_data.map_id = std::stoi(map_id);
            my_data.zone_mode = std::stoi(zone_mode);
            my_data.zone_type = std::stoi(zone_type);
            my_data.zone_coordinates = zone_coordinates;
            zones_data.push_back(my_data);
        }
        return ret;
    }

    /**@brief 查指定map_id的索引地图信息
     * 
     * @param[in] map_id 地图id
     * @param[out] index_map_data map_id的索引地图信息
     * @return int 查询成功返回0，否则返回1
     */
    int DatabaseTool::QueryIndexMapByMapId(int map_id,IndexMapData &index_map_data)
    {
        std::string str_sql = "select * from  frontend_map WHERE map_id = " + std::to_string(map_id) + ";";
        std::vector<std::vector<std::string>> query_data;
        int ret = SelectData(db, str_sql, query_data);
        
        std::string id = query_data[0][0];
        std::string query_map_id = query_data[0][1];
        std::string map_name = query_data[0][2];
        std::string origin_file_name = query_data[0][3];
        std::string create_at = query_data[0][4];
        std::string update_at = query_data[0][5];
        std::string version = query_data[0][6];
        std::string start_point = query_data[0][7];
        IndexMapData my_data;   
        my_data.id = std::stoi(id);
        my_data.map_id = std::stoi(query_map_id);
        my_data.map_name = map_name;
        my_data.origin_file_name = origin_file_name;
        my_data.create_at = create_at;
        my_data.update_at = update_at;
        my_data.version = version;
        my_data.start_point = start_point;
        index_map_data=  my_data;
        
        return ret;
    }
    
    /**@brief 查所有索引地图信息
     *
     * @param[out] index_map_data 数据库中所有索引地图
     * @return int 查询成功返回0，否则返回1
     */
    int DatabaseTool::QueryAllIndexMap(std::vector<IndexMapData> &index_map_data)
    {
        std::string str_sql = "SELECT * FROM frontend_map;";
        std::vector<std::vector<std::string>> query_data;
        int ret = SelectData(db, str_sql, query_data);
        for (int i = 0; i < query_data.size(); i++)
        {
            std::string id = query_data[i][0];
            std::string map_id = query_data[i][1];
            std::string map_name = query_data[i][2];
            std::string origin_file_name = query_data[i][3];
            std::string create_at = query_data[i][4];
            std::string update_at = query_data[i][5];
            std::string version = query_data[i][6];
            std::string start_point = query_data[i][7];
            IndexMapData my_data;   
            my_data.id = std::stoi(id);
            my_data.map_id = std::stoi(map_id);
            my_data.map_name = map_name;
            my_data.origin_file_name = origin_file_name;
            my_data.create_at = create_at;
            my_data.update_at = update_at;
            my_data.version = version;
            my_data.start_point = start_point;
            index_map_data.push_back(my_data);
        }
        return ret;
    }    
    

     /**@brief 根据map_id更新 origin_map_name,同时更新update_at
         *        map_service
         * @param[in] map_id 删除记录的id
         * @param[in] origin_map_name 存储文件名称
         * @return int 写入成功返回0，否则返回1
         */
    int DatabaseTool::UpateMapName(int map_id,std::string old_name,std::string new_name)
    {
        std::string update_at = GetCurrentDateTimeString();
        
        // 1. 更新 maps 表
        std::string str_tmp = "UPDATE maps SET origin_file_name ='";
        str_tmp += new_name;
        str_tmp += "', update_at = '";
        str_tmp += update_at;
        str_tmp += "' WHERE map_id = ";
        str_tmp += std::to_string(map_id);
        str_tmp += ";";
        RCLCPP_INFO(rclcpp::get_logger("DatabaseTool"), "%s", str_tmp.c_str());
        char* errMsg = nullptr;
        char *zErrMsg = 0;
        const char *sql = str_tmp.data();
        int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
        if (rc != SQLITE_OK) {
            RCLCPP_ERROR(rclcpp::get_logger("DatabaseTool"), "error: %s.", zErrMsg);
            sqlite3_free(errMsg);
            return SQLITE_ERROR;
        }
        
        // 2. 更新 frontend_map 表
        std::string str_tmp2 = "UPDATE frontend_map SET map_name = '";
        str_tmp2 += new_name;  // 更新 map_name
        str_tmp2 += "', update_at = '";
        str_tmp2 += update_at;  // 更新时间
        str_tmp2 += "' WHERE map_id = ";
        str_tmp2 += std::to_string(map_id);
        str_tmp2 += ";";
        RCLCPP_INFO(rclcpp::get_logger("DatabaseTool"), "%s", str_tmp2.c_str());
        
        rc = sqlite3_exec(db, str_tmp2.c_str(), nullptr, nullptr, &errMsg);
        if (rc != SQLITE_OK) {
            RCLCPP_ERROR(rclcpp::get_logger("DatabaseTool"), "error: %s.", zErrMsg);
            sqlite3_free(errMsg);
            return SQLITE_ERROR;
        }
        
        RCLCPP_INFO(rclcpp::get_logger("DatabaseTool"), "Update successfully");
        return SQLITE_OK;
    }

        /**@brief 写地图记录到数据库
         *     map_service
         * @param[in] map_name 地图名
         * @return int 写入成功返回0，否则返回1
         */
    int DatabaseTool::SaveMapWriteData(std::string map_name,std::string origin_map_name,std::string start_point)
    {
        std::string update_at = GetCurrentDateTimeString();
        MapData my_data;
        my_data.map_name = map_name;
        my_data.origin_map_name = origin_map_name;
        my_data.create_at = update_at;
        my_data.update_at = update_at;
        my_data.version = "1.0";
        my_data.start_point = start_point;
        WriteMapData(my_data);
    }

        /**@brief 根据map_id,删除数据库中的虚拟墙信息
         *
         * @param[in] map_id 要删除虚拟墙的地图id
         * @return int 写入成功返回0，否则返回1
         */
    int DatabaseTool::RemoveAllZoneByMapId(int map_id)
    {
        std::vector<std::string> delete_datas;
        std::string str_tmp = "delete from map_zones WHERE map_id = " + std::to_string(map_id) + ";";
        delete_datas.push_back(str_tmp);
        if (delete_datas.size() > 0)
        {
            int res = DeleteData(db, "map_zones", delete_datas);
            return res;
        }
        return 0;
    }
}
