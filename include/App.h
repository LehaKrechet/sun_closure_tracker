#pragma once
#include <memory>
#include <Recognizer.h>
#include <Sendler.h>

class App{
    private:
        std::unique_ptr<ARecogniser> recogniser;
        std::unique_ptr<ASendler> sendler;
    public:
        App(std::unique_ptr<ARecogniser> rec, std::unique_ptr<ASendler> s) : recogniser(std::move(rec)), sendler(std::move(s)) {}

        int run();
};