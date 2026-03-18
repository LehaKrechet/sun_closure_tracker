// #include <iostream>
// #include "Engine.h"
// #include "Receiver.h"

// int main(){
//     FooCmdReceiver reseiver;
//     FooEngine engine;
//     std::string cmd  = reseiver.receiver();
//     if (cmd  == "start"){
//         engine.start();
//     }else{
//         engine.stop();
//     }
// }


#include <iostream>
#include <App.h>

int main(){
    auto receiver = std::make_unique<FooCmdReceiver>();
    auto engine = std::make_unique<FooEngine>();
    App app(std::move(engine), std::move(receiver));
    app.run();

}
//Пример с просмотром камеры
// #include <opencv2/opencv.hpp>
// #include <iostream>

// int main() {

//     cv::VideoCapture cap(0);
    
//     if (!cap.isOpened()) {
//         std::cerr << "Ошибка: Не удалось открыть веб-камеру!" << std::endl;
//         return -1;
//     }
    
//     std::cout << "Веб-камера успешно открыта!" << std::endl;
//     std::cout << "Нажмите 'ESC' для выхода" << std::endl;
    
//     cv::Mat frame;
    
//     while (true) {
//         cap >> frame;
        
//         if (frame.empty()) {
//             std::cerr << "Ошибка: Не удалось получить кадр с камеры!" << std::endl;
//             break;
//         }
        
//         cv::imshow("Webcam Video", frame);
        
//         if (cv::waitKey(1) == 27) {
//             std::cout << "Выход из программы..." << std::endl;
//             break;
//         }
//     }
    
//     cap.release();
//     cv::destroyAllWindows();
    
//     return 0;
// }