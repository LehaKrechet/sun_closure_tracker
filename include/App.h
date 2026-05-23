#pragma once
#include <memory>
#include <Recognizer.h>
#include <Sendler.h>

class App{
    private:
        std::unique_ptr<ARecogniser> recogniser;
        std::unique_ptr<ASendler> sendler;
        bool useCamera;
        std::string path;
    public:
        App(std::unique_ptr<ARecogniser> rec, std::unique_ptr<ASendler> s, bool useCamera = false, std::string path = "../video/SkySun.mp4") : recogniser(std::move(rec)), sendler(std::move(s)), useCamera(useCamera), path(path) {}

        int run();
};