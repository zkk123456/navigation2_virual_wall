# V2.0

## 编译说明

将nlohmann目录拷贝到/usr/local/include目录下

## 创建数据库

执行 ros2 run beefast_database  createdb 可以创建数据库，注意修改源码中的相关路径

ros2 run beefast_database   testsrv 测试相关service

## 测试

启动数据库进程：

ros2 run beefast_database beefast_database_node

python3 test.py  --op getallmap
python3 test.py  --op addmap
python3 test.py  --op getmap


# V1.0

## database库提供的服务：
### 数据结构

服务名称：
    /database_service

GetDatabase
uint8 WRITE= 0
uint8 DELETE = 1
uint8 UPDATE= 2
uint8 QUERY= 3

uint8 command
string type #map_service,virual_wall

string request_data
---
string status
string response_data

### 示例
参数结果：
实例1：根据map_id查询所有的虚拟墙信息：
    uint8 command = 3
    string type = "virual_wall"
    string request_data="map_id=1"
    ---
    string status = success 或 failed
    string response_data //所有虚拟墙数据，用";"分割不同记录，","分割不同字段  


实例2：修改地图名称，
    uint8 command = 2
    string type = "map_service"
    string request_data="map_id=1,old_name=,new_name="
    ---
    string status = success 或 failed
    string response_data //无
实例3：删除地图
    uint8 command = 1
    string type = "map_service"
    string request_data="map_id=1"
    ---
    string status = success 或 failed
    string response_data //无
实例4：保存地图
    uint8 command = 0
    string type = "map_service"
    string request_data="map_name=xxx,origin_file_name=xxx"
    ---
    string status = success 或 failed
    string response_data //无    

实例5：添加虚拟墙数据：
            uint8 command = 0
            string type = "virtual_wall_service"
            string request_data="map_id=1,zone_mode=1,zone_type=3,zone_coordinates=(1.0 0)(0 0)(0 0)(0 1.0)"
            ---
            string status = success 或 failed
            string response_data //无

实例6：根据zone_id删除虚拟墙信息：
            uint8 command = 1
            string type = "virtual_wall_service"
            string request_data="zone_id=1"
            ---
            string status = success 或 failed
            string response_data //无

实例7：增加虚拟区
ros2 service call /database_service beefast_interfaces/srv/GetDatabase "{command: 0, type: 'virtual_wall_service', request_data: 'map_id=1,zone_mode=1,zone_type=3,zone_coordinates=(1.0 0)(0 0)(0 0)(0 1.0)'}"

实例8：删除虚拟墙数据
ros2 service call /database_service beefast_interfaces/srv/GetDatabase "{command: 1, type: 'virtual_wall_service', request_data: 'zone_id=8'}"

实例8：查询指定地图的虚拟墙
ros2 service call /database_service beefast_interfaces/srv/GetDatabase "{command: 3, type: 'virual_wall', request_data: '1'}"

实例8：修改地图名
ros2 service call /database_service beefast_interfaces/srv/GetDatabase "{command: 2, type: 'map_service', request_data: 'map_id=1,old_name=nihao,new_name=buhao'}"

实例9： 修改地图名
ros2 service call /database_service beefast_interfaces/srv/GetDatabase "{command: 1, type: 'map_service', request_data: 'map_id=1'}"

实例10： 保存map
ros2 service call /database_service beefast_interfaces/srv/GetDatabase "{command: 0, type: 'map_service', request_data: 'map_name=map13,origin_map_name=map13_zone'}"

INSERT INTO maps (map_name, file_name) VALUES('test','test');


ros2 service call /db_list_maps beefast_interfaces/srv/ListAllMap


