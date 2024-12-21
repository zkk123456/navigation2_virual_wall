#pragma once

#include <chrono>
#include "geometry_msgs/msg/point.hpp"
#include "rclcpp/rclcpp.hpp"
#include "nav_msgs/msg/occupancy_grid.hpp"
#include "map_msgs/msg/occupancy_grid_update.hpp"

#include "nav2_costmap_2d/costmap_2d.hpp"
#include "nav2_costmap_2d/cost_values.hpp"


namespace beefast_filter_mask
{
    using nav2_costmap_2d::FREE_SPACE;
    using nav2_costmap_2d::LETHAL_OBSTACLE;
    using nav2_costmap_2d::NO_INFORMATION;
    struct PointInt
    {
        int x;
        int y;
    };

    class Costmap
    {
    public:
        void updateFullMap(const nav_msgs::msg::OccupancyGrid::SharedPtr msg);

        void updatePartialMap(const map_msgs::msg::OccupancyGridUpdate::SharedPtr msg);

        void prepareGrid();

        void updateProhibitionCells(const std::vector<std::vector<geometry_msgs::msg::Point>> &prohibition_polygons,
                                const bool &fill_polygons);

        void updateProhibitedPolygon(const std::vector<geometry_msgs::msg::Point> &prohibition_polygon,
                                    std::vector<PointInt> &polygon_cells,const bool &fill_polygons);

        void rasterizePolygon(const std::vector<PointInt> &polygon,
                            std::vector<PointInt> &polygon_cells,
                            bool fill);

        void polygonOutlineCells(const std::vector<PointInt> &polygon,
                                std::vector<PointInt> &polygon_cells);

        void raytrace(int x0, int y0, int x1, int y1,
                    std::vector<PointInt> &cells);

        const nav2_costmap_2d::Costmap2D *getCostmap() const;

        nav2_costmap_2d::Costmap2D *getCostmap();   

        std::string getGlobalFrame();

        std::unique_ptr<nav_msgs::msg::OccupancyGrid> grid_;

    private:
        nav2_costmap_2d::Costmap2D costmap_;

        std::string global_frame_;

        float grid_resolution;
        unsigned int grid_width, grid_height;

        char* cost_to_map_translation_table_ = NULL;

        char* cost_translation_table_ = NULL;
    };

} // namespace beefast_filter_mask
