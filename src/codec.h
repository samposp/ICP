#pragma once

#include <opencv2\opencv.hpp>

double getPSNR(const cv::Mat& I1, const cv::Mat& I2);
cv::Mat lossy_bw_limit(cv::Mat& input_img, double psnr);
void lossyEncodeAsync(cv::Mat& frame, cv::Mat& encodeFrame, bool& appClosed, std::mutex& mutex, float& compressionQuality);