#include "beefast_database/beefast_db.hpp"
int main() {
    beefast_database::BeefastDB db("/opt/beefast/db/beefast.db");

    const std::string& name="1st_floor";
    const std::string& file_name=name+".pbstream";
    const std::string& index_map_name=name+".pgm";

    db.addMap(name, file_name);
    db.addIndexMap(name, index_map_name, "100,100");
    auto json_lines = db.getIndexMap();
    for (const auto& line : json_lines) {
        std::cout << line.dump() << std::endl;
    }

    auto some_map = db.getIndexMap(1);
    std::cout << "id=1 的index_map: " << some_map.dump() << std::endl;
    std::cout << "update id=2:" << std::endl;
    db.updateIndexMap(2, "new_map");
    std::cout << "remove id=2:" << std::endl;
    db.removeIndexMap(2);

    return 0;
}