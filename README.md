navigation2的虚拟墙服务包含database，filter_mask，costmap_filter_info_server和virual_wall_service等功能包。其具体功能如下：

database：提供虚拟墙数据的增加和删除，同时提供查询虚拟墙服务；
filter_mask：输入/map和虚拟墙数据，输出nav_msgs::msg::OccupancyGrid格式的复合地图信息；
costmap_filter_info_server:通过发送/costmap_filter_info话题，通知导航更新成本地图；
virual_wall_service：提供虚拟墙节点的启动和关闭接口(costmap_filter_info_server,beefast_filter_mask的开启与关闭)； 提供动态更新虚拟墙接口，能够动态更新导航的虚拟墙信息：

实现流程图如下：
![96868d50f08445719112b4d1df2af1a4](https://github.com/user-attachments/assets/e0670e44-025e-4818-b7b2-63fa83b5366c)
