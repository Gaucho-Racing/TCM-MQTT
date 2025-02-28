#include <mqtt/async_client.h>
#include <string> 
#include <iostream>
#include <random> // for random data
#include <chrono>
// #include <thread>  // for sleep
#include <sqlite3.h>
#include <vector>

using namespace std;

const int vehicleID = 6969;
const int uploadKey = 1234;
const string serverURI("tcp://localhost:1883");
const string clientID("jetson_client");
const string persistDir("./persist/");
const string dbName("grdata.db");
const string tableName("gr25");
mqtt::async_client client(serverURI,clientID,persistDir);
sqlite3 *db; 

// CREATE TABLE IF NOT EXISTS gr25 (timestamp INTEGER, topic TEXT, data BLOB, synced INTEGER);

long long getTimestamp(){
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return ms;
}
void connect(){
    if(sqlite3_open(dbName.c_str(), &db) == SQLITE_OK){
        cout << "Database connected!" << endl;

        const string createTableQuery = 
            "CREATE TABLE IF NOT EXISTS gr25 ("
            "timestamp INTEGER, "
            "topic TEXT, "
            "data BLOB, "
            "synced INTEGER);";
            
        sqlite3_stmt *stmt;
        if (sqlite3_prepare_v2(db, createTableQuery.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            cout << "Table created or exists already!" << endl;
        } else {
            cerr << "Error creating table!" << endl;
        }

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
    bool synced = 0;

    try {
        client.publish(topic, arr, length, 1, true)->wait();
        cout << "Published to " << topic << endl;
    } catch (const mqtt::exception &e) {
        cerr << "MQTT error: " << e.what() << endl;
    }

    sqlite3_stmt* stmt;
    string query = "INSERT INTO " + tableName + " (timestamp, topic, data, synced) VALUES (?, ?, ?, ?);";

    long long timestamp = getTimestamp();
    
    sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, timestamp);
    sqlite3_bind_text(stmt, 2, topic.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_blob(stmt, 3, arr, length, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, synced);
    sqlite3_step(stmt); // add error checking? 
    sqlite3_finalize(stmt);

    int size = 10+length;
    vector<uint8_t> m(size, 0);
    for (int i = 7; i >= 0; i--) {
        m[7-i] = timestamp >> ((i * (8))) & 0xFF;
    }

    for (int i = 10; i < size; i++) {
        m[i] = arr[i-10];
    }

    // std::cout << "Byte representation: ";
    // for (unsigned char byte : m) {
    //     std::cout << std::hex << static_cast<int>(byte) << " ";  // Print in hex format
    // }
    // std::cout << std::endl; 

    try {
        // client.publish(topic, arr, length, 1, true)->wait();
        client.publish(topic, m.data(), size, 1, true)->wait();

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

