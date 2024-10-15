
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

    void lossyEncodeAsync(cv::Mat& frame, cv::Mat& encodeFrame);
    void captureAsync(cv::Mat& frame);
    cv::Mat lossy_bw_limit(cv::Mat& input_img, double psnr);
    cv::Mat readImage(std::string filepath);
    void drawCrossNormalized(cv::Mat& img, cv::Point2f center_relative, int size);
    cv::Point2f getCentroidNormalized(cv::Mat frame, bool binaryImage);
    cv::Point2f centroidNonzero(cv::Mat& scene, cv::Scalar& lower_threshold, cv::Scalar& upper_threshold);
    cv::Point2f findFace(cv::Mat& frame);
    void captureAndFindFace(cv::Mat& frame, cv::Point2f& faceCenter);
    double getPSNR(const cv::Mat& I1, const cv::Mat& I2);

    ~App();
private:
    cv::VideoCapture capture;
    cv::CascadeClassifier faceCascade = cv::CascadeClassifier("resources/haarcascade_frontalface_default.xml");
    std::mutex mutex;
    std::atomic<bool> cameraRunning = false;
    std::atomic<bool> appClosed = false;
    std::atomic<int> compressionQuality = 50;
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
    cv::Mat frame, decoded_frame, transfer_frame, encode_transfer_frame;

    std::thread captureThread = std::thread(&App::captureAsync, this, std::ref(transfer_frame));
    std::thread encodeThread = std::thread(&App::lossyEncodeAsync, this, std::ref(transfer_frame), std::ref(encode_transfer_frame));

    try {
        while (!appClosed)
        {
            if (!cameraRunning)
            {
                appClosed = true;
                std::cerr << "Camera stopped\n";
                break;
            }
            {
                std::scoped_lock lock(mutex);
                encode_transfer_frame.copyTo(decoded_frame);
                transfer_frame.copyTo(frame);
            }


            if (!frame.empty())
            {
                cv::namedWindow("original");
                cv::imshow("original", frame);
            }

            if (!decoded_frame.empty())
            {
                cv::namedWindow("decoded");
                cv::imshow("decoded", decoded_frame);
            }


            // key handling
            int c = cv::pollKey();
            switch (c) {
            case 27:
                appClosed = true;
                std::cout << "Finished OK...\n";
                captureThread.join();
                encodeThread.join();
                return EXIT_SUCCESS;
                break;
            case 'q':
                compressionQuality = compressionQuality + 1;
                break;
            case 'a':
                compressionQuality = compressionQuality - 1;
                break;
            default:
                break;
            }

            int compression = compressionQuality;
            compressionQuality = std::clamp(compression, 0, 100);
            std::cout << "Compression quality: " << compressionQuality << "% \n";
        }
    }
    catch (std::exception const& e) {
        appClosed = true;
        std::cerr << "App failed : " << e.what() << std::endl;
        captureThread.join();
        encodeThread.join();
        return EXIT_FAILURE;
    }

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

double App::getPSNR(const cv::Mat& I1, const cv::Mat& I2)
{
    cv::Mat s1;
    absdiff(I1, I2, s1);       // |I1 - I2|
    s1.convertTo(s1, CV_32F);  // cannot make a square on 8 bits
    s1 = s1.mul(s1);           // |I1 - I2|^2

    cv::Scalar s = sum(s1);         // sum elements per channel

    double sse = s.val[0] + s.val[1] + s.val[2]; // sum channels

    if (sse <= 1e-10) // for small values return zero
        return 0;
    else
    {
        double  mse = sse / (double)(I1.channels() * I1.total());
        double psnr = 10.0 * log10((255 * 255) / mse);
        return psnr;
    }
}

cv::Mat App::lossy_bw_limit(cv::Mat& input_img, double psnr)
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
    cv::Mat decoded_frame;
    //try step-by-step to decrease quality by 5%, until it fits into limit
    for (auto i = 100; i > 0; i -= 5) {
        compression_params = compression_params_template; // reset parameters
        compression_params.push_back(i);                  // set desired quality

        // try to encode
        cv::imencode(suff, input_img, bytes, compression_params);
        decoded_frame = cv::imdecode(bytes, cv::IMREAD_ANYCOLOR);
        // check the size limit
        double actualPSNR = getPSNR(input_img, decoded_frame) / 100.0;
        std::cout << i << ':' << actualPSNR << ',';
        if (actualPSNR <= psnr)
            break; // ok, done 
    }
    std::cout << "]\n";

    return decoded_frame;
}

void App::lossyEncodeAsync(cv::Mat& frame, cv::Mat& encodeFrame)
{
    std::string suff(".jpg"); // target format
    if (!cv::haveImageWriter(suff))
        throw std::runtime_error("Can not compress to format:" + suff);

    cv::Mat input_img, encoded_frame;
    std::vector<uchar> bytes;
    std::vector<int> compression_params;

    // prepare parameters for JPEG compressor
    // we use only quality, but other parameters are available (progressive, optimization...)
    std::vector<int> compression_params_template;
    compression_params_template.push_back(cv::IMWRITE_JPEG_QUALITY);

    while (!appClosed) {

        {
            std::scoped_lock lock(mutex);
            frame.copyTo(input_img);
        }


        compression_params = compression_params_template; // reset parameters
        compression_params.push_back(compressionQuality);                  // set desired quality

        // try to encode
        if (!input_img.empty())
        {
            cv::imencode(suff, input_img, bytes, compression_params);
            encoded_frame = cv::imdecode(bytes, cv::IMREAD_ANYCOLOR);

            {
                std::scoped_lock lock(mutex);
                encoded_frame.copyTo(encodeFrame);
            }
        }
    }
}

void App::captureAsync(cv::Mat& frame)
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
                std::scoped_lock lk(mutex);
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
        center.x = (float)(faces[0].x + (faces[0].width / 2)) / (float)frame.cols;
        center.y = (float)(faces[0].y + (faces[0].height / 2)) / (float)frame.rows;
    }

    return center;
}
