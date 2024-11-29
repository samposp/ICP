
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
#include "ShaderProgram.hpp"
#include "Model.h"

App::App()
{
    std::cout << "Constructed\n";
}

//============================== INIT =========================================

bool App::init()
{
    try {

        init_glfw();
        init_glew();
        init_imgui();
        glfwSetFramebufferSizeCallback(window, fbsize_callback);
        glfwSetScrollCallback(window, scroll_callback);
        glfwSetCursorPosCallback(window, cursorPositionCallback);

        init_assets();
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

        glCullFace(GL_BACK);
        glEnable(GL_CULL_FACE);

        // disable cursor, so that it can not leave window, and we can process movement
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        // get first position of mouse cursor
        glfwGetCursorPos(window, &cursorLastX, &cursorLastY);

        update_projection_matrix();
        glViewport(0, 0, width, height);

        camera.Position = glm::vec3(0, 0, 1000);

        double last_frame_time = glfwGetTime();
        // FPS related
        double fps_last_displayed = last_frame_time;
        int fps_counter_frames = 0;
        double FPS = 0.0;

        // animation related
        double frame_begin_timepoint = last_frame_time;
        double previous_frame_render_time{};
        double time_speed{};

        // Clear color saved to OpenGL state machine: no need to set repeatedly in game loop
        glClearColor(0, 0, 0, 0);

        while (!glfwWindowShouldClose(window))
        {

            //
            // RENDER: GL drawCalls
            // 

            // Clear OpenGL canvas, both color buffer and Z-buffer
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            double delta_t = glfwGetTime() - last_frame_time; // render time of the last frame 
            last_frame_time = glfwGetTime();
            camera.ProcessInput(window, delta_t); // process keys etc.

            //########## create and set View Matrix according to camera settings  ##########
            for (auto& tuple: scene) {
                Mesh mesh = tuple.second;

                mesh.draw(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(100.0f));
                mesh.shader.setUniform("uV_m", camera.GetViewMatrix());
                mesh.shader.setUniform("uP_m", projection_matrix);
            }

            //
            // SWAP + VSYNC
            //
            glfwSwapBuffers(window);

            //
            // POLL
            //
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

    // clean up ImGUI
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // clean-up GLFW
    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }
    glfwTerminate();
}

void App::update_projection_matrix(void)
{
    if (height < 1)
        height = 1;   // avoid division by 0

    float ratio = static_cast<float>(width) / height;

    projection_matrix = glm::perspective(
        glm::radians(fov),   // The vertical Field of View, in radians: the amount of "zoom". Think "camera lens". Usually between 90° (extra wide) and 30° (quite zoomed in)
        ratio,               // Aspect Ratio. Depends on the size of your window.
        0.1f,                // Near clipping plane. Keep as big as possible, or you'll get precision issues.
        20000.0f             // Far clipping plane. Keep as little as possible.
    );
}