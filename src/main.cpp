#include <iostream>
#include <App.h>
#include <Recognizer.h>
#include <Sendler.h>




int main(int argc, char* argv[]) {
    std::string ip = "0.0.0.0";
    std::string port = "8080";
    std::string path = "../video/SkySun.mp4";
    bool useCamera = false;

    for (int i  = 1; i < argc; i++) {

        std::string arg = argv[i];

        if (arg == "-ip") {

            ip = argv[++i];

        }else if (arg == "-p"){
            port = argv[++i];

        }else if (arg == "-c"){
            useCamera = true;
            
        }else if (arg == "-v"){
            path = argv[++i];
        }
    }

    auto recognizer = std::make_unique<FooRecognizer>();
    auto sendler = std::make_unique<FooSendler>(ip, port);
    App app(std::move(recognizer), std::move(sendler), useCamera, path);
    app.run();
}