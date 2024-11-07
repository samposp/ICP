
// C++
// include anywhere, in any order
#include <iostream>
#include <chrono>
#include <stack>
#include <random>
#include <numeric>
#include <thread>
#include <mutex>
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
#include "assets.hpp"


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

        if (!GLEW_ARB_direct_state_access)
            throw std::runtime_error("No DSA :-(");

        // see https://github.com/ocornut/imgui/wiki/Getting-Started

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init();
        std::cout << "ImGUI version: " << ImGui::GetVersion() << "\n";

        init_assets();

    }
    catch (std::exception const& e) {
        std::cerr << "Init failed : " << e.what() << std::endl;
        throw;
    }
    std::cout << "Initialized...\n";
    return true;
}

void App::init_assets(void) {
    //
    // Initialize pipeline: compile, link and use shaders
    //

    //SHADERS - define & compile & link
    const char* vertex_shader =
        "#version 460 core\n"
        "in vec3 attribute_Position;"
        "void main() {"
        "  gl_Position = vec4(attribute_Position, 1.0);"
        "}";

    const char* fragment_shader =
        "#version 460 core\n"
        "uniform vec4 uniform_Color;"
        "out vec4 FragColor;"
        "void main() {"
        "  FragColor = uniform_Color;"
        "}";

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertex_shader, NULL);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragment_shader, NULL);
    glCompileShader(fs);

    shader_prog_ID = glCreateProgram();
    glAttachShader(shader_prog_ID, fs);
    glAttachShader(shader_prog_ID, vs);
    glLinkProgram(shader_prog_ID);

    //now we can delete shader parts (they can be reused, if you have more shaders)
    //the final shader program already linked and stored separately
    glDeleteShader(vs);
    glDeleteShader(fs);

    // 
    // Create and load data into GPU using OpenGL DSA (Direct State Access)
    //

    // Create VAO + data description (just envelope, or container...)
    glCreateVertexArrays(1, &VAO_ID);

    GLint position_attrib_location = glGetAttribLocation(shader_prog_ID, "attribute_Position");

    glEnableVertexArrayAttrib(VAO_ID, position_attrib_location);
    glVertexArrayAttribFormat(VAO_ID, position_attrib_location, 3, GL_FLOAT, GL_FALSE, offsetof(vertex, position));
    glVertexArrayAttribBinding(VAO_ID, position_attrib_location, 0); // (GLuint vaobj, GLuint attribindex, GLuint bindingindex)

    // Create and fill data
    glCreateBuffers(1, &VBO_ID);
    glNamedBufferData(VBO_ID, triangle_vertices.size() * sizeof(vertex), triangle_vertices.data(), GL_STATIC_DRAW);

    // Connect together
    glVertexArrayVertexBuffer(VAO_ID, 0, VBO_ID, 0, sizeof(vertex)); // (GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride)
}

//================================== RUN ===================================

int App::run(void)
{


    try {
        //GLfloat r, g, b, a;
        //r = g = b = a = 1.0f;

        // Activate shader program. There is only one program, so activation can be out of the loop. 
        // In more realistic scenarios, you will activate different shaders for different 3D objects.
        glUseProgram(shader_prog_ID);

        // Get uniform location in GPU program. This will not change, so it can be moved out of the game loop.
        GLint uniform_color_location = glGetUniformLocation(shader_prog_ID, "uniform_Color");
        if (uniform_color_location == -1) {
            std::cerr << "Uniform location is not found in active shader program. Did you forget to activate it?\n";
        }

        double now = glfwGetTime();
        // FPS related
        double fps_last_displayed = now;
        int fps_counter_frames = 0;
        double FPS = 0.0;

        // animation related
        double frame_begin_timepoint = now;
        double previous_frame_render_time{};
        double time_speed{};

        // Clear color saved to OpenGL state machine: no need to set repeatedly in game loop
        glClearColor(0, 0, 0, 0);

        while (!glfwWindowShouldClose(window))
        {
            // ImGui prepare render (only if required)
            if (show_imgui) {
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();
                //ImGui::ShowDemoWindow(); // Enable mouse when using Demo!
                ImGui::SetNextWindowPos(ImVec2(10, 10));
                ImGui::SetNextWindowSize(ImVec2(250, 100));

                ImGui::Begin("Info", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
                ImGui::Text("V-Sync: %s", vsync ? "ON" : "OFF");
                ImGui::Text("FPS: %.1f", FPS);
                ImGui::Text("(press RMB to release mouse)");
                ImGui::Text("(hit D to show/hide info)");
                ImGui::End();
            }

            //
            // UPDATE: recompute object.position = object.position + object.speed * (previous_frame_render_time * time_speed); // s = s0 + v*delta_t
            //
            if (show_imgui) {
                // pause application
                time_speed = 0.0;
            }
            else {
                // imgui not displayed, run app at normal speed
                time_speed = 1.0;
            }

            //
            // RENDER: GL drawCalls
            // 

            // Clear OpenGL canvas, both color buffer and Z-buffer
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            // drawCalls to render object, scene, ...


            // ImGui display
            if (show_imgui) {
                ImGui::Render();
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            }

            //set uniform parameter for shader
// (try to change the color in key callback)          
            glUniform4f(uniform_color_location, r, g, b, a);

            //bind 3d object data
            glBindVertexArray(VAO_ID);

            // draw all VAO data
            glDrawArrays(GL_TRIANGLES, 0, triangle_vertices.size());

            //
            // SWAP + VSYNC
            //
            glfwSwapBuffers(window);

            //
            // POLL
            //
            glfwPollEvents();

            // Time/FPS measurement
            now = glfwGetTime();
            previous_frame_render_time = now - frame_begin_timepoint; //compute delta_t
            frame_begin_timepoint = now; // set new start

            fps_counter_frames++;
            if (now - fps_last_displayed >= 1) {
                FPS = fps_counter_frames / (now - fps_last_displayed);
                fps_last_displayed = now;
                fps_counter_frames = 0;
                std::cout << "\r[FPS]" << FPS << "     "; // Compare: FPS with/without ImGUI
            }
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

    //new stuff: cleanup GL data
    glDeleteProgram(shader_prog_ID);
    glDeleteBuffers(1, &VBO_ID);
    glDeleteVertexArrays(1, &VAO_ID);
    // clean up ImGUI
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // clean up OpenCV
    cv::destroyAllWindows();

    // clean-up GLFW
    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    glfwTerminate();
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
        case GLFW_KEY_KP_7:
            this_inst->r += 0.1;
            this_inst->r = std::clamp(this_inst->r, 0.0f, 1.0f);
            break;
        case GLFW_KEY_KP_4:
            this_inst->r -= 0.1;
            this_inst->r = std::clamp(this_inst->r, 0.0f, 1.0f);
            break;
        case GLFW_KEY_KP_8:
            this_inst->g += 0.1;
            this_inst->g = std::clamp(this_inst->g, 0.0f, 1.0f);
            break;
        case GLFW_KEY_KP_5:
            this_inst->g -= 0.1;
            this_inst->g = std::clamp(this_inst->g, 0.0f, 1.0f);
        case GLFW_KEY_KP_9:
            this_inst->b += 0.1;
            this_inst->b = std::clamp(this_inst->b, 0.0f, 1.0f);
            break;
        case GLFW_KEY_KP_6:
            this_inst->b -= 0.1;
            this_inst->b = std::clamp(this_inst->b, 0.0f, 1.0f);
        default:
            break;
        }
    }
}