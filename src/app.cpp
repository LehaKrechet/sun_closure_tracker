#include <App.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <Recognizer.h>
#include <Sendler.h>

int App::run(){
    
    std::string response = sendler -> send("stop");
    bool engine_state = 0;

    bool useCamera = false;
    cv::VideoCapture cap(useCamera ? 0 : "d:/SkySun.mp4");  
    
    if (!cap.isOpened()) {
        std::cerr << "Ошибка: Не удалось открыть веб-камеру!" << std::endl;
        return -1;
    }
    
    std::cout << "Веб-камера успешно открыта!" << std::endl;
    std::cout << "Нажмите 'ESC' для выхода" << std::endl;
    
    cv::Mat frame;
    cv::Mat image;
    double lastSaveTime = cv::getTickCount() / cv::getTickFrequency();
    const double saveInterval = 0.2; // 1 секунда
    
    while (cap.read(frame)) {  //   true /  cap.read(frame)
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
                if (!image.empty()) {
                    
                recogniser->recognize(image);

                bool hasCoverage = recogniser->isSunCoveragePredicted();
                bool hasCovered = recogniser->isSunCovered();      
                if ((hasCoverage||hasCovered) && !engine_state) {
                    // Если солнце закрыто(оется) и двигатель выключен -> включаем
                    response = sendler->send("start");
                    engine_state = true;
                    std::cout << "START: " << response << std::endl;
                } else if (!hasCovered && !hasCoverage && engine_state) {
                    // солнце открыто и не будет закрыто и двигатель включен -> выключаем
                    response = sendler->send("stop");
                    engine_state = false;
                    std::cout << "STOP: " << response << std::endl;
                }                       
                
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