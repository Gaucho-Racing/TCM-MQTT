#include <mqtt/async_client.h>
#include <string> 
#include <iostream>
#include <random> // for random data
#include <thread>  // for sleep

#include <sqlite3.h>

const std::string serverURI("tcp://localhost:1883");
const std::string clientID("jetson_client");
const std::string persistDir("./localstorage/");
const std::string topic("ingest/ecu"); // test
const int interval(2); // publish interval

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
    int i;
    for(i=0; i<argc; i++){
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}  // ripped from https://www.sqlite.org/quickstart.html

std::string byteArrayToBinaryString(const uint8_t* arr) {
    // converts byte array to 64 length string
    std::string binaryStr;
    for (int i = 0; i < 8; i++) { 
        for (int j = 7; j >= 0; j--) { 
            if (arr[i] & (1 << j)) {
                binaryStr += '1';
            } else {
                binaryStr += '0';
            }
        }
    }
    return binaryStr;
}

int main(){
    mqtt::async_client client(serverURI,clientID,persistDir);
    sqlite3 *db; // database connector pointer

    sqlite3_open("test.db", &db);
    // CREATE TABLE saved_data (id INTEGER PRIMARY KEY AUTOINCREMENT, data TEXT CHECK (LENGTH(data) = 64) );
    
    char* error = nullptr;
    try{
        std::cout << "Connecting..." << std::endl;
        mqtt::connect_options options;
        options.set_clean_session(true);
        client.connect(options)->wait();
        std::cout << "Success!" << std::endl;

        std::random_device rd;
        std::mt19937 generator(rd());
        std::uniform_int_distribution<uint8_t> dist(0, 255); // 1 byte = 8 bits = 2^8 = 256 

        while(true){
            uint8_t arr[8];
            for (int j = 0; j < 8; j++) {
                arr[j] = dist(generator); 
            }
            std::string binary_string = byteArrayToBinaryString(arr);

            sqlite3_stmt* stmt;
            const char* query = "INSERT INTO saved_data (data) VALUES (?);";
            sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
            sqlite3_bind_text(stmt, 1, binary_string.c_str(), -1, SQLITE_STATIC);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            

            client.publish(topic, arr, 8, 1, true)->wait(); // alternatively you can use message_ptr and make_message 
            std::cout << "Published to " << topic << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(interval)); // publish interval
        }
        
        client.disconnect()->wait();
        std::cout << "Disconnected." << std::endl;

    } catch(const mqtt::exception& e) {
        std::cout << "error" << std::endl;
        return -1;
    }

    sqlite3_close(db);

    return 0;
}


