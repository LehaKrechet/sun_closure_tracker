#include <iostream>
#include <App.h>
#include <Recognizer.h>




int main() {
    auto receiver = std::make_unique<FooCmdReceiver>();
    auto engine = std::make_unique<FooEngine>();
    auto recognizer = std::make_unique<FooCloudRecognizer>();
    App app(std::move(engine), std::move(receiver), std::move(recognizer));
    app.run();
}