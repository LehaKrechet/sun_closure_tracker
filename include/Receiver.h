#include <iostream>
#pragma once
class ACmdReceiver{
    public:
        virtual std::string receiver() = 0;
        virtual ~ACmdReceiver() = default;


};

class FooCmdReceiver : public ACmdReceiver{
    public:
        std::string receiver();
};