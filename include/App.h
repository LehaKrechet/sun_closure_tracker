#pragma once
#include <Engine.h>
#include <Receiver.h>
#include <memory>

class App{
    private:
        std::unique_ptr<AEngine> engine;
        std::unique_ptr<ACmdReceiver> receiver;
    public:
        App(std::unique_ptr<AEngine> e, std::unique_ptr<ACmdReceiver> r) : receiver(std::move(r)), engine(std::move(e)) {}

        void run();
};