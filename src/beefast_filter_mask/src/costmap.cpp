#include "beefast_filter_mask/costmap.hpp"
#include "rclcpp/rclcpp.hpp"

namespace beefast_filter_mask
{
  std::string Costmap::getGlobalFrame() 
  {
    return global_frame_;
  }

  const nav2_costmap_2d::Costmap2D *Costmap::getCostmap() const
  {
    return &costmap_;
  }

  nav2_costmap_2d::Costmap2D *Costmap::getCostmap()
  {
    return &costmap_;
  }

  std::array<unsigned char, 256> init_translation_table()
  {
    std::array<unsigned char, 256> cost_translation_table;

    // lineary mapped from [0..100] to [0..255]
    for (size_t i = 0; i < 256; ++i)
    {
      cost_translation_table[i] =
          static_cast<unsigned char>(1 + (251 * (i - 1)) / 97);
    }

    // special values:
    cost_translation_table[0] = 0;                                // NO obstacle
    cost_translation_table[99] = 253;                             // INSCRIBED obstacle
    cost_translation_table[100] = 254;                            // LETHAL obstacle
    cost_translation_table[static_cast<unsigned char>(-1)] = 255; // UNKNOWN

    return cost_translation_table;
  }

  void Costmap::raytrace(int x0, int y0, int x1, int y1,
                    std::vector<PointInt> &cells)
  {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    PointInt pt;
    pt.x = x0;
    pt.y = y0;
    int n = 1 + dx + dy;
    int x_inc = (x1 > x0) ? 1 : -1;
    int y_inc = (y1 > y0) ? 1 : -1;
    int error = dx - dy;
    dx *= 2;
    dy *= 2;

    for (; n > 0; --n)
    {
        cells.push_back(pt);

        if (error > 0)
        {
            pt.x += x_inc;
            error -= dy;
        }
        else
        {
            pt.y += y_inc;
            error += dx;
        }
    }
  }

  void Costmap::polygonOutlineCells(const std::vector<PointInt> &polygon,std::vector<PointInt> &polygon_cells)
  {
      for (unsigned int i = 0; i < polygon.size() - 1; ++i)
      {
          raytrace(polygon[i].x, polygon[i].y, polygon[i + 1].x, polygon[i + 1].y, polygon_cells);
      }
      if (!polygon.empty())
      {
          unsigned int last_index = polygon.size() - 1;
          raytrace(polygon[last_index].x, polygon[last_index].y, polygon[0].x, polygon[0].y, polygon_cells);
      }
  }

  void Costmap::rasterizePolygon(const std::vector<PointInt> &polygon,std::vector<PointInt> &polygon_cells,bool fill)
  {
    if (polygon.size() < 3)
    {
      RCLCPP_WARN(rclcpp::get_logger("Costmap"),"polygon.size() : %zu",polygon.size());
      return;
    }        
    polygonOutlineCells(polygon, polygon_cells);
  }

  void Costmap::updateProhibitedPolygon(const std::vector<geometry_msgs::msg::Point> &prohibition_polygon,
                                    std::vector<PointInt> &polygon_cells,const bool &fill_polygons)
  {
    //costmap_
    std::vector<PointInt> map_polygon;
    for (unsigned int i = 0; i < prohibition_polygon.size(); ++i)
    {
        PointInt loc;
        costmap_.worldToMapNoBounds(prohibition_polygon[i].x, prohibition_polygon[i].y, loc.x, loc.y);
        map_polygon.push_back(loc);
    }
    rasterizePolygon(map_polygon, polygon_cells, fill_polygons);
  }

  void Costmap::updateProhibitionCells(
      const std::vector<std::vector<geometry_msgs::msg::Point>> &prohibition_polygons,const bool &fill_polygons)
  {
      std::vector<PointInt> prohibited_cells;
      int min_i = 0;
      int min_j = 0;
      int max_i = costmap_.getSizeInCellsX();
      int max_j = costmap_.getSizeInCellsY();
      for (int i = 0; i < prohibition_polygons.size(); ++i)
      {
          std::vector<PointInt> polygon_cells;
          updateProhibitedPolygon(prohibition_polygons[i], polygon_cells, fill_polygons);
          for (std::vector<PointInt>::iterator it = polygon_cells.begin(); it != polygon_cells.end(); ++it)
          {
              int mx = it->x;
              int my = it->y;
              if (mx < min_i || mx >= max_i)
                  continue;
              if (my < min_j || my >= max_j)
                  continue;
              prohibited_cells.push_back(*it);
          }
      }
    unsigned char *costmap_data = costmap_.getCharMap();
    for (unsigned int i = 0; i < prohibited_cells.size(); ++i)
    {
        costmap_data[prohibited_cells[i].y * (max_i - min_i) + prohibited_cells[i].x] = LETHAL_OBSTACLE;
    }
  }

  static const std::array<unsigned char, 256> cost_translation_table__ = init_translation_table();

  // prepare grid_ message for publication.
  void Costmap::prepareGrid()
  {
    std::unique_lock<nav2_costmap_2d::Costmap2D::mutex_t> lock(*(costmap_.getMutex()));
    grid_resolution = costmap_.getResolution();
    grid_width = costmap_.getSizeInCellsX();
    grid_height = costmap_.getSizeInCellsY();

    grid_ = std::make_unique<nav_msgs::msg::OccupancyGrid>();
    grid_->header.frame_id = global_frame_;    
    grid_->header.stamp = rclcpp::Clock().now();
    grid_->info.resolution = grid_resolution;
    grid_->info.width = grid_width;
    grid_->info.height = grid_height;

    double wx, wy;
    costmap_.mapToWorld(0, 0, wx, wy);
    grid_->info.origin.position.x = wx - grid_resolution / 2;
    grid_->info.origin.position.y = wy - grid_resolution / 2;
    grid_->info.origin.position.z = 0.0;
    grid_->info.origin.orientation.w = 1.0;

    grid_->data.resize(grid_->info.width * grid_->info.height);

    if (cost_to_map_translation_table_== NULL)
    {
        cost_to_map_translation_table_= new char[256];
        // special values:
        cost_to_map_translation_table_[0] = 0;  // NO obstacle
        cost_to_map_translation_table_[253] = 99;  // INSCRIBED obstacle
        cost_to_map_translation_table_[254] = 100;  // LETHAL obstacle
        cost_to_map_translation_table_[255] = -1;  // UNKNOWN
        // regular cost values scale the range 1 to 252 (inclusive) to fit
        // into 1 to 98 (inclusive).
        for (int i = 1; i < 253; i++) 
        {
          cost_to_map_translation_table_[i] = static_cast<char>(1 + (97 * (i - 1)) / 251);
        }
    }
    unsigned char * data = costmap_.getCharMap();
    for (unsigned int i = 0; i < grid_->data.size(); i++) 
    {
      grid_->data[i] = cost_to_map_translation_table_[data[i]];
    }
    return ;
  }

  void Costmap::updateFullMap(const nav_msgs::msg::OccupancyGrid::SharedPtr msg)
  {
    global_frame_ = msg->header.frame_id;

    unsigned int size_in_cells_x = msg->info.width;
    unsigned int size_in_cells_y = msg->info.height;
    double resolution = msg->info.resolution;
    double origin_x = msg->info.origin.position.x;
    double origin_y = msg->info.origin.position.y;
    costmap_.resizeMap(size_in_cells_x, size_in_cells_y, resolution, origin_x,
                       origin_y);
    auto *mutex = costmap_.getMutex();
    std::lock_guard<nav2_costmap_2d::Costmap2D::mutex_t> lock(*mutex);

    //fill map with data
    unsigned char *costmap_data = costmap_.getCharMap();
    size_t costmap_size = costmap_.getSizeInCellsX() * costmap_.getSizeInCellsY();

    // 映射函数
    auto map_value_mapping = [](unsigned char cell_cost) {
      if (cell_cost > 30 && cell_cost <= 100) 
      {
        return 100;  // 大于50的映射为100
      } else if (cell_cost >= 0 && cell_cost <= 30){
        return 0;    // 小于等于50的映射为0
      }else
      {
        return 255;
      }
    };
    for (size_t i = 0; i < costmap_size && i < msg->data.size(); ++i)
    {
      // unsigned char cell_cost = static_cast<unsigned char>(msg->data[i]);
      // costmap_data[i] = cost_translation_table__[cell_cost];
      unsigned char cell_cost = static_cast<unsigned char>(msg->data[i]);
      costmap_data[i] = cost_translation_table__[map_value_mapping(cell_cost)];
    }
  }

  void Costmap::updatePartialMap(const map_msgs::msg::OccupancyGridUpdate::SharedPtr msg)
  {
    global_frame_ = msg->header.frame_id;
    if (msg->x < 0 || msg->y < 0)
    {
      return;
    }
    size_t x0 = static_cast<size_t>(msg->x);
    size_t y0 = static_cast<size_t>(msg->y);
    size_t xn = msg->width + x0;
    size_t yn = msg->height + y0;

    // lock as we are accessing raw underlying map
    auto *mutex = costmap_.getMutex();
    std::lock_guard<nav2_costmap_2d::Costmap2D::mutex_t> lock(*mutex);

    size_t costmap_xn = costmap_.getSizeInCellsX();
    size_t costmap_yn = costmap_.getSizeInCellsY();

    if (xn > costmap_xn || x0 > costmap_xn || yn > costmap_yn ||
        y0 > costmap_yn)
    {
      RCLCPP_WARN(rclcpp::get_logger("Costmap"), "received update doesn't fully fit into existing map, "
               "only part will be copied. received: [%lu, %lu], [%lu, %lu] "
               "map is: [0, %lu], [0, %lu]",
               x0, xn, y0, yn, costmap_xn, costmap_yn);
    }

    // update map with data
    unsigned char *costmap_data = costmap_.getCharMap();

  // 映射函数
  auto map_value_mapping = [](unsigned char cell_cost) {
    if (cell_cost > 62 && cell_cost <= 100) {
      return 100;  // 大于50的映射为100
    } else if (cell_cost >= 0 && cell_cost <= 30){
      return 0;    // 小于等于50的映射为0
    }else
    {
      return 255;
    }
  };

    size_t i = 0;
    for (size_t y = y0; y < yn && y < costmap_yn; ++y)
    {
      for (size_t x = x0; x < xn && x < costmap_xn; ++x)
      {
        // size_t idx = costmap_.getIndex(x, y);
        // unsigned char cell_cost = static_cast<unsigned char>(msg->data[i]);
        // costmap_data[idx] = cost_translation_table__[cell_cost];
        // ++i;
      size_t idx = costmap_.getIndex(x, y);
      unsigned char cell_cost = static_cast<unsigned char>(msg->data[i]);
      costmap_data[idx] = cost_translation_table__[map_value_mapping(cell_cost)];
      ++i;
      }
    }    
  }


  

} // namespace beefast_filter_mask
