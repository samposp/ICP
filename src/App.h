#pragma once

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

#include "camera.hpp"
#include "Mesh.h"


class App {
public:
    App();

    bool init(void);
    int run(void);

    void init_assets(void);
    void init_glew();
    void init_glfw();
    void init_imgui();
    void init_capture();
    void init_hm(void);
    Mesh GenHeightMap(const cv::Mat& hmap, const unsigned int mesh_step_size);
    static void error_callback(int error, const char* description);
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void fbsize_callback(GLFWwindow* window, int width, int height);
    static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos);
    void update_projection_matrix(void);
    GLuint gen_tex(cv::Mat& image);
    GLuint textureInit(const std::filesystem::path& file_name);

    void captureAndFindFace(cv::Mat& frame, cv::Point2f& faceCenter);
    void findFace(cv::Mat& frame, cv::Point2f outCenter);
    ~App();

private:
    std::mutex mutex;
    std::atomic<bool> cameraRunning = false;
    std::atomic<bool> stopApp = false;
    cv::VideoCapture capture;
    cv::CascadeClassifier faceCascade = cv::CascadeClassifier("resources/haarcascade_frontalface_default.xml");


    GLFWwindow* window = NULL;
    int vsync = 0;
    bool show_imgui = true;

    GLuint shader_prog_ID{ 0 };
    GLuint VBO_ID{ 0 };
    GLuint VAO_ID{ 0 };

    GLfloat r{ 1.0f }, g{ 0.0f }, b{ 0.0f }, a{ 1.0f };

    std::unordered_map<std::string, Mesh> scene;

    std::vector<ShaderProgram> shaders;
    //ShaderProgram shader;
    
protected: 
    int width{ 800 }, height{ 600 };
    float fov = 60.0f;
    glm::mat4 projection_matrix = glm::identity<glm::mat4>();

    // camera related 
    Camera camera = Camera(glm::vec3(0, 0, 1000));
    // remember last cursor position, move relative to that in the next frame
    double cursorLastX{ 0 };
    double cursorLastY{ 0 };
};