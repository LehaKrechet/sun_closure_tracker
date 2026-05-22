#include <iostream>
#include <App.h>
#include <Recognizer.h>
#include <Sendler.h>




int main() {
    auto receiver = std::make_unique<FooCmdReceiver>();
    auto engine = std::make_unique<FooEngine>();
    auto recognizer = std::make_unique<FooRecognizer>();
    auto sendler = std::make_unique<FooSendler>();
    App app(std::move(engine), std::move(receiver), std::move(recognizer), std::move(sendler));
    app.run();
}