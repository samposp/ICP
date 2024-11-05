
// C++
// include anywhere, in any order
#include <iostream>
#include <chrono>
#include <stack>
#include <random>
#include <numeric>
#include <thread>
#include <mutex>

// OpenCV (does not depend on GL)
#include <opencv2\opencv.hpp>

// OpenGL Extension Wrangler: allow all multiplatform GL functions
#include <GL/glew.h> 
// WGLEW = Windows GL Extension Wrangler (change for different platform) 
// platform specific functions (in this case Windows)
#include <GL/wglew.h> 

// GLFW toolkit
// Uses GL calls to open GL context, i.e. GLEW __MUST__ be first.
#include <GLFW/glfw3.h>

// OpenGL math (and other additional GL libraries, at the end)
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "gl_err_callback.h"

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
    GLFWwindow* window = NULL;
};

App::App()
{
    std::cout << "Constructed\n";
}

//============================== INIT =========================================


void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

bool App::init()
{
    try {

        // init glfw
        // https://www.glfw.org/documentation.html
        if (!glfwInit())
            return -1;

        glfwSetErrorCallback(error_callback);
        

        // open window (GL canvas) with no special properties
        // https://www.glfw.org/docs/latest/quick.html#quick_create_window
        window = glfwCreateWindow(800, 600, "OpenGL context", NULL, NULL);
        if (!window)
        {
            glfwTerminate();
            return -1;
        }

        glfwSetKeyCallback(window, key_callback);
        glfwMakeContextCurrent(window);

        GLenum glew_ret;
        glew_ret = glewInit();
        if (glew_ret != GLEW_OK) {
            throw std::runtime_error(std::string("GLEW failed with error: ")
                + reinterpret_cast<const char*>(glewGetErrorString(glew_ret)));
        }
        else {
            std::cout << "GLEW successfully initialized to version: " << glewGetString(GLEW_VERSION) << std::endl;
        }
        // Platform specific init.
        glew_ret = wglewInit();
        if (glew_ret != GLEW_OK) {
            throw std::runtime_error(std::string("WGLEW failed with error: ")
                + reinterpret_cast<const char*>(glewGetErrorString(glew_ret)));
        }
        else {
            std::cout << "WGLEW successfully initialized platform specific functions." << std::endl;
        }

        if (GLEW_ARB_debug_output)
        {
            glDebugMessageCallback(MessageCallback, 0);
            glEnable(GL_DEBUG_OUTPUT);

            //default is asynchronous debug output, use this to simulate glGetError() functionality
            //glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

            std::cout << "GL_DEBUG enabled." << std::endl;
        }
        else
            std::cout << "GL_DEBUG NOT SUPPORTED!" << std::endl;

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
        while (!glfwWindowShouldClose(window))
        {

            glClear(GL_COLOR_BUFFER_BIT);

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }
    catch (std::exception const& e) {
        appClosed = true;
        std::cerr << "App failed : " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

//============================ DESTRUCTOR =================================
App::~App()
{
    if (window)
        glfwDestroyWindow(window);
    glfwTerminate();
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
