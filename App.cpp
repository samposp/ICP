
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

    std::vector<uchar> lossy_bw_limit(cv::Mat& input_img, size_t size_limit);
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
    cv::Mat frame;
    std::vector<uchar> bytes;
    float target_coefficient = 0.5f; // used as size-ratio, or quality-coefficient
    try {
        while (capture.isOpened())
        {
            capture >> frame;

            if (frame.empty()) {
                std::cerr << "device closed (or video at the end)" << '\n';
                capture.release();
                break;
            }

            // encode image with bandwidth limit
            auto size_uncompressed = frame.elemSize() * frame.total();
            auto size_compressed_limit = size_uncompressed * target_coefficient;

            //
            // Encode single image with limitation by bandwidth
            //
            auto start = std::chrono::steady_clock::now();
            bytes = lossy_bw_limit(frame, size_compressed_limit); // returns JPG compressed stream for single image
            auto end = std::chrono::steady_clock::now();
            std::chrono::duration<double> compressionTime = end - start;

            //
            // TASK 1: Replace function lossy_bw_limit() - limitation by bandwith - with limitation by quality.
            //         Implement the function:
            // bytes = lossy_quality_limit(frame, target_coefficient);
            // 
            //         Use PSNR (Peak Signal to Noise Ratio)
            //         or  SSIM (Structural Similarity) 
            // (from https://docs.opencv.org/2.4/doc/tutorials/highgui/video-input-psnr-ssim/video-input-psnr-ssim.html#image-similarity-psnr-and-ssim ) 
            //         to estimate quality of the compressed image.
            //


            // display compression ratio
            start = std::chrono::steady_clock::now();
            auto size_compreessed = bytes.size();
            end = std::chrono::steady_clock::now();
            std::chrono::duration<double> decompressionTime = end - start;
            std::cout << "Size: uncompressed = " << size_uncompressed << ", compressed = " << size_compreessed << ", = " << size_compreessed / (size_uncompressed / 100.0) << " % \n";
            std::cout << "Time to compress = " << compressionTime.count() << " time to decopmress = " << decompressionTime.count() << std::endl;

            //
            // decode compressed data
            //  
            cv::Mat decoded_frame = cv::imdecode(bytes, cv::IMREAD_ANYCOLOR);

            cv::namedWindow("original");
            cv::imshow("original", frame);

            cv::namedWindow("decoded");
            cv::imshow("decoded", decoded_frame);

            // key handling
            int c = cv::pollKey();
            switch (c) {
            case 27:
                return EXIT_SUCCESS;
                break;
            case 'q':
                target_coefficient *= 2;
                break;
            case 'a':
                target_coefficient /= 2;
                break;
            default:
                break;
            }

            target_coefficient = std::clamp(target_coefficient, 0.01f, 1.0f);
            std::cout << "Target coeff: " << target_coefficient * 100.0f << " %\n";
        }
    }
    catch (std::exception const& e) {
        std::cerr << "App failed : " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Finished OK...\n";
    return EXIT_SUCCESS;
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

std::vector<uchar> App::lossy_bw_limit(cv::Mat& input_img, size_t size_limit)
{
    std::string suff(".jpg"); // target format
    if (!cv::haveImageWriter(suff))
        throw std::runtime_error("Can not compress to format:" + suff);

    std::vector<uchar> bytes;
    std::vector<int> compression_params;

    // prepare parameters for JPEG compressor
    // we use only quality, but other parameters are available (progressive, optimization...)
    std::vector<int> compression_params_template;
    compression_params_template.push_back(cv::IMWRITE_JPEG_QUALITY);

    std::cout << '[';

    //try step-by-step to decrease quality by 5%, until it fits into limit
    for (auto i = 100; i > 0; i -= 5) {
        compression_params = compression_params_template; // reset parameters
        compression_params.push_back(i);                  // set desired quality
        std::cout << i << ',';

        // try to encode
        cv::imencode(suff, input_img, bytes, compression_params);

        // check the size limit
        if (bytes.size() <= size_limit)
            break; // ok, done 
    }
    std::cout << "]\n";

    return bytes;
}

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
