#include <iostream>
#include <App.h>
#include <Recognizer.h>
#include <Sendler.h>




int main() {
    auto recognizer = std::make_unique<FooRecognizer>();
    auto sendler = std::make_unique<FooSendler>();
    App app(std::move(recognizer), std::move(sendler));
    app.run();
}