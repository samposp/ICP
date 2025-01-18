
#include "App.h"

void App::captureAndFindFace(cv::Mat& frame, cv::Point2f& faceCenter) {

    {
        cv::Mat cameraFrame, resizedFrame;
        //cv::Point2f cameraFaceCenter;

        while (true) {
            capture.read(cameraFrame);
            if (cameraFrame.empty())
            {
                cameraRunning = false;
                return;
            }
            if (glfwWindowShouldClose(window))
                return;

            cv::resize(cameraFrame, resizedFrame, cv::Size(512, 512), cv::INTER_LINEAR);

            {
                std::scoped_lock lk(mutex);
                resizedFrame.copyTo(frame);
                //faceCenter.x = cameraFaceCenter.x;
                //faceCenter.y = cameraFaceCenter.y;
            }
        }
    }
}


void App::findFace(cv::Mat& frame, cv::Point2f outCenter)
{
    cv::Point2f center(0.0f, 0.0f);
    cv::Mat scene_grey;
    while (!glfwWindowShouldClose(window)) {
        std::vector<cv::Rect> faces;
        if (!frame.empty()) {
            {
                std::scoped_lock lk(mutex);
                cv::cvtColor(frame, scene_grey, cv::COLOR_BGR2GRAY);
            }

            faceCascade.detectMultiScale(scene_grey, faces);
        }


        if (faces.size() > 0)
        {
            stopApp = false;
            // compute "center" as normalized coordinates of the face  
            center.x = (float)(faces[0].x + (faces[0].width / 2)) / (float)frame.cols;
            center.y = (float)(faces[0].y + (faces[0].height / 2)) / (float)frame.rows;
        }
        else {
            stopApp = true;
        }
        
        cv::Mat scene_grey;
        {
            std::scoped_lock lk(mutex);
            outCenter.x = center.x;
            outCenter.y = center.y;
        }
    }
}
