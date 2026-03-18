#include <iostream>
#pragma once

class AEngine{
    public:
        virtual void start() = 0;
        virtual void stop() = 0;
        virtual ~AEngine() = default;
};
class FooEngine : public AEngine{
    public:
        void start();
        void stop();
};