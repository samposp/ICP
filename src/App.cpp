
// C++
// include anywhere, in any order
#include <iostream>
#include <chrono>
#include <stack>
#include <random>
#include <numeric>
#include <thread>
#include <mutex>

#include "gl_err_callback.h"
#include "App.h"


App::App()
{
    std::cout << "Constructed\n";
}

//============================== INIT =========================================

bool App::init()
{
    try {

        // init glfw
        // https://www.glfw.org/documentation.html
        if (!glfwInit())
            return -1;


        // open window (GL canvas) with no special properties
        // https://www.glfw.org/docs/latest/quick.html#quick_create_window
        window = glfwCreateWindow(800, 600, "OpenGL context", NULL, NULL);
        if (!window)
        {
            glfwTerminate();
            return -1;
        }
        
        glfwSetWindowUserPointer(window, this);
        glfwSetErrorCallback(error_callback);

        glfwSetScrollCallback(window, scroll_callback);
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

            // get info about GL
            GLint profile, majorVersion, minorVersion;

            glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);
            if (profile & GL_CONTEXT_CORE_PROFILE_BIT) {
                std::cout << "We are using CORE profile\n";
            }
            else {
                if (profile & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT) {
                    std::cout << "We are using COMPATIBILITY profile\n";
                }
                else {
                    throw std::exception("Unknown profile of GL profile\n");
                }
            }

            glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
            glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
            if (majorVersion > 4 || (majorVersion == 4 && minorVersion > 6)) {
                std::cerr << "GL initialized ONLY with version: " << majorVersion << '.' << minorVersion << std::endl;
            }
            std::cout << "GL initialized with version: " << majorVersion << '.' << minorVersion << std::endl;
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

        glfwSwapInterval(vsync); // Set V-Sync OFF.

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
    float fps;
    float previous = (float)glfwGetTime();
    try {
        while (!glfwWindowShouldClose(window))
        {

            glClear(GL_COLOR_BUFFER_BIT);

            float current = (float)glfwGetTime();
            fps = 1.f / (current - previous);
            previous = current;

            char title[30];
            snprintf(title, sizeof title, "V-sync: %d, fps: %f", vsync, fps);
            glfwSetWindowTitle(window, title);

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

//============================= HELPER FUNCTIONS ============================
void App::error_callback(int error, const char* description) {
    std::cerr << "Error: " << description << std::endl;
}


void App::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (yoffset > 0.0) {
        std::cout << "wheel up...\n";
    }
}

void App::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto this_inst = static_cast<App*>(glfwGetWindowUserPointer(window));

    if ((action == GLFW_PRESS) || (action == GLFW_REPEAT))
    {
        switch (key)
        {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        case GLFW_KEY_V:
            this_inst->vsync = this_inst->vsync == 1 ? 0: 1;
            glfwSwapInterval(this_inst->vsync);

            break;
        default:
            break;
        }
    }
}