#include <Recognizer.h>
#include <opencv2/opencv.hpp>

void FooCloudRecognizer::recognize(cv::Mat image){
    cloud_boxes.clear();
    sun_box = cv::Rect();
    if (image.empty()) {
        std::cerr << "Ошибка: Не удалось получить фото!" << std::endl;
    }else{
        // 1. Перевод в HSV
        cv::Mat hsv;
        cv::cvtColor(image, hsv, cv::COLOR_BGR2HSV);

        // 2. Маска облаков (светлые области)
        cv::Mat mask;
        cv::inRange(hsv, cv::Scalar(0, 0, 180), cv::Scalar(180, 60, 255), mask);

        // 3. Удаление шума
        cv::GaussianBlur(mask, mask, cv::Size(5,5), 0);
        cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, cv::Mat(), cv::Point(-1,-1), 2);

        // 4. Поиск контуров
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        // 5. Извлечение канала яркости
        double maxBrightness = -1.0;
        cv::Rect brightestBox;
        cv::Mat channelV;
        cv::extractChannel(hsv, channelV, 2);

        for (const std::vector<cv::Point>& cnt : contours) {
            double area = cv::contourArea(cnt);
            if (area < 1000) continue;

            // Bounding box
            cv::Rect box = cv::boundingRect(cnt);

            // Центр
            // int cx = box.x + box.width / 2;
            // int cy = box.y + box.height / 2;

            cloud_boxes.push_back(box);

            // std::cout << "Cloud:" << std::endl;
            // std::cout << "  x=" << box.x << " y=" << box.y << std::endl;
            // std::cout << "  w=" << box.width << " h=" << box.height << std::endl;
            // std::cout << "  center=(" << cx << "," << cy << ")" << std::endl;

            // Визуализация
            // cv::rectangle(image, box, cv::Scalar(0,255,0), 2);
            // cv::circle(image, cv::Point(cx, cy), 5, cv::Scalar(0,0,255), -1);
            
            // Вычисление средней яркости области
            cv::Rect safeBox = box & cv::Rect(0, 0, channelV.cols, channelV.rows);
            if (safeBox.area() <= 0) continue;

            cv::Mat roi = channelV(safeBox);
            cv::Scalar meanVal = cv::mean(roi);
            double brightness = meanVal[0];

            // Обновление самого яркого
            if (brightness > maxBrightness) {
                maxBrightness = brightness;
                brightestBox = box;
            }
        }
        sun_box = brightestBox;
        
        // cv::imshow("Clouds", image);
        // cv::waitKey(0);
    }
}

