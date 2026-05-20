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

        // 2. Маска облаков с адаптивной бинаризацией (пункт 2.4)
        cv::Mat grayV;
        cv::extractChannel(hsv, grayV, 2);               // канал яркости V
        cv::Mat mask;
        // Адаптивный порог: блок 75, константа 10, THRESH_BINARY_INV делает яркие области белыми
        cv::adaptiveThreshold(grayV, mask, 255,
                              cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                              cv::THRESH_BINARY_INV, 75, 10);

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
        // Первичное определение солнца (ярчайший бокс)
        sun_box = brightestBox;

        // 6. Трекинг облаков
        // Вычисляем dt (в секундах) между вызовами
        double currentTime = static_cast<double>(cv::getTickCount()) / cv::getTickFrequency();
        double dt = 1.0; // значение по умолчанию
        if (lastTime > 0.0) {
            dt = currentTime - lastTime;
        }
        lastTime = currentTime;

        // Запоминаем количество треков до добавления новых
        const size_t oldTrackCount = tracks.size();

        // 6.1 Предсказание новых положений всех существующих треков
        for (auto& track : tracks) {
            if (track.isInitialized) {
                track.kf.transitionMatrix.at<float>(0,2) = dt;
                track.kf.transitionMatrix.at<float>(1,3) = dt;
                cv::Mat predicted = track.kf.predict();
                float x = predicted.at<float>(0);
                float y = predicted.at<float>(1);
                float w = predicted.at<float>(4);
                float h = predicted.at<float>(5);
                track.predictedBox = cv::Rect(cvRound(x - w/2), cvRound(y - h/2), cvRound(w), cvRound(h));
            }
        }

        // 6.2 Ассоциация предсказанных треков с новыми детекциями
        std::vector<bool> detectionMatched(cloud_boxes.size(), false);
        std::vector<bool> trackMatched(oldTrackCount, false);
        std::vector<int> matchTrackIdx(cloud_boxes.size(), -1);

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
            kf.statePost = (cv::Mat_<float>(6,1) << initX, initY, 0, 0,
                            static_cast<float>(det.width), static_cast<float>(det.height));

            newTrack.kf = kf;
            newTrack.predictedBox = det;
            newTrack.updatedBox = det;
            newTrack.isInitialized = true;
            tracks.push_back(newTrack);
        }

        // 6.5 Увеличиваем счётчик пропусков для старых не сопоставленных треков
        for (size_t t = 0; t < oldTrackCount; ++t) {
            if (!trackMatched[t] && tracks[t].isInitialized) {
                tracks[t].missedFrames++;
            }
        }

        // 6.6 Удаление треков, не получавших измерений слишком долго
        const int maxMissedFrames = 5;
        tracks.erase(std::remove_if(tracks.begin(), tracks.end(),
            [&](const TrackedCloud& t) {
                return t.missedFrames >= maxMissedFrames || !t.isInitialized;
            }), tracks.end());

        // ... весь предыдущий код без изменений до пункта 7 ...

        // 7. Трекинг солнца (пункт 2.5)
        const double sunBrightnessThreshold = 240.0;   // жёсткий порог: только яркое солнце
        const double areaTolerance = 0.5;               // допустимое отклонение площади от эталона (50%)
        bool measurementValid = false;

        // Проверяем, что измерение (brightestBox) похоже на солнце
        if (!sun_box.empty() && maxBrightness > sunBrightnessThreshold) {
            if (sunTrack.isInitialized && sunTrack.referenceArea > 0) {
                double currentArea = sun_box.area();
                double ratio = currentArea / sunTrack.referenceArea;
                if (ratio >= (1.0 - areaTolerance) && ratio <= (1.0 + areaTolerance)) {
                    // Размер близок к эталонному
                    measurementValid = true;
                }
            } else {
                // Трекер ещё не инициализирован – первое измерение всегда валидно
                measurementValid = true;
            }
        }

        if (!sunTrack.isInitialized) {
            // 7.1 Инициализация трека солнца, если есть валидное измерение
            if (measurementValid) {
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

                float initX = sun_box.x + sun_box.width/2.0f;
                float initY = sun_box.y + sun_box.height/2.0f;
                kf.statePost = (cv::Mat_<float>(6,1) << initX, initY, 0, 0,
                                static_cast<float>(sun_box.width), static_cast<float>(sun_box.height));

                sunTrack.kf = kf;
                sunTrack.predictedBox = sun_box;
                sunTrack.updatedBox = sun_box;
                sunTrack.isInitialized = true;
                sunTrack.missedFrames = 0;
                sunTrack.referenceArea = sun_box.area();   // запомнили эталонную площадь
            }
        } else {
            // 7.2 Предсказание положения солнца
            sunTrack.kf.transitionMatrix.at<float>(0,2) = dt;
            sunTrack.kf.transitionMatrix.at<float>(1,3) = dt;
            cv::Mat predicted = sunTrack.kf.predict();
            float px = predicted.at<float>(0);
            float py = predicted.at<float>(1);
            float pw = predicted.at<float>(4);
            float ph = predicted.at<float>(5);
            sunTrack.predictedBox = cv::Rect(cvRound(px - pw/2), cvRound(py - ph/2), cvRound(pw), cvRound(ph));

            if (measurementValid) {
                // Дополнительная проверка расстояния до предсказания (максимум 50 пикселей)
                cv::Point predCenter(cvRound(px), cvRound(py));
                cv::Point measCenter(sun_box.x + sun_box.width/2, sun_box.y + sun_box.height/2);
                double dist = cv::norm(predCenter - measCenter);
                if (dist < 50.0) {
                    // 7.3 Обновление трека солнца по измерению
                    float mx = sun_box.x + sun_box.width/2.0f;
                    float my = sun_box.y + sun_box.height/2.0f;
                    float mw = static_cast<float>(sun_box.width);
                    float mh = static_cast<float>(sun_box.height);
                    cv::Mat measurement = (cv::Mat_<float>(4,1) << mx, my, mw, mh);
                    sunTrack.kf.correct(measurement);
                    sunTrack.missedFrames = 0;
                } else {
                    sunTrack.missedFrames++;
                }
            } else {
                sunTrack.missedFrames++;
            }

            // 7.4 Извлечение уточнённого/предсказанного состояния
            cv::Mat state = sunTrack.kf.statePost;
            float x = state.at<float>(0);
            float y = state.at<float>(1);
            float w = state.at<float>(4);
            float h = state.at<float>(5);
            sunTrack.updatedBox = cv::Rect(cvRound(x - w/2), cvRound(y - h/2), cvRound(w), cvRound(h));

            // Используем отслеженное положение солнца вместо сырого brightestBox
            sun_box = sunTrack.updatedBox;
        }

        // cv::imshow("Clouds", image);
        // cv::waitKey(0);
    }
}