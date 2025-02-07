#pragma once

#include <optional>

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

#include "irrKlang/irrKlang.h"

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
    void init_sound();
    Mesh GenHeightMap(const cv::Mat& hmap, const unsigned int mesh_step_size);
    glm::vec3 getPositionOnTerrain(glm::vec3 position);
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

    irrklang::ISoundEngine* engine = nullptr;
    irrklang::ISound* music = nullptr;

    void move_dog(Mesh& m, float dog_speed, glm::vec3 player_pos);

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

    cv::Mat terrain;

    GLuint shader_prog_ID{ 0 };
    GLuint VBO_ID{ 0 };
    GLuint VAO_ID{ 0 };

    std::unordered_map<std::string, Mesh> scene;

    std::vector<ShaderProgram> shaders;

    // Lights
    glm::vec3 ambientLight = glm::vec3(0.2);
    bool spotlight_on = true;

    
protected: 
    int width{ 800 }, height{ 600 };
    float fov = 60.0f;
    glm::mat4 projection_matrix = glm::identity<glm::mat4>();

    // camera related 
    Camera camera = Camera(glm::vec3(500.0f, 0, 500.0f));
    // remember last cursor position, move relative to that in the next frame
    double cursorLastX{ 0 };
    double cursorLastY{ 0 };
};