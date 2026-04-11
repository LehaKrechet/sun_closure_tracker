#include <App.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <Recognizer.h>
#include <Sendler.h>

int App::run(){
    // std::string cmd  = receiver -> receiver();
    // if (cmd  == "start"){
    //     engine->start();
    // }else{
    //     engine->stop();
    // }
    std::string response = sendler -> send("Start");
    cv::VideoCapture cap(0);
    
    if (!cap.isOpened()) {
        std::cerr << "Ошибка: Не удалось открыть веб-камеру!" << std::endl;
        return -1;
    }
    
    std::cout << "Веб-камера успешно открыта!" << std::endl;
    std::cout << "Нажмите 'ESC' для выхода" << std::endl;
    
    cv::Mat frame;
    cv::Mat image;
    double lastSaveTime = cv::getTickCount() / cv::getTickFrequency();
    const double saveInterval = 1.0; // 1 секунда
    
    while (true) {
        cap >> frame;
        if (frame.empty()) {
            std::cerr << "Ошибка: Не удалось получить кадр с камеры!" << std::endl;
            break;
        }
        
        cv::imshow("Webcam Video", frame);

        double currentTime = cv::getTickCount() / cv::getTickFrequency();
        
        if (currentTime-lastSaveTime>=saveInterval){
            if(cv::imwrite("../image/foto.png", frame)){
                std::cout << "Cadr save" << std::endl;
                lastSaveTime = currentTime;

                image = cv::imread("../image/foto.png");
                // image = cv::imread("../image/Sky1.png");
                if (!image.empty()) {
                    
                recogniser->recognize(image);
                for (const cv::Rect& box : recogniser->getCloudBoxes()) {
                    // Визуализация
                    cv::rectangle(image, box, cv::Scalar(0,255,0), 2);
                }
                cv::rectangle(image, recogniser->getSunBox() , cv::Scalar(0,0,255), 3);
                cv::imshow("Clouds", image);

                }
            }
        }  
        
        if (cv::waitKey(1) == 27) {
            std::cout << "Выход из программы..." << std::endl;
            break;
        }
    }
    
    cap.release();
    cv::destroyAllWindows();
    return 0;
}