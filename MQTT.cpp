#include <mqtt/async_client.h>
#include <string> 
#include <iostream>
#include <random> 
#include <chrono>
#include <sqlite3.h>
#include <vector>
#include <thread>  

using namespace std;

const string serverURI("tcp://localhost:1883"); // replace 
 

const string clientID("jetson_client");
const string persistDir("./persist/");
const string dbName("grdata.db");
const string tableName("gr25");
mqtt::async_client client(serverURI,clientID, persistDir);
sqlite3 *db; 

const uint8_t uploadKey[2] = {69, 69}; // doesnt do anything rn
const string mqttPassword("fortnite"); // doesnt do anything rn


class callback : public virtual mqtt::callback {
    void connection_lost(const string& cause){
        cerr << "Callback: MQTT connection lost!" << cause << endl;
    }
    void connected(const string& cause){
        cout << "Callback: MQTT connected!" << endl;
    }
};

long long getTimestamp(){
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return ms;
}

void connectDB(){
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
    cout << endl;
}

void connectMQTT(){
    try{
        mqtt::connect_options options;
        options.set_clean_session(false); 
        options.set_automatic_reconnect(true);
        options.set_keep_alive_interval(1); // 60 if unspecified
        options.set_password(mqttPassword); 
        client.connect(options)->wait(); 
    } catch(const mqtt::exception& e) {
        cerr << e.what() << endl; // failed to connect 
        return;
    }
    cout << endl;
}

void disconnect(){
    sqlite3_close(db);
    cout << "Disconnected from database." << endl;
    client.disconnect()->wait();
    cout << "Disconnected from MQTT." << endl;
}

void publishData(string nodeID, string messageID, const uint8_t arr[], int length){ // change 

    string topic = "gr25/gr25-main/" + nodeID + "/" + messageID; 
    bool synced = 0;

    sqlite3_stmt* stmt;
    string query = "INSERT INTO " + tableName + " (timestamp, topic, data, synced) VALUES (?, ?, ?, ?);";

    long long timestamp = getTimestamp();
    
    sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, timestamp);
    sqlite3_bind_text(stmt, 2, topic.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_blob(stmt, 3, arr, length, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, synced);
    sqlite3_step(stmt); 
    sqlite3_finalize(stmt);
    // cout << "Saved: " << timestamp << endl;

    int size = 10+length;
    vector<uint8_t> m(size, 0);

    for (int i = 7; i >= 0; i--) {
        m[7-i] = timestamp >> ((i * (8))) & 0xFF;
    }
    for (int i = 8; i < 10; i++) {
        m[i] = uploadKey[i-8];
    }
    for (int i = 10; i < size; i++) {
        m[i] = arr[i-10];
    }

    try {

        // cout << "Attempting to publish: " << timestamp << " - ";
        // for(int i = 0; i < length+10; i++){
        //     cout << int(m[i]) << " ";
        // }
        // cout << endl;

        // cout << client.get_server_uri() << endl;
        client.publish(topic, m.data(), size, 1, true);
        cout << "Published: " << timestamp << " to " << topic << endl; 
    } catch (const mqtt::exception &e) {
        cout << "Publish failed, reconnecting: " << timestamp << endl;
        cerr << e.what() << endl; // not connected, MQTT error [-3]: Disconnected
        connectMQTT(); // unneeded as should auto reconnect, but this makes it faster
    }
}


void test(){
 
    random_device rd;
    mt19937 generator(rd());
    uniform_int_distribution<uint8_t> dist(0, 255);

    int len;
    while(true){
        len = (dist(generator)%64)+1;
        uint8_t arr[len];

        for (int j = 0; j < len; j++) {
            arr[j] = dist(generator); 
        }

        publishData("ecu","0x01", arr, len);


        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); 
        //break;
    }
}

void test2(){ // poor length, edge cases for length
    random_device rd;
    mt19937 generator(rd());
    uniform_int_distribution<uint8_t> dist(0, 255);

    int len;
    while(true){
        len = (dist(generator)+1)%5;
        uint8_t arr[len];

        for (int j = 0; j < len; j++) {
            arr[j] = dist(generator); 
            cout << int(arr[j]) << " ";
        }

        publishData("ecu","0x01", arr, len+10);
        publishData("ecu","0x01", arr, -1);
        publishData("ecu","0x01", arr, 1);

        std::this_thread::sleep_for(std::chrono::seconds(20)); // 1 second delay
    }
}

int main(){
    connectDB();
    callback cb;
    client.set_callback(cb); // set callback before connectMQTT();
    connectMQTT();

    test();
    disconnect();
    return 0;
}


