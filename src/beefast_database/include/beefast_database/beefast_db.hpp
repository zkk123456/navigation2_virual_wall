// Copyright (c) 2024, Ontoptech Inc.
// All rights reserved.
// Author: Rockey Shao
// Created: 2024/11/14

#ifndef BEEFAST_DATABASE__BEEFAST_DB_HPP_
#define BEEFAST_DATABASE__BEEFAST_DB_HPP_

#include <SQLiteCpp/SQLiteCpp.h>
#include <iostream>
#include <string>
#include <mutex>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <functional>
#include <nlohmann/json.hpp>
#include <rclcpp/rclcpp.hpp>
#include <memory>


namespace beefast_database {
using json = nlohmann::json;

class BeefastDB {
public:
    BeefastDB(const std::string& t_db_file);

    void addRooms(const std::vector<std::string>& t_rooms);

    void addRooms(const json& t_rooms);

    std::vector<json> readRooms(const int t_map_id);

    json getRooms(const int t_map_id);

    void updateRoomName(const int t_map_id,const int t_room_value, const std::string& t_new_name);

    void deleteRoom(const int t_map_id,const int t_room_value);

    void createRooms();


    json getVirualZones(const int t_map_id);

    void deleteVirualZone(const int t_zone_id);

    void deleteAllVirualZone(const int t_map_id);

    void addVirualZone(const json& t_rooms);


    void createMap();

    void createIndexMap();

    void createMapZones();

    void createTrigger();

    void createRoomIndexAndTrigger();

    void addMap(const std::string& name, const std::string& file_name);

    void removeMap(const int t_map_id);

    void addIndexMap(const std::string& name, const std::string& file_name, const std::string& start_point);

    int getLastMapId();

    std::vector<json> getIndexMap();

    json getAllIndexMap();

    json getIndexMap(int t_id);

    void updateIndexMap(int t_id, const std::string& name);

    void removeIndexMap(const int t_id);

private:
    std::string db_file_;
    SQLite::Database db_;
    std::mutex m_mutex; // 用于多线程保护

    // rclcpp::Logger& logger_; 
    rclcpp::Logger logger_{rclcpp::get_logger("BeefastDB")};
};

} // namespace beefast_database
#endif // BEEFAST_DATABASE__BEEFAST_DB_HPP_