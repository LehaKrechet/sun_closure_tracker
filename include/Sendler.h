#include <iostream>
#pragma once
class ASendler{
    public:
        virtual std::string send(std::string msg = "test") = 0;
        virtual ~ASendler() = default;


};

class FooSendler : public ASendler{
    private:
        std::string m_ip;
        std::string m_port;
    public:
        FooSendler(const std::string& ip = "0.0.0.0", std::string port = "8080") : m_ip(ip), m_port(port) {}
        std::string send(std::string msg = "test") override;
    

};