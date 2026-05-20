#include <opencv2/opencv.hpp>
#include <vector>
#pragma once

class ARecogniser{
    protected:
        std::vector<cv::Rect> cloud_boxes;
        cv::Rect sun_box;
    public:
        virtual void recognize(cv::Mat image) = 0;
        virtual std::vector<cv::Rect> getCloudBoxes() const { return cloud_boxes; }
        virtual cv::Rect getSunBox() const { return sun_box; }        
};

class FooSunRecognizer : public ARecogniser{
    private:
        float position;
        float speed;
    public:
        void recognize();
};

class FooCloudRecognizer : public ARecogniser{
    private:
        // Структура для хранения отслеживаемого облака с фильтром Калмана
        struct TrackedCloud {
            int id;
            cv::KalmanFilter kf;
            cv::Rect predictedBox;   // предсказанный bounding box на текущий кадр
            cv::Rect updatedBox;     // скорректированный бокс после обновления
            int age;                 // сколько кадров существует трек
            int missedFrames;        // кадров без сопоставления
            bool isInitialized;
        };
        std::vector<TrackedCloud> tracks;
        int nextTrackId = 0;
        double lastTime = -1.0;      // время последнего вызова (для вычисления dt)
        float speed;
    public:
        void recognize(cv::Mat image);
};