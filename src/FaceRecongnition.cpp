
#include "FaceRecognition.h"

void FaceRecognition::init() {
    //open first available camera
    capture = cv::VideoCapture(cv::CAP_DSHOW);

    if (!capture.isOpened())
    {
        std::cerr << "no source?" << std::endl;
        exit(EXIT_FAILURE);
    }
    else
    {
        cameraRunning = true;
        std::cout << "Source: " <<
            ": width=" << capture.get(cv::CAP_PROP_FRAME_WIDTH) <<
            ", height=" << capture.get(cv::CAP_PROP_FRAME_HEIGHT) << '\n';
    }
}

void FaceRecognition::captureAndFindFace(cv::Mat& frame, cv::Point2f& faceCenter) {

    {
        cv::Mat cameraFrame;
        cv::Point2f cameraFaceCenter;

        while (true) {
            capture.read(cameraFrame);
            if (cameraFrame.empty())
            {
                cameraRunning = false;
                return;
            }
            if (appClosed)
                return;

            cameraFaceCenter = findFace(cameraFrame);

            {
                std::scoped_lock lk(mutex);
                cameraFrame.copyTo(frame);
                faceCenter.x = cameraFaceCenter.x;
                faceCenter.y = cameraFaceCenter.y;
            }
        }
    }
}


cv::Point2f FaceRecognition::findFace(cv::Mat& frame)
{
    cv::Point2f center(0.0f, 0.0f);

    cv::Mat scene_grey;
    cv::cvtColor(frame, scene_grey, cv::COLOR_BGR2GRAY);

    std::vector<cv::Rect> faces;
    faceCascade.detectMultiScale(scene_grey, faces);

    if (faces.size() > 0)
    {
        // compute "center" as normalized coordinates of the face  
        center.x = (float)(faces[0].x + (faces[0].width / 2)) / (float)frame.cols;
        center.y = (float)(faces[0].y + (faces[0].height / 2)) / (float)frame.rows;
    }

    return center;
}
