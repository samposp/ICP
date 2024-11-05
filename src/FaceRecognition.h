#pragma once

#include <opencv2/opencv.hpp>

class FaceRecognition {
public:
	void init();
	cv::Point2f findFace(cv::Mat& frame);
	void captureAndFindFace(cv::Mat& frame, cv::Point2f& faceCenter);

private:
	cv::CascadeClassifier faceCascade = cv::CascadeClassifier("../resources/haarcascade_frontalface_default.xml");
	cv::VideoCapture capture;
	std::atomic<bool> cameraRunning = false;
	std::atomic<bool> appClosed = false;
	std::mutex mutex;
};