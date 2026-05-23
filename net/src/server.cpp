#include <iostream>
#include "httplib.h"
#include "json.hpp"
#include "gpio_control.h"

using json = nlohmann::json;

struct start_command {};
struct stop_command {};

class EngineStateMachine {
private:
    enum class State { STOPPED, RUNNING } state = State::STOPPED;

public:
    // Обработка команды СТАРТ
    void process_event(start_command const&) {
        switch (state) {
            case State::STOPPED:
                std::cout << "Переход: STOPPED -> RUNNING. Включаем мотор." << std::endl;
                start();
                state = State::RUNNING;
                break;
            case State::RUNNING:
                std::cout << "Мотор уже запущен. Игнорируем дубликат команды." << std::endl;
                break;
        }
    }

    // Обработка команды СТОП
    void process_event(stop_command const&) {
        switch (state) {
            case State::RUNNING:
                std::cout << "Переход: RUNNING -> STOPPED. Выключаем мотор." << std::endl;
                stop();
                state = State::STOPPED;
                break;
            case State::STOPPED:
                std::cout << "Мотор уже остановлен. Игнорируем дубликат команды." << std::endl;
                break;
        }
    }

    // Вспомогательный метод, чтобы отдавать текущий статус по HTTP GET
    std::string get_status_string() const {
        return (state == State::RUNNING) ? "running" : "stopped";
    }
};

int main(int argc, char* argv[]) {
    httplib::Server svr;
        // Значения по умолчанию
    std::string ip = "0.0.0.0";  // Слушаем все интерфейсы
    int port = 8080;
    
    // Парсим параметры командной строки
    if (argc >= 2) {
        ip = argv[1];  // Первый параметр - IP
    }
    if (argc >= 3) {
        port = std::stoi(argv[2]);  // Второй параметр - порт
    }
    
    EngineStateMachine engine_fsm;
    
    // GET: Получить текущее состояние мотора
    svr.Get("/commands", [&engine_fsm](const httplib::Request& req, httplib::Response& res) {
        json response_json;
        response_json["engine_status"] = engine_fsm.get_status_string();
        res.set_content(response_json.dump(), "application/json");
        res.status = 200;
    });
   
    // POST: Отправить команду автомату
    svr.Post("/commands", [&engine_fsm](const httplib::Request& req, httplib::Response& res) {
        if (req.get_header_value("Content-Type") != "application/json") {
            res.status = 415;
            res.set_content("Unsupported Content-Type. Expected application/json", "text/plain");
            return;
        }

        try {
            json request_json = json::parse(req.body);
            std::string cmd_name = request_json.value("command", "none");
            int id = request_json.value("id", 0);

            bool command_known = true;

            if (cmd_name == "start") {
                engine_fsm.process_event(start_command{});
            } else if (cmd_name == "stop") {
                engine_fsm.process_event(stop_command{});
            } else {
                command_known = false;
            }

            json response_json;
            if (command_known) {
                response_json["status"] = "success";
                response_json["message"] = "Command processed by Motor";
                response_json["current_engine_state"] = engine_fsm.get_status_string();
                res.status = 200;
            } else {
                response_json["status"] = "error";
                response_json["message"] = "Unknown command: " + cmd_name;
                res.status = 400; // Bad Request
            }
            
            res.set_content(response_json.dump(), "application/json");

        } catch (const json::parse_error& e) {
            res.status = 400;
            res.set_content("Invalid JSON format: " + std::string(e.what()), "text/plain");
        }
    });

    std::cout << "Server listening on http://" << ip << ":" << port << std::endl;
    svr.listen(ip, port);
    return 0;
}