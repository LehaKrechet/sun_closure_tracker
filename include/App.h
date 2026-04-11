#pragma once
#include <Engine.h>
#include <Receiver.h>
#include <memory>
#include <Recognizer.h>
#include <Sendler.h>

class App{
    private:
        std::unique_ptr<AEngine> engine;
        std::unique_ptr<ACmdReceiver> receiver;
        std::unique_ptr<ARecogniser> recogniser;
        std::unique_ptr<ASendler> sendler;
    public:
        App(std::unique_ptr<AEngine> e, std::unique_ptr<ACmdReceiver> r, std::unique_ptr<ARecogniser> rec, std::unique_ptr<ASendler> s) : receiver(std::move(r)), engine(std::move(e)), recogniser(std::move(rec)), sendler(std::move(s)) {}

        int run();
};