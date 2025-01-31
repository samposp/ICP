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
    //shader = ShaderProgram("resources/Shaders/basic_core.vert", "resources/Shaders/basic_core.frag");
    //shaders.push_back(shader);

    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;

    // CREATE CUBE
    glm::vec3 orientation = glm::vec3(45.0f);
    glm::vec3 size = glm::vec3(10.0f);
    glm::vec3 origin = glm::vec3(300.0f, 0.0f, 400.0f);
    origin = getPositionOnTerrain(origin);
    origin.y += 5;
    loadOBJ("resources/Objects/cube_tri_vnt.obj", vertices, indices);
    Mesh cube = Mesh(GL_TRIANGLES, shaders[0], vertices, indices, origin, orientation, size);
    cube.texture_id = textureInit("resources/textures/box_rgb888.png");
    scene.insert({ "cube", cube });
    

    // TRANSPARENT CUBE
    orientation = glm::vec3(0.0f);
    size = glm::vec3(10.0f);
    origin = glm::vec3(10.0f, 10.0f, 0.0f);
    origin = getPositionOnTerrain(origin);
    origin.y += 5;
    Mesh transparentCube = Mesh(GL_TRIANGLES, shaders[0], vertices, indices, origin, orientation, size);
    transparentCube.texture_id = textureInit("resources/textures/box_rgb888.png");
    transparentCube.diffuse_color = glm::vec4(1.0f, 1.0f, 1.0f, 0.5f);
    transparentCube.transparent = true;
    scene.insert({ "transparentCube", transparentCube });
    

    // TEAPOT WITH SOUND
    loadOBJ("resources/Objects/teapot_tri_vnt.obj", vertices, indices);
    orientation = glm::vec3(-25.0f);
    origin = glm::vec3(515.0f, 136.0f, 526.0f);
    size = glm::vec3(1.5f);
    origin = getPositionOnTerrain(origin);
    origin.y += 5;
    Mesh teapot = Mesh(GL_TRIANGLES, shaders[0], vertices, indices, origin, orientation, size);
    teapot.texture_id = textureInit("resources/textures/pot_texture.jpg");
    scene.insert({ "teapot", teapot });

    //loadOBJ("resources/Objects/sphere_tri_vnt.obj", vertices, indices);
    //size = glm::vec3(100.0f);
    //origin = glm::vec3(-5.0f, 0.0f, 0.0f);
    //Mesh sphere = Mesh(GL_TRIANGLES, shader, vertices, indices, origin, orientation, size);
    //scene.insert({ "sphere", sphere });
}

void App::init_capture() {
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

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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

GLuint App::textureInit(const std::filesystem::path & file_name)
{
    cv::Mat image = cv::imread(file_name.string(), cv::IMREAD_UNCHANGED);  // Read with (potential) Alpha
    if (image.empty()) {
        throw std::runtime_error("No texture in file: " + file_name.string());
    }

    // or print warning, and generate synthetic image with checkerboard pattern 
    // using OpenCV and use as a texture replacement 

    GLuint texture = gen_tex(image);

    return texture;
}

GLuint App::gen_tex(cv::Mat& image)
{
    GLuint ID;

    if (image.empty()) {
        throw std::runtime_error("Image empty?\n");
    }

    // Generates an OpenGL texture object
    glCreateTextures(GL_TEXTURE_2D, 1, &ID);

    switch (image.channels()) {
    case 1:
        glTextureStorage2D(ID, 1, GL_R8, image.cols, image.rows);
        glTextureSubImage2D(ID, 0, 0, 0, image.cols, image.rows, GL_R8, GL_UNSIGNED_BYTE, image.data);
        break;
    case 3:
        // Create and clear space for data - immutable format
        glTextureStorage2D(ID, 1, GL_RGB8, image.cols, image.rows);
        // Assigns the image to the OpenGL Texture object
        glTextureSubImage2D(ID, 0, 0, 0, image.cols, image.rows, GL_BGR, GL_UNSIGNED_BYTE, image.data);
        break;
    case 4:
        glTextureStorage2D(ID, 1, GL_RGBA8, image.cols, image.rows);
        glTextureSubImage2D(ID, 0, 0, 0, image.cols, image.rows, GL_BGRA, GL_UNSIGNED_BYTE, image.data);
        break;
    default:
        throw std::runtime_error("texture failed"); // Check the image, we want Alpha in this example    
    }

    // Configures the type of algorithm that is used to make the image smaller or bigger
    // nearest neighbor - ugly & fast 
    //glTextureParameteri(ID, GL_TEXTURE_MAG_FILTER, GL_NEAREST);  
    //glTextureParameteri(ID, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // bilinear - nicer & slower
    //glTextureParameteri(ID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);    
    //glTextureParameteri(ID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // MIPMAP filtering + automatic MIPMAP generation - nicest, needs more memory. Notice: MIPMAP is only for image minifying.
    glTextureParameteri(ID, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // bilinear magnifying
    glTextureParameteri(ID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // trilinear minifying
    glGenerateTextureMipmap(ID);  //Generate mipmaps now.

    // Configures the way the texture repeats
    glTextureParameteri(ID, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(ID, GL_TEXTURE_WRAP_T, GL_REPEAT);

    return ID;
}

void App::init_sound() {
    engine = irrklang::createIrrKlangDevice();
    if (!engine) {
        throw std::runtime_error("Can not initialize sound engine.");
    }
    engine->setRolloffFactor(0.8f);
    engine->setSoundVolume(0.7f);

    glm::vec3 tepotPos = scene.at("teapot").origin;

    irrklang::vec3df soundPos = irrklang::vec3df(tepotPos.x, tepotPos.y, tepotPos.z);
    music = engine->play3D("resources/sound/calm_rain.mp3", soundPos, true, false, true);
    if (music)
        music->setMinDistance(20.0f);
}