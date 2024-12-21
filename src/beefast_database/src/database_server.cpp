#include "beefast_database/database_server.hpp"

namespace beefast_database {

// Changed services history QoS to keep all so we don't lose any client service calls
static const rmw_qos_profile_t rmw_qos_profile_services_hist_keep_all = {
    RMW_QOS_POLICY_HISTORY_KEEP_ALL,
    5,  // message queue depth
    RMW_QOS_POLICY_RELIABILITY_RELIABLE,
    RMW_QOS_POLICY_DURABILITY_VOLATILE,
    RMW_QOS_DEADLINE_DEFAULT,
    RMW_QOS_LIFESPAN_DEFAULT,
    RMW_QOS_POLICY_LIVELINESS_SYSTEM_DEFAULT,
    RMW_QOS_LIVELINESS_LEASE_DURATION_DEFAULT,
    false};

// 构造函数 DatabaseServer，用于初始化 DatabaseServer 类的对象
// 参数：
// - executor：指向 rclcpp::Executor 的共享指针，用于执行回调函数
// - t_db_path：数据库的路径，类型为 std::string
// - node_name：节点名称，类型为 std::string
// - node_namespace：节点的命名空间，类型为 std::string
DatabaseServer::DatabaseServer(
    std::shared_ptr<rclcpp::Executor> executor,
    const std::string & t_db_path,
    const std::string & node_name, const std::string & node_namespace) 
    : rclcpp::Node(node_name, node_namespace), executor_(executor), 
      db_(t_db_path){
        // 调用 init_services 函数进行服务的初始化
        init_services();
    }

// 函数 init_services 用于初始化服务
// 该函数属于 DatabaseServer 类
void DatabaseServer::init_services() {
    // 创建一个互斥的回调组
    best_effort_callback_group_ = create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
    using namespace std::placeholders;

    // 创建一个名为 "/database/service" 的服务，使用 manageDatabase 作为回调函数
    // 服务的 QoS 配置为 rmw_qos_profile_services_hist_keep_all，使用 best_effort_callback_group_ 作为回调组
    manage_database_service_ = create_service<ManageDbService>(
        "/database/service", std::bind(&DatabaseServer::manageDatabase, this, _1, _2),
        rmw_qos_profile_services_hist_keep_all, best_effort_callback_group_);
    
    // 调用 loadOperator 函数
    loadOperator();
}

// 函数 getMapList 是 DatabaseServer 类的成员函数
// @param data json 类型的数据，可能包含一些查询条件或其他相关信息
// 该函数的目的是从数据库中获取所有的地图索引信息
// 函数内部调用 db_ 对象的 getAllIndexMap 函数获取所有的地图索引信息
// 并将结果存储在 some_maps 变量中
// 最后将结果作为函数的返回值返回

json DatabaseServer::getMapList(json data) {
    json some_maps = db_.getAllIndexMap();
    return some_maps;
}

json DatabaseServer::getMap(json data) {
    json map = db_.getIndexMap(data["map_id"]);
    return map;
}

json DatabaseServer::addMap(json data) {
    db_.addIndexMap(data["name"], data["file_name"], data["start_point"]);
    return json();
}

json DatabaseServer::updateMap(json data) {
    db_.updateIndexMap(data["map_id"], data["name"]);
    return json();
}

json DatabaseServer::deleteMap(json data) {
    db_.removeIndexMap(data["map_id"]);
    return json();
}

json DatabaseServer::getMapRoom(json data) {
    json some_rooms = db_.getRooms(data["map_id"]);
    return some_rooms;
}

json DatabaseServer::addMapRoom(json data) {
    // std::string some_rooms = data.dump();
    db_.addRooms(data);
    return json();  
}

json DatabaseServer::updateMapRoom(json data) {
    db_.updateRoomName(data["map_id"], data["room_value"], data["room_name"]);
    return json();
}

json DatabaseServer::deleteMapRoom(json data) {
    db_.deleteRoom(data["map_id"], data["room_value"]);
    return json();   
}

 json DatabaseServer::getVirualZone(json data)
 {
    json some_rooms = db_.getVirualZones(data["data"]["map_id"]);
    return some_rooms;
 }

json DatabaseServer::addVirualZone(json data)
{
    db_.addVirualZone(data);
}

json DatabaseServer::deleteVirualZone(json data)
{
    db_.deleteVirualZone(data["data"]["zone_id"]);
    return json();   
}

json DatabaseServer::deleteAllVirualZone(json data)
{
    std::cout <<"deleteAllVirualZone " <<std::endl;
    db_.deleteAllVirualZone(data["data"]["map_id"]);
    return json();   
}


void DatabaseServer::registerOperator(const std::string& operator_name, 
    CallbackType callback) {
        db_operators_[operator_name] = callback;
}

json DatabaseServer::callOperator(json request) {
    // RCLCPP_INFO(get_logger(), "db operator size <%ld> ", db_operators_.size());
    // for (const auto& pair : db_operators_) {
    //     RCLCPP_INFO(get_logger(), "db operator key <%s> ", pair.first.c_str());
    // }

    const std::string& operator_name = request["op"];
    // RCLCPP_INFO(rclcpp::get_logger("DatabaseServer"), "op [%s];data [%s] ", operator_name.c_str(), request.dump().c_str());
    RCLCPP_INFO(rclcpp::get_logger("DatabaseServer"), "data [%s] ", request.dump().c_str());
    auto it = db_operators_.find(operator_name);
    if (it != db_operators_.end()) {
        return it->second(request); // 调用回调函数
    } else {
        RCLCPP_INFO(get_logger(), "db operator <%s> not found", operator_name.c_str());
    }    
}

void DatabaseServer::loadOperator() {
    // 注册服务名和对应的回调函数
    // 使用 lambda 表达式捕获 this 指针
    registerOperator("getallmap", [this](json data) { return this->getMapList(data);});
    registerOperator("getmap", [this](json data) { return this->getMap(data);});
    registerOperator("addmap", [this](json data) { return this->addMap(data);});
    registerOperator("updatemap", [this](json data) { return this->updateMap(data);});
    registerOperator("deletemap", [this](json data) { return this->deleteMap(data);});
    registerOperator("getroom", [this](json data) { return this->getMapRoom(data);});
    registerOperator("addroom", [this](json data) { return this->addMapRoom(data);});
    registerOperator("updateroom", [this](json data) { return this->updateMapRoom(data);});
    registerOperator("deleteroom", [this](json data) { return this->deleteMapRoom(data);});
    
    registerOperator("getzone", [this](json data) { return this->getVirualZone(data);});
    registerOperator("addzone", [this](json data) { return this->addVirualZone(data);});
    registerOperator("deletezone", [this](json data) { return this->deleteVirualZone(data);});
    registerOperator("deletezones", [this](json data) { return this->deleteAllVirualZone(data);});
    RCLCPP_INFO(get_logger(), "11 db operator size <%d>", db_operators_.size());
}

void DatabaseServer::manageDatabase(
    const std::shared_ptr<ManageDbService::Request> request,
    std::shared_ptr<ManageDbService::Response> response) {
    json some_request = json::parse(request->request_data);
    json some_response = callOperator(some_request);
    std::string result = some_response.dump();
    response->response_data = result;
}


}