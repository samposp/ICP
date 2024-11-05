#pragma once

#include <opencv2/opencv.hpp>

void captureAsync(cv::Mat& frame, bool& appClosed, cv::VideoCapture& capture, bool& cameraRunning, std::mutex& mutex);
void drawCrossNormalized(cv::Mat& img, cv::Point2f center_normalized, int size);
cv::Point2f getCentroidNormalized(cv::Mat frame, bool binaryImage);
cv::Point2f centroidNonzero(cv::Mat& scene, cv::Scalar& lower_threshold, cv::Scalar& upper_threshold);
