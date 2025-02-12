
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
#include < windows.h >

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

#include "irrKlang/irrKlang.h"


App::App()
{
    std::cout << "Constructed\n";
}

//============================== INIT =========================================

bool App::init()
{
    try {

        init_capture();
        init_glfw();
        init_glew();
        init_imgui();
        glfwSetFramebufferSizeCallback(window, fbsize_callback);
        glfwSetScrollCallback(window, scroll_callback);
        glfwSetCursorPosCallback(window, cursorPositionCallback);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthFunc(GL_LEQUAL);

        shaders.push_back(ShaderProgram("resources/Shaders/tex.vert", "resources/Shaders/tex.frag"));
        init_hm();
        init_assets();
        init_sound();

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
    cv::Mat frame, cameraFrame;
    cv::Point2f center, cameraCenter;
    std::thread captureThread = std::thread(&App::captureAndFindFace, this, std::ref(cameraFrame), std::ref(cameraCenter));
    std::thread findFaceThread = std::thread(&App::findFace, this, std::ref(cameraFrame), std::ref(cameraCenter));    


    try {

        glEnable(GL_DEPTH_TEST);

        glEnable(GL_LINE_SMOOTH);
        glEnable(GL_POLYGON_SMOOTH);

        glCullFace(GL_BACK);
        glEnable(GL_CULL_FACE);


        // disable cursor, so that it can not leave window, and we can process movement
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        // get first position of mouse cursor
        glfwGetCursorPos(window, &cursorLastX, &cursorLastY);
        
        update_projection_matrix();
        glViewport(0, 0, width, height);
        

        double now = glfwGetTime();
        double last{};
        // FPS related
        double fps_last_displayed = now;
        int fps_counter_frames = 0;
        double FPS = 0.0;


        // animation related
        double frame_begin_timepoint = now;
        double previous_frame_render_time{};
        double time_speed{};

        // Wait for first frame from camera
        while (frame.empty())
        {
            {
                std::scoped_lock lk(mutex);
                cameraFrame.copyTo(frame);
                center.x = cameraCenter.x;
                center.y = cameraCenter.y;
            }
        }

        // Clear color saved to OpenGL state machine: no need to set repeatedly in game loop
        glClearColor(0, 0, 0, 0);

        // set light 
        for (auto& shader : shaders) {
            shader.activate();
            shader.setUniform("ambient_intensity", ambientLight);
            shader.setUniform("diffuse_intensity", glm::vec3(0.5f));
            shader.setUniform("specular_intensity", glm::vec3(0.2f));
            shader.setUniform("specular_shinines", 10.0f);
            shader.setUniform("cut_off", glm::cos(glm::radians(25.0f)));
        }


        while (!glfwWindowShouldClose(window))
        {

            glm::vec3 player_pos = camera.Position;

            player_pos = getPositionOnTerrain(player_pos);
            player_pos.y += 20;
            camera.Position = player_pos;
            std::cout << "Player_pos: " << player_pos.x << " " << player_pos.y << " " << player_pos.z << "\n";

            glm::vec3 player_look = camera.GetViewMatrix()[2];
            glm::vec3 player_up = camera.Up;
           
            engine->setListenerPosition(irrklang::vec3df(player_pos.x, player_pos.y, player_pos.z), irrklang::vec3df(player_look.x, player_look.y, player_look.z), irrklang::vec3df(0, 0, 0), irrklang::vec3df(player_up.x, player_up.y, player_up.z));
                    
            now = glfwGetTime();
            double delta_t = now - last; // render time of the last fram
            last = now;
            camera.ProcessInput(window, delta_t); // process keys etc.

            //
            // RENDER: GL drawCalls
            // 

            // Clear OpenGL canvas, both color buffer and Z-buffer
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


            //########## create and set View Matrix according to camera settings  ##########
            {
                std::scoped_lock lk(mutex);
                cameraFrame.copyTo(frame);
                center.x = cameraCenter.x;
                center.y = cameraCenter.y;
            }
            

            std::vector<Mesh*> transparent;    // temporary, vector of pointers to transparent objects
            transparent.reserve(scene.size());  // reserve size for all objects to avoid reallocation

            for (auto& shader : shaders) {
                shader.activate();
                // set projection matrices
                shader.setUniform("uV_m", camera.GetViewMatrix());
                shader.setUniform("uP_m", projection_matrix);
                shader.setUniform("camPos", camera.Position);

                // set spotlight
                shader.setUniform("spotlight_direction", camera.Front);
                shader.setUniform("spotlight_on", spotlight_on);
            }

            
            // FIRST PART - draw all non-transparent in any order
            float dog_speed = delta_t * 14;
            for (auto& m : scene) {
                if (!m.second.transparent) {
                    if (m.first == "dog") {
                        move_dog(m.second, dog_speed, player_pos);
                    }
                    m.second.draw();
                }
                else
                    transparent.emplace_back(&m.second); // save pointer for painters algorithm
            }

            // SECOND PART - draw only transparent - painter's algorithm (sort by distance from camera, from far to near)
            std::sort(transparent.begin(), transparent.end(), [&](Mesh const* a, Mesh const* b) {
                return glm::distance(camera.Position, a->origin) > glm::distance(camera.Position, b->origin); // sort by distance from camera
                });

            // set GL for transparent objects
            glEnable(GL_BLEND);
            glDepthMask(GL_FALSE);
            glDisable(GL_CULL_FACE);

            // draw sorted transparent
            for (auto mesh : transparent) {
                mesh->draw();
            }

            // restore GL properties
            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);
            glEnable(GL_CULL_FACE);

            if (show_imgui) {
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();
                ImGui::SetNextWindowPos(ImVec2(10, 10));
                ImGui::SetNextWindowSize(ImVec2(250, 150));
                ImGui::Begin("Info", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
                ImGui::Text("V-Sync: %s", vsync ? "ON" : "OFF");
                ImGui::Text("FPS: %.1f", FPS);
                ImGui::Text("H to show/hide info");
                ImGui::Text("C to show/hide cursor");
                ImGui::Text("M to mute sound");
                ImGui::Text("L to toggle spotlight");
                ImGui::End();
            }

            if (show_imgui) {
                ImGui::Render();
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            }


            //
            // SWAP + VSYNC
            //
            glfwSwapBuffers(window);

            //
            // POLL
            //
            glfwPollEvents();

            // Time/FPS measurement
            previous_frame_render_time = now - frame_begin_timepoint; //compute delta_t
            frame_begin_timepoint = now; // set new start

            fps_counter_frames++;
            if (now - fps_last_displayed >= 1) {
                FPS = fps_counter_frames / (now - fps_last_displayed);
                fps_last_displayed = now;
                fps_counter_frames = 0;
            }
        }
    }
    catch (std::exception const& e) {
        std::cerr << "App failed : " << e.what() << std::endl;
        captureThread.join();
        findFaceThread.join();
        return EXIT_FAILURE;
    }
    captureThread.join();
    findFaceThread.join();
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

    // clean-up irrKlang
    if (engine)
        engine->drop();
    if (music)
        music->drop();
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

void App::move_dog(Mesh& m, float dog_speed, glm::vec3 player_pos) {
    //std::cout << "Dog_pos: " << m.origin.x << " " << m.origin.y << " " << m.origin.z << "\n";
    if (glm::round(player_pos.x - m.origin.x) >= 25) {
        m.origin.x += dog_speed;
        m.orientation.y = 90.0f;
    }
    else if (glm::round(player_pos.x - m.origin.x) <= -25) {
        m.origin.x -= dog_speed;
        m.orientation.y = 270.0f;
    }
    if (glm::round(player_pos.z - m.origin.z) >= 20) {
        m.origin.z += dog_speed;
        m.orientation.y = 0.0f;
    }
    else if (glm::round(player_pos.z - m.origin.z) <= -20) {
        m.origin.z -= dog_speed;
        m.orientation.y = 180.0f;
    }
    m.origin = getPositionOnTerrain(m.origin);
    m.origin.y += 8;
}


