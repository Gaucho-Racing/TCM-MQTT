#include <mqtt/async_client.h>
#include <string> 
#include <iostream>

const std::string serverURI("tcp://localhost:1883");
const std::string clientID("jetson_client");
const std::string persistDir("./localstorage/");
const std::string topic("ingest/ecu"); // test

int main(){
    mqtt::async_client client(serverURI,clientID,persistDir);

    try{
        std::cout << "Connecting..." << std::endl;
        mqtt::connect_options options;
        options.set_clean_session(true);
        client.connect(options)->wait();
        std::cout << "Success!" << std::endl;


        // insert publisher and random data generator logic here
        // use whatever topic to test

        // in the future: implement local storage of generated random data(?)
        std::string message = "test";
        while (true) {
            mqtt::message_ptr pubMessage = mqtt::make_message(topic, message, 8, true);
            client.publish(pubMessage)->wait();

        }

        client.disconnect()->wait();
        std::cout << "Disconnected." << std::endl;

 
    } catch(const mqtt::exception& e) {
        std::cout << "error" << std::endl;
        return -1;
    }

    return 0;
}


