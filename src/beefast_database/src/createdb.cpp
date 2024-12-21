#include "beefast_database/beefast_db.hpp"
int main() {
    beefast_database::BeefastDB db("/opt/beefast/db/beefast.db");

    db.createRooms();
    db.createRoomIndexAndTrigger();
    db.createMap();
    db.createIndexMap();
    db.createMapZones();
    db.createTrigger();
    return 0;
}