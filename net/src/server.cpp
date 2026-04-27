#include <iostream>
#include "httplib.h"
#include "json.hpp"
#include "gpio_control.h"

using json = nlohmann::json;

int main() {
    httplib::Server svr;
    
    svr.Get("/commands", [](const httplib::Request& req, httplib::Response& res) {
        std::string str = req.body;
        std::cout << "Get command !!!! "<< str;
        res.set_content("Hello !!!", "text/plain");
    });
   
    
    svr.Post("/commands", [](const httplib::Request& req, httplib::Response& res) {
        std::string request_body = req.body;
        bool engine_stat = 0;
        int ret;
        if (req.get_header_value("Content-Type") != "application/json") {
            res.status = 415; // Unsupported Media Type
            res.set_content("Unsupported Content-Type. Expected application/json", "text/plain");
            return;
        }
        try {
            json request_json = json::parse(request_body);
            // Access data from the JSON object
            std::string name = request_json.value("command", "none");
            float cmdtime    = request_json.value("time", 0.0);
            int id = request_json.value("id", 0);
            if (name == "start"){
                engine_stat = 1;
                start();
            }else if (name == "stop"){
                engine_stat = 0;
                stop();
            }
            
            std::cout << "command: " << name << std::endl;
            std::cout << "stat: " << engine_stat << std::endl;

            // Create a JSON response object
            json response_json;
            response_json["message"] = "Command received successfully: " + name;
            response_json["received_id"] = id;
            response_json["status"] = "success";
            res.set_content(response_json.dump(), "application/json");
                        
            res.status = 200; // OK
        } catch (const json::parse_error& e) {
            res.status = 400; // Bad Request
            res.set_content("Invalid JSON format: " + std::string(e.what()), "text/plain");
        }
    });
    std::cout << "Server listening on http://localhost:8080" << std::endl;
    std::cout << "Try: curl -X POST -H \"Content-Type: application/json\" -d '{\"command\": \"forward\", \"time\": 10, \"id\": 123}' http://localhost:8080/commands" << std::endl;    
    gpio_list("/dev/gpiochip0");
    
    svr.listen("0.0.0.0", 8080);
    return 0;
}
