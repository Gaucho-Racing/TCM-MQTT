#include <mqtt/async_client.h>
#include <string> 
#include <iostream>
#include <random> // for random data
#include <thread>  // for sleep

const std::string serverURI("tcp://localhost:1883");
const std::string clientID("jetson_client");
const std::string persistDir("./localstorage/");
const std::string topic("ingest/ecu"); // test
const int interval(2); // publish interval


int main(){
    mqtt::async_client client(serverURI,clientID,persistDir);

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
            // [ add local storage logic ] 
            
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

    return 0;
}


