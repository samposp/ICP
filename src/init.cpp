// C++
// include anywhere, in any order
#include <iostream>
#include <chrono>
#include <stack>
#include <random>
#include <numeric>
#include <vector>

#include <opencv2/opencv.hpp>
#include <GL/glew.h> 
#include <GL/wglew.h> 
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// ImGUI headers
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "gl_err_callback.h"
#include "App.h"
#include "ShaderProgram.hpp"
#include "Model.h"
#include "OBJLoader.hpp"

void App::init_assets(void)
{
    // load models, load textures, load shaders, initialize level, etc...   -most can be parallel
    shader = ShaderProgram("resources/Shaders/basic_core.vert", "resources/Shaders/basic_core.frag");


    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;

    glm::vec3 origin = glm::vec3(0.0f);
    glm::vec3 orientation = glm::vec3(0.0f);

    loadOBJ("resources/Objects/cube_triangles_vnt.obj", vertices, indices);

    Mesh my_mesh = Mesh(GL_TRIANGLES, shader, vertices, indices, origin, orientation);

    scene.insert({ "mesh1", my_mesh });
}

void App::init_imgui() {
    // see https://github.com/ocornut/imgui/wiki/Getting-Started

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();
    std::cout << "ImGUI version: " << ImGui::GetVersion() << "\n";
}

void App::init_glfw() {
    // https://www.glfw.org/documentation.html
    if (!glfwInit())
        throw std::runtime_error(std::string("Error in Init glfw"));


    // open window (GL canvas) with no special properties
    // https://www.glfw.org/docs/latest/quick.html#quick_create_window
    window = glfwCreateWindow(width, height, "OpenGL context", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        throw std::runtime_error(std::string("Error in creating window with glfw"));
    }

    glfwSetWindowUserPointer(window, this);
    glfwSetErrorCallback(error_callback);

    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwMakeContextCurrent(window);

    glfwSwapInterval(vsync); // Set V-Sync OFF.
}

void App::init_glew() {
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

    if (!GLEW_ARB_direct_state_access)
        throw std::runtime_error("No DSA :-(");
}