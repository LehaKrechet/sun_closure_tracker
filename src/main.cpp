#include <iostream>
#include "Engine.h"
#include "Receiver.h"

int main(){
    FooCmdReceiver reseiver;
    FooEngine engine;
    std::string cmd  = reseiver.receiver();
    if (cmd  == "start"){
        engine.start();
    }else{
        engine.stop();
    }
}