// Copyright (c) 2024, Ontoptech Inc.
// All rights reserved.
// Author: Rockey Shao
// Created: 2024/11/14

#ifndef BEEFAST_DATABASE__DATABASE_SERVER_HPP_
#define BEEFAST_DATABASE__DATABASE_SERVER_HPP_
#include <string>
#include "rclcpp/rclcpp.hpp"
#include "beefast_database/beefast_db.hpp"
#include "beefast_interfaces/srv/manage_db.hpp"

namespace beefast_database {
using json = nlohmann::json;
using ManageDbService = beefast_interfaces::srv::ManageDb;
using CallbackType = std::function<json(json)>;



class DatabaseServer : public rclcpp::Node
{
public:
    DatabaseServer(std::shared_ptr<rclcpp::Executor> executor,
        const std::string & t_db_path="/opt/beefast/db/beefast.db",
        const std::string & node_name = "database_sever",
        const std::string & node_namespace = "beefast");

    void manageDatabase(
        const std::shared_ptr<ManageDbService::Request> request,
        std::shared_ptr<ManageDbService::Response> response);

    json callOperator(json request);

    void loadOperator();

protected:
    void init_services();

private:
    // Best effort (non real-time safe) callback group, e.g., service callbacks.
    // Best effort (non real-time safe) callback group for callbacks that can possibly break
    // real-time requirements, for example, service callbacks.
    rclcpp::CallbackGroup::SharedPtr best_effort_callback_group_;

    std::shared_ptr<rclcpp::Executor> executor_;

    const std::string node_name_;

    const std::string namespace_;

    beefast_database::BeefastDB db_;
    // std::unique_ptr<BeefastDB> db_; 
    rclcpp::Service<ManageDbService>::SharedPtr manage_database_service_;

    // 服务名和回调函数的映射
    std::unordered_map<std::string, CallbackType> db_operators_;

    // 注册服务名和回调函数
    void registerOperator(const std::string& operatorName, CallbackType callback);

    json getMapList(json data);

    json getMap(json data);

    json addMap(json data);

    json updateMap(json data);

    json deleteMap(json data);

    json getMapRoom(json data);

    json addMapRoom(json data);

    json updateMapRoom(json data);

    json deleteMapRoom(json data);

    json getVirualZone(json data);

    json addVirualZone(json data);

    json deleteVirualZone(json data);

    json deleteAllVirualZone(json data);

};

} // namespace beefast_database
#endif // BEEFAST_DATABASE__DATABASE_SERVER_HPP_