/*
'''
@Time    : 2024/5/23  18:45
@Author  : ZHANG KAI KAI
@Contact : 1936230913@qq.com
@Version : 0.3
@Language: c++
@Desc    : 禁行区域配置存储模块
  */
#ifndef BEEFAST_DATABASE__DATABASE_TOOL_HPP_
#define BEEFAST_DATABASE__DATABASE_TOOL_HPP_
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <string.h>
#include <algorithm>
#include <chrono> 
#include <unordered_map>

#include "beefast_interfaces/srv/get_database.hpp"
#include "rclcpp/rclcpp.hpp"


namespace beefast_database
{
    /*地图结构定义
    */
    struct MapData
    {
        int map_id;
        std::string map_name;
        std::string origin_map_name;
        std::string create_at;
        std::string update_at;
        std::string version;
        std::string start_point;
    };

    /*禁行区域定义
    */
    struct MapZone
    {
        int map_id;
        int zone_id;
        int zone_mode; //1-禁区 2-清扫分区
        int zone_type;
        std::string zone_coordinates;
    }; 
    
    /*索引地图结构定义
    */
    struct IndexMapData
    {
        int id;
        int map_id;
        std::string map_name;
        std::string origin_file_name;
        std::string create_at;
        std::string update_at;
        std::string version;
        std::string start_point;
    }; 

    /**
     * @brief 数据库操作回调函数，暂不处理
     *
     * @param[in] data 数据
     * @param[in] argc 参数数目
     * @param[in] argv 回调的参数
     * @param[in] azColName  回调的列名
     * @return static int  执行结果
     */
    static int callback(void *data, int argc, char **argv, char **azColName)
    {
        return 0;
    }

    using DatabaseService = beefast_interfaces::srv::GetDatabase;

    class DatabaseTool : public rclcpp::Node
    {
    public:
        /**
         * @brief 禁行区域数据库类的构造函数
         *
         * @param[in] n ros句柄
         */
        DatabaseTool(std::string database_file_path);

        /**
         * @brief 析构函数
         */
        ~DatabaseTool();

    public:
       /**
        * @brief 根据zoon_id删除表map_zones 中的记录
        *
        * @param[in] zone_id 删除记录的id
        * @return int 写入成功返回0，否则返回1
        */
        int RemoveByZoneId(int zone_id);

        /**@brief  根据zoon_id删除表map_zones 中的记录
         * 
         * @param[in] zone_ids 删除记录的id集合
         * @return int 写入成功返回0，否则返回1
         */
        int RemoveByZoneId(std::vector<int> zone_ids);

        /**@brief 根据map_id删除表maps中的记录
         * 
         * @param[in] map_id 删除记录的id
         * @return int 写入成功返回0，否则返回1
         */
        int RemoveByMapId(int map_id);

        /**@brief 根据map_id删除表maps中的记录
         * 
         * @param[in] map_id 删除记录的id
         * @return int 写入成功返回0，否则返回1
         */
        int RemoveByMapId(std::vector<int> map_ids);

        /**@brief 写入地图数据
         * 
         * @param[in] map_datas 地图数据
         * @return int 写入成功返回0，否则返回1
         */
        int WriteMapData(std::vector<MapData> map_datas);

        /**@brief 写入禁止区域数据
         * 
         * @param[in] zone_datas 禁止区域数据
         * @return int 写入成功返回0，否则返回1
         */
        int WriteZoneData(std::vector<MapZone> zone_datas);

        /**@brief 写入地图数据
         * 
         * @param[in] map_datas 地图数据
         * @return int 写入成功返回0，否则返回1
         */
        int WriteMapData(MapData map_data);

        /**@brief 写入禁止区域数据
         * 
         * @param[in] zone_datas 禁止区域数据
         * @return int 写入成功返回0，否则返回1
         */
        int WriteZoneData(MapZone zone_data);

        /**@brief 查所有的地图
         * 
         * @param[out] maps_data maps表中所有的记录
         * @return int 查询成功返回0，否则返回1
         */
        int QueryMapsData(std::vector<MapData> &maps_data);

        /**@brief 查指定map_id的禁行区域
         * 
         * @param[in] map_id 地图id
         * @param[out] zones_data map_id中所有的禁止区域
         * @return int 查询成功返回0，否则返回1
         */
        int QueryZoneByMapId(int map_id,std::vector<MapZone> &zones_data);
        
        /**@brief 查指定map_id的索引地图
         * 
         * @param[in] map_id 地图id
         * @param[out] index_map_data map_id相同的索引地图信息
         * @return int 查询成功返回0，否则返回1
         */
        int QueryIndexMapByMapId(int map_id,IndexMapData &index_map_data);
        /**@brief 查所有的的索引地图信息
         * 
         * @param[out] index_map_data 所有的索引地图信息
         * @return int 查询成功返回0，否则返回1
         */
        int QueryAllIndexMap(std::vector<IndexMapData> &index_map_data);

        //新提供的地图接口
        /**@brief 根据map_id更新 origin_map_name,同时更新update_at
         *        map_service
         * @param[in] map_id 删除记录的id
         * @param[in] origin_map_name 存储文件名称
         * @return int 写入成功返回0，否则返回1
         */
        int UpateMapName(int map_id,std::string old_name,std::string new_name);

        /**@brief 写地图记录到数据库
         *     map_service
         * @param[in] map_name 地图名
         * @return int 写入成功返回0，否则返回1
         */
        int SaveMapWriteData(std::string map_name,std::string origin_map_name,std::string start_point);

        int QueryMapById(int map_id,std::vector<MapData> &maps_data);

        /**@brief 根据map_id,删除数据库中地图信息
         * 
         * @param[in] map_name 地图名
         * @return int 写入成功返回0，否则返回1
         */
        int RemoveMapDataById(int map_id);

        /**@brief 根据map_id,删除数据库中的虚拟墙信息
         *
         * @param[in] map_id 要删除虚拟墙的地图id
         * @return int 写入成功返回0，否则返回1
         */
        int RemoveAllZoneByMapId(int map_id);

        void GetDatabaseData(const std::shared_ptr<DatabaseService::Request> request,std::shared_ptr<DatabaseService::Response> response);

        void GetCoordinate(std::string coordinate_data,std::vector<std::pair<double, double>> &coordinates);

        std::unordered_map<std::string, std::string> ParseKeyword(const std::string &input);

    private:
        sqlite3 *db; // 数据库对象
        std::string database_file_path_;
        std::string map_name_;

        rclcpp::Service<DatabaseService>::SharedPtr database_server_;

        std::string GetCurrentDateTimeString();

        

        /**
         * @brief 读数据
         *
         * @param[in] db 数据库对象
         * @param[in] str_sql 查询语句
         * @param[out] resultdata 查询的数据集合
         * @return int  查询成功返回SQLITE_OK，否则返回 1
         */
        int SelectData(sqlite3 *db, const std::string str_sql, std::vector<std::vector<std::string>> &resultdata);

        /**
         * @brief 删除表中的所有记录
         *
         * @param[in] db 数据库对象
         * @param[in] tablename 表名
         * @param[in] writedata 待删除的记录id
         * @return int  删除数据表中记录成功返回SQLITE_OK，否则返回 0
         */
        int DeleteData(sqlite3 *db, const std::string tablename, const std::vector<std::string> writedata);

        /**
         * @brief 写入数据
         *
         * @param[in] db 数据库对象
         * @param[in] tablename 表名
         * @param[in] writedata 要写入的数据集合
         * @return int  写入数据成功返回SQLITE_OK，否则返回0
         */
        int InsertData(sqlite3 *db, const std::string tablename, const std::vector<std::string> writedata);
    };

} // namespace beefast_database

#endif // BEEFAST_DATABASE__DATABASE_TOOL_HPP_
