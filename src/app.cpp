#include <App.h>

void  App::run(){
    std::string cmd  = receiver -> receiver();
    if (cmd  == "start"){
        engine->start();
    }else{
        engine->stop();
    }
}