
#include "imageProcessing.h"
#include <numeric>

void captureAsync(cv::Mat& frame, bool& appClosed, cv::VideoCapture& capture, bool& cameraRunning, std::mutex& mutex)
{
    cv::Mat cameraFrame;
    while (!appClosed) {
        capture.read(cameraFrame);
        if (cameraFrame.empty())
        {
            cameraRunning = false;
            return;
        }
        {
            std::scoped_lock lk(mutex);
            cameraFrame.copyTo(frame);
        }
    }
}

void drawCrossNormalized(cv::Mat& img, cv::Point2f center_normalized, int size)
{
    center_normalized.x = std::clamp(center_normalized.x, 0.0f, 1.0f);
    center_normalized.y = std::clamp(center_normalized.y, 0.0f, 1.0f);
    size = std::clamp(size, 1, std::min(img.cols, img.rows));

    cv::Point2f center_absolute(center_normalized.x * img.cols, center_normalized.y * img.rows);

    cv::Point2f p1(center_absolute.x - size / 2, center_absolute.y);
    cv::Point2f p2(center_absolute.x + size / 2, center_absolute.y);
    cv::Point2f p3(center_absolute.x, center_absolute.y - size / 2);
    cv::Point2f p4(center_absolute.x, center_absolute.y + size / 2);

    cv::line(img, p1, p2, CV_RGB(255, 0, 0), 3);
    cv::line(img, p3, p4, CV_RGB(255, 0, 0), 3);
}

cv::Point2f getCentroidNormalized(cv::Mat frame, bool binaryImage) {

    cv::Moments m = cv::moments(frame, binaryImage);
    cv::Point2f centroid = cv::Point2f(m.m10 / m.m00, m.m01 / m.m00);
    centroid.x /= frame.cols;
    centroid.y /= frame.rows;
    return centroid;
}

cv::Point2f centroidNonzero(cv::Mat& scene, cv::Scalar& lower_threshold, cv::Scalar& upper_threshold)
{
    cv::Mat scene_hsv;
    cv::cvtColor(scene, scene_hsv, cv::COLOR_BGR2HSV);

    cv::Mat scene_threshold;
    cv::inRange(scene_hsv, lower_threshold, upper_threshold, scene_threshold);

    cv::namedWindow("scene_threshold", 0);
    cv::imshow("scene_threshold", scene_threshold);

    std::vector<cv::Point> whitePixels;
    cv::findNonZero(scene_threshold, whitePixels);
    int whiteCnt = whitePixels.size();

    cv::Point whiteAccum = std::accumulate(whitePixels.begin(), whitePixels.end(), cv::Point(0.0, 0.0));

    cv::Point2f centroid_relative(0.0f, 0.0f);
    if (whiteCnt > 0)
    {
        cv::Point centroid = { whiteAccum.x / whiteCnt, whiteAccum.y / whiteCnt };
        centroid_relative = { static_cast<float>(centroid.x) / scene.cols, static_cast<float>(centroid.y) / scene.rows };
    }

    return centroid_relative;
}
