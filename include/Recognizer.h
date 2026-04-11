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
        float speed;
    public:
        void recognize(cv::Mat image);
};