// Copyright (c) 2024, Ontoptech Inc.
// All rights reserved.
// Author: Rockey Shao
// Created: 2024/11/14

#include "beefast_database/beefast_db.hpp"

namespace beefast_database {
    BeefastDB::BeefastDB(const std::string& t_db_file) 
        : db_file_(t_db_file), 
          db_(t_db_file, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE){}

    // 数据库操作函数
    void BeefastDB::createMap() {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::ostringstream sql;
        sql << "CREATE TABLE IF NOT EXISTS maps (map_id INTEGER PRIMARY KEY, "
            << "map_name TEXT NOT NULL, origin_file_name TEXT NOT NULL, create_at TEXT,update_at TEXT,version TEXT);";
        std::cout << "Creating Map table..." << sql.str() << std::endl;
        db_.exec(sql.str());
    }

    void BeefastDB::createIndexMap() {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::ostringstream sql;
        sql << "CREATE TABLE IF NOT EXISTS frontend_map (id INTEGER PRIMARY KEY, map_id INTEGER, "
            << "map_name TEXT, origin_file_name TEXT,create_at TEXT,update_at TEXT,version TEXT, start_point TEXT, "
            << "FOREIGN KEY(map_id) REFERENCES maps(map_id));";
        std::cout << "Creating IndexMap table..." << sql.str() << std::endl;
        db_.exec(sql.str());
    }


    void BeefastDB::createMapZones() {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::ostringstream sql;
        sql << "CREATE TABLE IF NOT EXISTS map_zones (zone_id INTEGER, map_id INTEGER, "
            << "zone_type INTEGER,zone_coordinates TEXT, "
            << "PRIMARY KEY(zone_id AUTOINCREMENT));";
        std::cout << "Creating MapZones table..." << sql.str() << std::endl;
        db_.exec(sql.str());
    }

    void BeefastDB::createRooms() {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::ostringstream sql;
        // -- is_used设置默认值为 1,表示 true
        sql << "CREATE TABLE IF NOT EXISTS rooms"
            << "(map_id INTEGER, pix_value INTEGER, name TEXT, "
            << "is_used INTEGER DEFAULT 1 , "
            << "create_at TEXT,update_at TEXT, "
            << "FOREIGN KEY(map_id) REFERENCES maps(map_id));";
        std::cout << "Creating User table..." << sql.str() << std::endl;
        db_.exec(sql.str());
    }

    void BeefastDB::createRoomIndexAndTrigger() {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::ostringstream sql;
        sql << "CREATE INDEX idx_rooms_map_id_pix_value ON rooms (map_id, pix_value);";
        std::cout << "Creating User Index..." << sql.str() << std::endl;
        db_.exec(sql.str());  
        std::ostringstream sql_some_a;
        // 触发器：在插入时设置 create_at
        sql_some_a << "CREATE TRIGGER IF NOT EXISTS trg_after_insert_rooms "
            << "AFTER INSERT ON rooms "
            << "FOR EACH ROW "
            << "BEGIN "
            << "UPDATE rooms SET create_at = datetime('now', 'localtime'), update_at = datetime('now', 'localtime') "
            << "WHERE map_id = NEW.map_id and pix_value = NEW.pix_value; "
            << "END;";
        db_.exec(sql_some_a.str());

        std::ostringstream update_sql;
        // -- 触发器：在更新时设置 update_at
        update_sql << "CREATE TRIGGER IF NOT EXISTS trg_after_update_rooms "
            << "AFTER UPDATE ON rooms "
            << "FOR EACH ROW "
            << "BEGIN "
            << "UPDATE rooms SET update_at = datetime('now', 'localtime') "
            << "WHERE map_id = NEW.map_id and pix_value = NEW.pix_value; "
            << "END;";
        db_.exec(update_sql.str());      
    }

    void BeefastDB::createTrigger() {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cout << "Creating map trigger..." << std::endl;
        std::ostringstream sql;
        // 触发器：在插入时设置 create_at
        sql << "CREATE TRIGGER IF NOT EXISTS trg_after_insert_maps "
            << "AFTER INSERT ON maps "
            << "FOR EACH ROW "
            << "BEGIN "
            << "UPDATE maps SET create_at = datetime('now', 'localtime'), update_at = datetime('now', 'localtime') "
            << "WHERE map_id = NEW.map_id; "
            << "END;";
        db_.exec(sql.str());

        std::ostringstream update_sql;
        // -- 触发器：在更新时设置 update_at
        update_sql << "CREATE TRIGGER IF NOT EXISTS trg_after_update_maps "
            << "AFTER UPDATE ON maps "
            << "FOR EACH ROW "
            << "BEGIN "
            << "UPDATE maps SET update_at = datetime('now', 'localtime') "
            << "WHERE map_id = NEW.map_id; "
            << "END;";
        db_.exec(update_sql.str());

        std::cout << "Creating indexmap trigger..." << std::endl;
        std::ostringstream trigger_sql_some;
        // 触发器：在插入时设置 create_at
        trigger_sql_some << "CREATE TRIGGER IF NOT EXISTS trg_after_insert_indexmaps "
            << "AFTER INSERT ON frontend_map "
            << "FOR EACH ROW "
            << "BEGIN "
            << "UPDATE frontend_map SET create_at = datetime('now', 'localtime'), update_at = datetime('now', 'localtime') "
            << "WHERE map_id = NEW.map_id; "
            << "END;";
        std::cout << "Creating map trigger..." << trigger_sql_some.str() << std::endl;
        db_.exec(trigger_sql_some.str());

        std::ostringstream trigger_sql_another;
        // -- 触发器：在更新时设置 update_at
        trigger_sql_another << "CREATE TRIGGER IF NOT EXISTS trg_after_update_indexmaps "
            << "AFTER UPDATE ON frontend_map "
            << "FOR EACH ROW "
            << "BEGIN "
            << "UPDATE frontend_map SET update_at = datetime('now', 'localtime') "
            << "WHERE map_id = NEW.map_id; "
            << "END;";
        db_.exec(trigger_sql_another.str());
    }

    void BeefastDB::addMap(const std::string& name, const std::string& file_name) {
        std::lock_guard<std::mutex> lock(m_mutex);
        SQLite::Statement query(db_, "INSERT INTO maps (map_name, origin_file_name) VALUES (?, ?);");
        query.bind(1, name);
        query.bind(2, file_name);
        query.exec();
    }

    void BeefastDB::addIndexMap(const std::string& name, const std::string& file_name, const std::string& start_point) {
        int map_id = getLastMapId();
        if (map_id > -1) {
            std::lock_guard<std::mutex> lock(m_mutex);
            SQLite::Statement query(db_, "INSERT INTO frontend_map (map_id, map_name, origin_file_name, start_point) VALUES (?, ?, ?, ?);");
            query.bind(1, map_id);           // 绑定 map_id
            query.bind(2, name);             // 绑定 map_name
            query.bind(3, file_name);        // 绑定 origin_file_name
            query.bind(4, start_point);      // 绑定 start_point
            query.exec();                    // 执行查询
        } else {
            std::cout << "cannot find valid map" << std::endl;
        }
    }

    std::vector<json> BeefastDB::getIndexMap() {
        std::vector<json> some_maps;
        std::lock_guard<std::mutex> lock(m_mutex);
        SQLite::Statement query(db_, "SELECT * FROM frontend_map;");
        while (query.executeStep())
        {
            json row;
            row["id"] = query.getColumn(0);
            row["map_id"] = query.getColumn(1);
            row["name"] = query.getColumn(2);
            row["create_at"] = query.getColumn(3);
            row["update_at"] = query.getColumn(4);
            some_maps.emplace_back(row);
        }
        return some_maps;
    }

    json BeefastDB::getAllIndexMap() {
        json some_maps;
        std::lock_guard<std::mutex> lock(m_mutex);
        SQLite::Statement query(db_, "SELECT id, map_id, map_name, origin_file_name, create_at, update_at, start_point FROM frontend_map ORDER BY create_at DESC LIMIT 5;");
        // 在 JSON 对象中添加一个名为 "addresses" 的 JSON 数组
        some_maps["frontend_map"] = nlohmann::json::array();
        RCLCPP_INFO(logger_, "get all index map");
        while (query.executeStep())
        {
            json row;
            row["id"] = query.getColumn(0);
            row["map_id"] = query.getColumn(1);
            row["name"] = query.getColumn(2);
            row["origin_file_name"] = query.getColumn(3);
            row["create_at"] = query.getColumn(4);
            row["update_at"] = query.getColumn(5);
            row["start_point"] = query.getColumn(6);
            // some_maps.emplace_back(row);
            some_maps["frontend_map"].push_back(row);
        }
        return some_maps;
    }

    json BeefastDB::getIndexMap(int t_id) {
    int some_map_id = -1;
    json row;
    json frontend_map;
    std::cout << "地图id: " << t_id << std::endl;
    // 只在查询数据库时加锁
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        SQLite::Statement query(db_, "SELECT id, map_id, map_name, origin_file_name, create_at, update_at, start_point from frontend_map where map_id = ?;");
        query.bind(1, t_id);

        if (query.executeStep()) {
            frontend_map["id"] = query.getColumn(0);
            frontend_map["map_id"] = query.getColumn(1);
            frontend_map["name"] = query.getColumn(2);
            frontend_map["origin_file_name"] = query.getColumn(3);
            frontend_map["create_at"] = query.getColumn(4);
            frontend_map["update_at"] = query.getColumn(5);
            frontend_map["start_point"] = query.getColumn(6);
        }
        
    }

    // 获取 map_rooms，在这里我们不加锁
    json map_rooms = getRooms(t_id); // 调用 getRooms

    if (map_rooms.contains("rooms")) {
        frontend_map["rooms"] = map_rooms["rooms"]; // 将 rooms 添加到 row
    }

    // 将所有内容作为 "frontend_map" 的值
    row["frontend_map"] = frontend_map;
    // row["frontend_map"] = nlohmann::json::array();
    // row["frontend_map"].push_back(frontend_map);
    // // 判断 row["frontend_map"] 是否为空，并设置 flag
    // if (row["frontend_map"].empty()) {
    //     row["flag"] = "false";  // 如果为空，设置 flag 为 "false"
    // } else {
    //     row["flag"] = "true";  // 如果不为空，设置 flag 为 "true"
    // }
        
    
    // std::cout << "Row content: " << row.dump(4) << std::endl;
    return row;
	}


    void BeefastDB::updateIndexMap(int t_id, const std::string& name){
        // std::lock_guard<std::mutex> lock(m_mutex);
        // SQLite::Statement query(db_, "UPDATE frontend_map SET map_name = ?,origin_file_name=? WHERE id = ?;");
        // query.bind(1, name);
        // query.bind(2, name);
        // query.bind(3, t_id);
        // query.exec();
        std::lock_guard<std::mutex> lock(m_mutex);
        SQLite::Statement query(db_, "UPDATE frontend_map SET map_name = ? WHERE id = ?;");
        query.bind(1, name);
        query.bind(2, t_id);
        query.exec();
    }

    int BeefastDB::getLastMapId() {
        int some_map_id = -1;
        std::lock_guard<std::mutex> lock(m_mutex);
        SQLite::Statement query(db_, "SELECT MAX(map_id) from maps;");
        if (query.executeStep())
        {
            some_map_id = query.getColumn(0);
        }
        return some_map_id;
    }

    void BeefastDB::removeMap(const int t_map_id) {
        std::lock_guard<std::mutex> lock(m_mutex);
        SQLite::Statement query(db_, "DELETE FROM maps WHERE map_id = ?;");
        query.bind(1, t_map_id);
        query.exec();
    }

    void BeefastDB::removeIndexMap(const int t_id) {
        std::lock_guard<std::mutex> lock(m_mutex);
        SQLite::Statement query(db_, "DELETE FROM frontend_map WHERE map_id = ?;");
        query.bind(1, t_id);
        query.exec();
    }

    void BeefastDB::addRooms(const std::vector<std::string>& t_rooms) {
        std::lock_guard<std::mutex> lock(m_mutex);
        // 构建 INSERT 语句
        std::ostringstream oss;
        oss << "INSERT INTO rooms (map_id, pix_value, name) VALUES ";
        bool first = true;
        for (const std::string room:t_rooms) {
            if (!first) {
                oss << ",";
            }
            first = false;
            json some_room_a  = json::parse(room);
            oss << "(" << some_room_a["map_id"] << "," << some_room_a["pix_value"] << ",'" << some_room_a["room_name"] << "')";
        }
        db_.exec(oss.str());
    }

    void BeefastDB::addRooms(const json& t_rooms) {
        std::lock_guard<std::mutex> lock(m_mutex);
        // 构建INSERT语句,房间值是像素值
        std::ostringstream oss;
        oss << "INSERT INTO rooms (map_id, pix_value, name) VALUES ";
        bool first = true;
        for (const auto& some_room_a:t_rooms["data"]) {
            if (!first) 
            {
                oss << ",";
            }
            first = false;
            oss << "(" << some_room_a["map_id"] << "," << some_room_a["room_value"] << ",'" << some_room_a["room_name"] << "')";
        }
        db_.exec(oss.str());
    }

    std::vector<json> BeefastDB::readRooms(const int t_map_id) {
        std::lock_guard<std::mutex> lock(m_mutex);
        SQLite::Statement query(db_, "SELECT map_id, pix_value, name FROM rooms where map_id = ?;");
        query.bind(1, t_map_id);
        std::vector<json> map_rooms;
        
        while (query.executeStep()) {
            json row;
            row["map_id"] = query.getColumn(0);
            row["pix_value"] = query.getColumn(1);
            row["name"] = query.getColumn(2);
            map_rooms.emplace_back(row);
        }
        return map_rooms;
    }

    json BeefastDB::getRooms(const int t_map_id) {
        std::lock_guard<std::mutex> lock(m_mutex);
        SQLite::Statement query(db_, "SELECT  pix_value, name FROM rooms where map_id = ?;");
        query.bind(1, t_map_id);
        json map_rooms;
        map_rooms["rooms"] = nlohmann::json::array();

        while (query.executeStep()) {
            json row;
            row["pix_value"] = query.getColumn(0);
            row["name"] = query.getColumn(1);
            map_rooms["rooms"].push_back(row);
        }
        return map_rooms;
    }

    void BeefastDB::updateRoomName(const int t_map_id,const int t_room_value, const std::string& t_new_name) {
        std::lock_guard<std::mutex> lock(m_mutex);
        SQLite::Statement query(db_, "UPDATE rooms SET name = ? WHERE map_id = ? and pix_value = ?;");
        query.bind(1, t_new_name);
        query.bind(2, t_map_id);
        query.bind(3, t_room_value);
        query.exec();
    }

    void BeefastDB::deleteRoom(const int t_map_id,const int t_room_value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        SQLite::Statement query(db_, "DELETE FROM rooms WHERE map_id = ? and pix_value = ?;");
        query.bind(1, t_map_id);
        query.bind(2, t_room_value);
        query.exec();
    }

    json BeefastDB::getVirualZones(const int t_map_id)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        SQLite::Statement query(db_, "SELECT map_id, zone_id,zone_type,zone_coordinates FROM map_zones where map_id = ?;");
        query.bind(1, t_map_id);
        json virual_zones;
        virual_zones["data"] = nlohmann::json::array();
        while (query.executeStep()) 
        {
            json row;
            row["map_id"] = query.getColumn(0);
            row["zone_id"] = query.getColumn(1);
            row["zone_type"] = query.getColumn(2);
            row["zone_coordinates"] = query.getColumn(3);
            virual_zones["data"].push_back(row);
        } 
        return virual_zones;
    }

    void BeefastDB::deleteVirualZone(const int t_zone_id)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        SQLite::Statement query(db_, "DELETE FROM map_zones WHERE zone_id = ?;");
        query.bind(1, t_zone_id);
        query.exec();
    }

     void BeefastDB::deleteAllVirualZone(const int t_map_id)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::cout << "t_map_id : " << t_map_id << std::endl;
        SQLite::Statement query(db_, "DELETE FROM map_zones WHERE map_id = ?;");
        query.bind(1, t_map_id);
        query.exec();
    }

    void BeefastDB::addVirualZone(const json& t_rooms)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        // 构建INSERT语句,房间值是像素值
        std::ostringstream oss;
        oss << "INSERT INTO map_zones (map_id,zone_type, zone_coordinates) VALUES ";
        bool first = true;
        for (const auto& some_room_a:t_rooms["data"]) 
        {
            if (!first)
            {
                oss << ",";
            }
            first = false;
            // oss << "("<< some_room_a["map_id"] << ","
            //      << some_room_a["zone_type"] << ",'" << some_room_a["zone_coordinates"] << "')";
            oss << "(" << some_room_a["map_id"] << "," << some_room_a["type"] << ",'" << some_room_a["coordinates"].dump()  << "')";

        }
        std::cout << "oss.str() : " << oss.str() << std::endl;
        db_.exec(oss.str());
    } 

} // namespace beefast_database