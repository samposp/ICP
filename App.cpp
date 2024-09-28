
// C++ 
#include <iostream>
#include <chrono>
#include <stack>
#include <random>
#include <numeric>

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

    ~App();
private:
    cv::VideoCapture capture;  // global variable, move to app class, protected

};

App::App()
{
    std::cout << "Constructed...\n";
}

//============================== INIT =========================================

bool App::init()
{
    try {
        
        //open first available camera
        capture = cv::VideoCapture(cv::CAP_DSHOW);

        //open video file
        //capture = cv::VideoCapture("Resource/video.mkv");

        if (!capture.isOpened())
        {
            std::cerr << "no source?" << std::endl;
            exit(EXIT_FAILURE);
        }
        else
        {
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
        cv::Mat scene, frame;
        
        scene = readImage("Resource/hsv-map.png");

        cv::namedWindow("scene", 0);
        
        int hm = 128, sm = 128, vm = 128, hx = 255, sx = 255, vx = 255;
        cv::createTrackbar("HMin", "scene", &hm, 255);
        cv::createTrackbar("SMin", "scene", &sm, 255);
        cv::createTrackbar("VMin", "scene", &vm, 255);
        cv::createTrackbar("HMax", "scene", &hx, 255);
        cv::createTrackbar("SMax", "scene", &sx, 255);
        cv::createTrackbar("VMax", "scene", &vx, 255);

        cv::Scalar lower_threshold;
        cv::Scalar upper_threshold;
        while (1)
        {
            capture.read(frame);
            if (frame.empty())
            {
                std::cerr << "Cam disconnected? End of file?\n";
                break;
            }

            // Get current positions of trackbars
            // HSV ranges between (0-255, 0-255, 0-255).
            lower_threshold = cv::Scalar(
                cv::getTrackbarPos("HMin", "scene"),
                cv::getTrackbarPos("SMin", "scene"),
                cv::getTrackbarPos("VMin", "scene"));

            upper_threshold = cv::Scalar(
                cv::getTrackbarPos("HMax", "scene"),
                cv::getTrackbarPos("SMax", "scene"),
                cv::getTrackbarPos("VMax", "scene"));            

            cv::Mat scene_hsv, scene_threshold;

            cv::cvtColor(frame, scene_hsv, cv::COLOR_BGR2HSV);

            cv::Scalar lower_threshold = cv::Scalar(hm, sm, vm);
            cv::Scalar upper_threshold = cv::Scalar(hx, sx, vx);
            cv::inRange(scene_hsv, lower_threshold, upper_threshold, scene_threshold);

            cv::Point2f centroid = getCentroidNormalized(scene_threshold, true);
            drawCrossNormalized(frame, centroid, 30);

            cv::imshow("grabbed", frame);
            cv::imshow("threshold", scene_threshold);

            if (cv::waitKey(1) == 27)
                break;
        }

        return EXIT_SUCCESS;
    }
    catch (std::exception const& e) {
        std::cerr << "App failed : " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Finished OK...\n";
    return EXIT_SUCCESS;
}

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

cv::Mat App::readImage(std::string filepath) {
    cv::Mat frame = cv::imread(filepath);  //can be JPG,PNG,GIF,TIFF,...

    if (frame.empty())
        throw std::exception("Empty file? Wrong path?");
    return frame;
}

void App::keepOpen() {
    // keep application open until ESC is pressed
    while (true)
    {
        int key = cv::pollKey(); // poll OS events (key press, mouse move, ...)
        if (key == 27) // test for ESC key
            break;
    }
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