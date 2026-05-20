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

        // 6. Трекинг облаков
        // Вычисляем dt (в секундах) между вызовами
        double currentTime = static_cast<double>(cv::getTickCount()) / cv::getTickFrequency();
        double dt = 1.0; // значение по умолчанию
        if (lastTime > 0.0) {
            dt = currentTime - lastTime;
        }
        lastTime = currentTime;

        // Запоминаем количество треков до добавления новых, чтобы избежать выхода за границы trackMatched
        const size_t oldTrackCount = tracks.size();

        // 6.1 Предсказание новых положений всех существующих треков
        for (auto& track : tracks) {
            if (track.isInitialized) {
                // Обновляем модель перехода с текущим dt
                track.kf.transitionMatrix.at<float>(0,2) = dt; // x = x + vx*dt
                track.kf.transitionMatrix.at<float>(1,3) = dt; // y = y + vy*dt
                // Предсказание
                cv::Mat predicted = track.kf.predict();
                // Извлекаем предсказанное состояние: x, y, w, h
                float x = predicted.at<float>(0);
                float y = predicted.at<float>(1);
                float w = predicted.at<float>(4);
                float h = predicted.at<float>(5);
                track.predictedBox = cv::Rect(cvRound(x - w/2), cvRound(y - h/2), cvRound(w), cvRound(h));
            }
        }

        // 6.2 Ассоциация предсказанных треков с новыми детекциями (cloud_boxes)
        std::vector<bool> detectionMatched(cloud_boxes.size(), false);
        std::vector<bool> trackMatched(oldTrackCount, false); // размер по числу старых треков
        std::vector<int> matchTrackIdx(cloud_boxes.size(), -1); // для каждой детекции индекс трека

        // Простой алгоритм сопоставления на основе IoU с порогом
        const float iouThreshold = 0.3f;
        for (size_t d = 0; d < cloud_boxes.size(); ++d) {
            float bestIoU = iouThreshold;
            int bestTrackIdx = -1;
            for (size_t t = 0; t < oldTrackCount; ++t) {
                if (trackMatched[t]) continue;
                if (!tracks[t].isInitialized) continue;
                cv::Rect pred = tracks[t].predictedBox;
                cv::Rect det = cloud_boxes[d];
                cv::Rect inter = pred & det;
                if (inter.area() <= 0) continue;
                float iou = static_cast<float>(inter.area()) / (pred.area() + det.area() - inter.area());
                if (iou > bestIoU) {
                    bestIoU = iou;
                    bestTrackIdx = static_cast<int>(t);
                }
            }
            if (bestTrackIdx >= 0) {
                matchTrackIdx[d] = bestTrackIdx;
                trackMatched[bestTrackIdx] = true;
                detectionMatched[d] = true;
            }
        }

        // 6.3 Обновление сопоставленных треков
        for (size_t d = 0; d < cloud_boxes.size(); ++d) {
            if (!detectionMatched[d]) continue;
            int t = matchTrackIdx[d];
            cv::Rect det = cloud_boxes[d];
            float mx = det.x + det.width/2.0f;
            float my = det.y + det.height/2.0f;
            float mw = static_cast<float>(det.width);
            float mh = static_cast<float>(det.height);
            cv::Mat measurement = (cv::Mat_<float>(4,1) << mx, my, mw, mh);
            tracks[t].kf.correct(measurement);
            tracks[t].age++;
            tracks[t].missedFrames = 0;
            cv::Mat state = tracks[t].kf.statePost;
            float x = state.at<float>(0);
            float y = state.at<float>(1);
            float w = state.at<float>(4);
            float h = state.at<float>(5);
            tracks[t].updatedBox = cv::Rect(cvRound(x - w/2), cvRound(y - h/2), cvRound(w), cvRound(h));
        }

        // 6.4 Создание новых треков для несопоставленных детекций
        for (size_t d = 0; d < cloud_boxes.size(); ++d) {
            if (detectionMatched[d]) continue;
            cv::Rect det = cloud_boxes[d];
            TrackedCloud newTrack;
            newTrack.id = nextTrackId++;
            newTrack.age = 1;
            newTrack.missedFrames = 0;

            cv::KalmanFilter kf(6, 4, 0);
            kf.transitionMatrix = (cv::Mat_<float>(6,6) <<
                1, 0, dt, 0, 0, 0,
                0, 1, 0, dt, 0, 0,
                0, 0, 1,  0, 0, 0,
                0, 0, 0,  1, 0, 0,
                0, 0, 0,  0, 1, 0,
                0, 0, 0,  0, 0, 1);
            kf.measurementMatrix = (cv::Mat_<float>(4,6) <<
                1,0,0,0,0,0,
                0,1,0,0,0,0,
                0,0,0,0,1,0,
                0,0,0,0,0,1);
            cv::setIdentity(kf.processNoiseCov, cv::Scalar::all(1e-2));
            cv::setIdentity(kf.measurementNoiseCov, cv::Scalar::all(1e-1));
            cv::setIdentity(kf.errorCovPost, cv::Scalar::all(1));

            float initX = det.x + det.width/2.0f;
            float initY = det.y + det.height/2.0f;
            kf.statePost = (cv::Mat_<float>(6,1) << initX, initY, 0, 0, static_cast<float>(det.width), static_cast<float>(det.height));

            newTrack.kf = kf;
            newTrack.predictedBox = det;
            newTrack.updatedBox = det;
            newTrack.isInitialized = true;
            tracks.push_back(newTrack);
        }

        // 6.5 Увеличиваем счётчик пропусков для старых не сопоставленных треков (до удаления)
        for (size_t t = 0; t < oldTrackCount; ++t) {
            if (!trackMatched[t] && tracks[t].isInitialized) {
                tracks[t].missedFrames++;
            }
        }

        // 6.6 Удаление треков, не получавших измерений слишком долго (5 кадров)
        const int maxMissedFrames = 5;
        tracks.erase(std::remove_if(tracks.begin(), tracks.end(),
            [&](const TrackedCloud& t) {
                return t.missedFrames >= maxMissedFrames || !t.isInitialized;
            }), tracks.end());

        // cv::imshow("Clouds", image);
        // cv::waitKey(0);
    }
}