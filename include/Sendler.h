#include <iostream>
#pragma once
class ASendler{
    public:
        // virtual std::string message() = 0;
        virtual std::string send(std::string msg = "test") = 0;
        virtual ~ASendler() = default;


};

class FooSendler : public ASendler{
    public:
        std::string send(std::string msg = "test");

};