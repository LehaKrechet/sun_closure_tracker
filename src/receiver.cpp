#include "Receiver.h"

std::string FooCmdReceiver::receiver(){
    std::string cmd;
    std::cin >> cmd;
    return cmd;
}