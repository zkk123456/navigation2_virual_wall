## 编译方式
    colcon build --merge-install --cmake-args -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON --packages-select beefast_filter_mask

## 编译方式
    cd ~/beefast
    source install/local_setup.sh
    ros2 run beefast_filter_mask virual_wall_node

## 功能
	根据/map和虚拟器信息生成复合的costmap，并通过/keepout_filter_mask话题发布nav_msgs::msg::OccupancyGrid格式的信息
