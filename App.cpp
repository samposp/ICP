
// C++ 
#include <iostream>
#include <chrono>
#include <stack>
#include <random>
#include <numeric>
#include <thread>
#include <mutex>

// OpenCV 
#include <opencv2\opencv.hpp>

class App {
public:
    App();

    bool init(void);
    int run(void);

    void keepOpen();
    cv::Mat readImage(std::string filepath);
    void drawCrossNormalized(cv::Mat& img, cv::Point2f center_relative, int size);
    cv::Point2f getCentroidNormalized(cv::Mat frame, bool binaryImage);
    cv::Point2f centroidNonzero(cv::Mat& scene, cv::Scalar& lower_threshold, cv::Scalar& upper_threshold);
    cv::Point2f findFace(cv::Mat& frame);
    void captureAndFindFace(cv::Mat& frame, cv::Point2f& faceCenter);

    ~App();
private:
    cv::VideoCapture capture;
    cv::CascadeClassifier faceCascade = cv::CascadeClassifier("resources/haarcascade_frontalface_default.xml");
    std::mutex findFaceMutex;
    std::thread findFaceThread;
    std::atomic<bool> cameraRunning = false;
    std::atomic<bool> appClosed = false;
};

App::App()
{
    std::cout << "Constructed\n";
}

//============================== INIT =========================================

bool App::init()
{
    try {
        
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
    catch (std::exception const& e) {
        std::cerr << "Init failed : " << e.what() << std::endl;
        throw;
    }
    std::cout << "Initialized...\n";
    return true;
}

//================================== RUN ===================================

int App::run(void)
{
    try {
         //std::scoped_lock 
         //std::unique_lock

        cv::Mat frame;
        cv::Mat scene_cross;
        cv::Point2f faceCenter;
        cv::Point2f cameraFaceCenter;

        findFaceThread = std::thread(&App::captureAndFindFace, this, std::ref(frame), std::ref(cameraFaceCenter));

        
        do {
            auto start = std::chrono::steady_clock::now();

            if (!cameraRunning) {
                std::cerr << "Camera Stopped\n";
                findFaceThread.join();
                return EXIT_FAILURE;
            }

            {
                std::scoped_lock lk(findFaceMutex);
                frame.copyTo(scene_cross);
                faceCenter.x = cameraFaceCenter.x;
                faceCenter.y = cameraFaceCenter.y;
            }

            //display result
            if (!scene_cross.empty()) {
                drawCrossNormalized(scene_cross, faceCenter, 30);
                cv::imshow("scene", scene_cross);
            }

            auto end = std::chrono::steady_clock::now();

            std::chrono::duration<double> elapsed_seconds = end - start;
            std::cout << "elapsed time: " << elapsed_seconds.count() << "sec" << std::endl;


        } while (cv::pollKey() != 27); //message loop untill ESC
        appClosed = true;
        findFaceThread.join();
        return EXIT_SUCCESS;
    }
    catch (std::exception const& e) {
        std::cerr << "App failed : " << e.what() << std::endl;
        appClosed = true;
        findFaceThread.join();
        return EXIT_FAILURE;
    }
}

//============================ DESTRUCTOR =================================
App::~App()
{
    cv::destroyAllWindows();
    std::cout << "Bye...\n";
}

App app;

//========================== MAIN =========================================

int main()
{
    if (app.init())
        return app.run();
}

// ====================== HELPER FUNCTIONS =================================

void App::captureAndFindFace(cv::Mat& frame, cv::Point2f& faceCenter) {

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
                std::scoped_lock lk(findFaceMutex);
                cameraFrame.copyTo(frame);
                faceCenter.x = cameraFaceCenter.x;
                faceCenter.y = cameraFaceCenter.y;
            }
        }
    }

}

cv::Mat App::readImage(std::string filepath) {
    cv::Mat frame = cv::imread(filepath);  //can be JPG,PNG,GIF,TIFF,...

    if (frame.empty())
        throw std::exception("Empty file? Wrong path?");
    return frame;
}

void App::drawCrossNormalized(cv::Mat& img, cv::Point2f center_normalized, int size)
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

cv::Point2f App::getCentroidNormalized(cv::Mat frame, bool binaryImage = false) {

    cv::Moments m = cv::moments(frame, binaryImage);
    cv::Point2f centroid = cv::Point2f(m.m10 / m.m00, m.m01 / m.m00);
    centroid.x /= frame.cols;
    centroid.y /= frame.rows;
    return centroid;
}

cv::Point2f App::centroidNonzero(cv::Mat& scene, cv::Scalar& lower_threshold, cv::Scalar& upper_threshold)
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

cv::Point2f App::findFace(cv::Mat& frame)
{
    cv::Point2f center(0.0f, 0.0f);

    cv::Mat scene_grey;
    cv::cvtColor(frame, scene_grey, cv::COLOR_BGR2GRAY);

    std::vector<cv::Rect> faces;
    faceCascade.detectMultiScale(scene_grey, faces);

    if (faces.size() > 0)
    {
        // compute "center" as normalized coordinates of the face  
        center.x = (float)(faces[0].x + (faces[0].width/2)) / (float)frame.cols;
        center.y = (float)(faces[0].y + (faces[0].height/2)) / (float)frame.rows;
    }

    return center;
}
