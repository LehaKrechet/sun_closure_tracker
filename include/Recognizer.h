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
        virtual bool isSunCoveragePredicted() const { return false; }
        virtual bool isSunCovered() const { return false; }
        virtual double getTimeToCoverage() const { return -1.0; }
        virtual int getCoveringCloudId() const { return -1; }
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
            cv::Rect predictedBox;
            cv::Rect updatedBox;
            int age;
            int missedFrames;
            bool isInitialized;
        };
        std::vector<TrackedCloud> tracks;
        int nextTrackId = 0;
        double lastTime = -1.0;
        float speed;

        // Трекер солнца (пункт 2.5)
        struct TrackedSun {
            cv::KalmanFilter kf;
            cv::Rect predictedBox;
            cv::Rect updatedBox;
            bool isInitialized = false;
            int missedFrames = 0;
        };
        TrackedSun sunTrack;

        // Прогноз покрытия солнца (пункты 2.3 и 2.6)
        bool sunCoveragePredicted = false;
        bool sunCover = false;
        double timeToCoverage = -1.0;  // секунды до покрытия
        int coveringCloudId = -1;

    public:
        // Методы доступа к прогнозу
        bool isSunCoveragePredicted() const override;
        bool isSunCovered() const override;
        double getTimeToCoverage() const override;
        int getCoveringCloudId() const override;

        void recognize(cv::Mat image);
};