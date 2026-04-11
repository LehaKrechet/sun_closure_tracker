#include "Sendler.h"
#include <iostream>
#include <thread>
#include <chrono>
#include "httplib.h"
#include "nlohmann/json.hpp"


using json = nlohmann::json;

std::string FooSendler::send(std::string msg){
    httplib::Client client("http://localhost:8080");
    // std::string msg = "test";
    std::string status = "None";
    
    json data = {
        {"command", msg},
        {"id", 666},
        {"time", 1.00}
    };
    
    auto response = client.Post("/commands", data.dump(), "application/json");
    
    if (response && response->status == 200) {
        std::cout << "Success: " << response->body << std::endl;
        status = "Success";
    } else if (response) {
        std::cout << "Error " << response->status << ": " << response->body << std::endl;
        status = "Error";
    } else {
        std::cout << "Connection failed" << std::endl;
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return status;
}