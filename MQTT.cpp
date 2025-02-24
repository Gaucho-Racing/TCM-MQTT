#include <mqtt/async_client.h>
#include <string> 
#include <iostream>
#include <random> // for random data
// #include <thread>  // for sleep

#include <sqlite3.h>

using namespace std;

const string serverURI("tcp://localhost:1883");
const string clientID("jetson_client");
const string persistDir("./persist/");

const string dbName("grdata.db");
const string tableName = "gr25";

mqtt::async_client client(serverURI,clientID,persistDir);
sqlite3 *db; 
// CREATE TABLE IF NOT EXISTS gr25 (timestamp INTEGER, topic TEXT, data BLOB, synced INTEGER);


void connect(){
    if(sqlite3_open(dbName.c_str(), &db) == SQLITE_OK){
        cout << "Database connected!" << endl;
    } else {
        cerr << "Failed to connect to database." << endl;
        return;
    }

    try{
        mqtt::connect_options options;
        options.set_clean_session(false); 
        client.connect(options)->wait(); 
        cout << "MQTT Connected!" << endl;
    } catch(const mqtt::exception& e) {
        cerr << "MQTT error: " << e.what() << endl;
        cerr << "Failed to connect to MQTT." << endl;
        return;
    }
}

void disconnect(){
    sqlite3_close(db);
    cout << "Disconnected from database." << endl;
    client.disconnect()->wait();
    cout << "Disconnected from MQTT." << endl;
}

void publishData(string nodeID, string messageID, const uint8_t arr[], int length){
    string topic = "gr25/gr25-main/" + nodeID + "/" + messageID; 
    int timestamp = 0;
    bool synced = 0;

    sqlite3_stmt* stmt;
    string query = "INSERT INTO " + tableName + " (timestamp, topic, data, synced) VALUES (CAST((julianday('now') - 2440587.5)*86400000000 AS INTEGER), ?, ?, ?);";

    sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, topic.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_blob(stmt, 2, arr, length, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, synced);
    sqlite3_step(stmt); // add error checking? 
    sqlite3_finalize(stmt);

    try {
        client.publish(topic, arr, length, 1, true)->wait();
        cout << "Published to " << topic << endl;
    } catch (const mqtt::exception &e) {
        cerr << "MQTT error: " << e.what() << endl;
    }
}

int main(){
    connect();

    random_device rd;
    mt19937 generator(rd());
    uniform_int_distribution<uint8_t> dist(0, 255);
    uint8_t arr[8];
    for (int j = 0; j < 8; j++) {
        arr[j] = dist(generator); 
    }

    publishData("ecu","0x01", arr, 8);
    disconnect();
    return 0;
}