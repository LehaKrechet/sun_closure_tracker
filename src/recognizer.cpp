#include <Recognizer.h>
#include <opencv2/opencv.hpp>

void FooRecognizer::recognize(cv::Mat image){
    cloud_boxes.clear();
    sun_box = cv::Rect();
    if (image.empty()) {
        std::cerr << "Ошибка: Не удалось получить фото!" << std::endl;
    }else{
        // Перевод в HSV
        cv::Mat hsv;
        cv::cvtColor(image, hsv, cv::COLOR_BGR2HSV);

        // Маска облаков (светлые области)
        cv::Mat mask;
        cv::inRange(hsv, cv::Scalar(0, 0, 180), cv::Scalar(180, 60, 255), mask);

        // Удаление шума
        cv::GaussianBlur(mask, mask, cv::Size(5,5), 0);
        cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, cv::Mat(), cv::Point(-1,-1), 2);

        // Поиск контуров
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        // Извлечение канала яркости
        double maxBrightness = -1.0;
        cv::Rect brightestBox;
        cv::Mat channelV;
        cv::extractChannel(hsv, channelV, 2);

        // Параметры центральной области для поиска солнца
        cv::Point imageCenter(image.cols / 2, image.rows / 2);
        double maxDistFromCenter = std::min(image.cols, image.rows) * 0.25; // 25% от размера кадра

        // Максимально допустимый размер контура-кандидата в солнце
        float maxSunSide = std::min(image.cols, image.rows) * 0.6f; // 60% от размера кадра

        for (const std::vector<cv::Point>& cnt : contours) {
            double area = cv::contourArea(cnt);
            if (area < 1000) continue;

            // Bounding box
            cv::Rect box = cv::boundingRect(cnt);

            cloud_boxes.push_back(box);

            // Проверка, находится ли контур в центральной области
            cv::Point cntCenter(box.x + box.width/2, box.y + box.height/2);
            double distToCenter = cv::norm(cntCenter - imageCenter);
            if (distToCenter > maxDistFromCenter) {
                // Этот контур не солнце
                continue;
            }

            // проверка на размер
            if (box.width > maxSunSide || box.height > maxSunSide) {
                continue;
            }

            // Вычисление средней яркости
            cv::Rect safeBox = box & cv::Rect(0, 0, channelV.cols, channelV.rows);
            if (safeBox.area() <= 0) continue;

            cv::Mat roi = channelV(safeBox);
            cv::Scalar meanVal = cv::mean(roi);
            double brightness = meanVal[0];

            // Обновление самого яркого среди центральных
            if (brightness > maxBrightness) {
                maxBrightness = brightness;
                brightestBox = box;
            }
        }
        sun_box = brightestBox;
        sunCover = sun_box.empty();  // true - солнце закрыто
        // Трекинг облаков
        // Вычисляем dt (в секундах) между вызовами
        double currentTime = static_cast<double>(cv::getTickCount()) / cv::getTickFrequency();
        double dt = 1.0;
        if (lastTime > 0.0) {
            dt = currentTime - lastTime;
        }
        lastTime = currentTime;

        // Запоминаем количество треков до добавления новых,
        const size_t oldTrackCount = tracks.size();

        // Предсказание новых положений всех существующих треков
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

        // Ассоциация предсказанных треков с новыми детекциями 
        std::vector<bool> detectionMatched(cloud_boxes.size(), false);
        std::vector<bool> trackMatched(oldTrackCount, false); // размер по числу старых треков
        std::vector<int> matchTrackIdx(cloud_boxes.size(), -1); // для каждой детекции индекс трека

        //сопоставления с порогом
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

        // Обновление сопоставленных треков
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

        // Создание новых треков для несопоставленных детекций
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

        // Увеличиваем счётчик пропусков для старых не сопоставленных треков
        for (size_t t = 0; t < oldTrackCount; ++t) {
            if (!trackMatched[t] && tracks[t].isInitialized) {
                tracks[t].missedFrames++;
            }
        }

        // Удаление треков, не получавших измерений слишком долго
        const int maxMissedFrames = 5;
        tracks.erase(std::remove_if(tracks.begin(), tracks.end(),
            [&](const TrackedCloud& t) {
                return t.missedFrames >= maxMissedFrames || !t.isInitialized;
            }), tracks.end());

        // Прогноз покрытия солнца 
        sunCoveragePredicted = false;
        timeToCoverage = -1.0;
        coveringCloudId = -1;
        if (!sun_box.empty()) {
            float sunX = sun_box.x + sun_box.width / 2.0f;
            float sunY = sun_box.y + sun_box.height / 2.0f;
            float sunRadius = std::max(sun_box.width, sun_box.height) / 2.0f;
            // bool sunLessCloud = false;
            const int minAgeForPrediction = 4; // минимальный возраст трека для прогноза
            double minTime = 3.1; //  60 c

            for (const auto& track : tracks) {
                // Пропускаем новые и потеряные треки
                if (!track.isInitialized || track.age < minAgeForPrediction)
                    continue;

                // Состояние облака
                cv::Mat cloudState = track.kf.statePost;
                float cloudX = cloudState.at<float>(0);
                float cloudY = cloudState.at<float>(1);
                float cloudVx = cloudState.at<float>(2);
                float cloudVy = cloudState.at<float>(3);
                float cloudRadius = std::max(track.updatedBox.width,
                                             track.updatedBox.height) / 2.0f;

                // Вектор от солнца к облаку
                float dx = cloudX - sunX;
                float dy = cloudY - sunY;

                // скорость облака
                float v2 = cloudVx * cloudVx + cloudVy * cloudVy;
                if (v2 < 1e-6f) continue; // нет движения  облака

                // Время до точки максимального сближения
                float t = -(dx * cloudVx + dy * cloudVy) / v2;
                if (t <= 0.0f || t > 3.0f) continue; // уже разошлись или слишком далеко

                // Минимальное расстояние
                float dx_t = dx + cloudVx * t;
                float dy_t = dy + cloudVy * t;
                float minDist = std::sqrt(dx_t * dx_t + dy_t * dy_t);

                float criticalDist = cloudRadius + sunRadius;
                if (minDist < criticalDist && t < minTime && (1.4*sun_box.width*sun_box.height<track.updatedBox.width*track.updatedBox.height)) {
                    minTime = t;
                    coveringCloudId = track.id;
                }
            }

            if (minTime <= 3.0) {
                sunCoveragePredicted = true;
                timeToCoverage = minTime;
            }
        }
    }
}

bool FooRecognizer::isSunCoveragePredicted() const {return sunCoveragePredicted;}
bool FooRecognizer::isSunCovered() const {return sunCover;}
double FooRecognizer::getTimeToCoverage() const {return timeToCoverage;}
int FooRecognizer::getCoveringCloudId() const {return coveringCloudId;}