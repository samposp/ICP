
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
        

        double last_frame_time = glfwGetTime();

        // Wait for first frame from camera
        while (frame.empty())
        {
            {
                std::scoped_lock lk(mutex);
                cameraFrame.copyTo(frame);
                center.x = cameraCenter.x;
                center.y = cameraCenter.y;
            }
            Sleep(100);
        }
        GLuint mytex = gen_tex(frame);
        // animation related
        double frame_begin_timepoint = last_frame_time;
        double previous_frame_render_time{};
        double time_speed{};

        // get texture size (this needs to be done just once, we use immutable format)
        int my_image_width = 0;
        int my_image_height = 0;
        int miplevel = 0;

        glBindTextureUnit(0, mytex);

        glGetTexLevelParameteriv(GL_TEXTURE_2D, miplevel, GL_TEXTURE_WIDTH, &my_image_width);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, miplevel, GL_TEXTURE_HEIGHT, &my_image_height);

        // Clear color saved to OpenGL state machine: no need to set repeatedly in game loop
        glClearColor(0, 0, 0, 0);


        while (!glfwWindowShouldClose(window))
        {

            glm::vec3 player_pos = camera.Position;

            player_pos = getPositionOnTerrain(player_pos);
            player_pos.y += 20;
            camera.Position = player_pos;

            glm::vec3 player_look = camera.GetViewMatrix()[2];
            glm::vec3 player_up = camera.Up;
           
            engine->setListenerPosition(irrklang::vec3df(player_pos.x, player_pos.y, player_pos.z), irrklang::vec3df(player_look.x, player_look.y, player_look.z), irrklang::vec3df(0, 0, 0), irrklang::vec3df(player_up.x, player_up.y, player_up.z));
                    

            //
            // RENDER: GL drawCalls
            // 

            //std::cout << std::endl << player_pos.x << ", " << player_pos.y << ", " << player_pos.z << std::endl;

            // Clear OpenGL canvas, both color buffer and Z-buffer
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            double delta_t = glfwGetTime() - last_frame_time; // render time of the last fram

            last_frame_time = glfwGetTime();
   
            camera.ProcessInput(window, delta_t); // process keys etc.


            //########## create and set View Matrix according to camera settings  ##########
            {
                std::scoped_lock lk(mutex);
                cameraFrame.copyTo(frame);
                center.x = cameraCenter.x;
                center.y = cameraCenter.y;
            }
            

            std::vector<Mesh*> transparent;    // temporary, vector of pointers to transparent objects
            transparent.reserve(scene.size());  // reserve size for all objects to avoid reallocation


            std::cout << "Spotlight dir: " << camera.Front.x << ", " << camera.Front.y << ", " << camera.Front.z << std::endl;
            for (auto& shader : shaders) {
                shader.activate();
                // set projection matrices
                shader.setUniform("uV_m", camera.GetViewMatrix());
                shader.setUniform("uP_m", projection_matrix);
                shader.setUniform("camPos", camera.Position);
                // set lights
                shader.setUniform("ambient_intensity", ambientLight);
                shader.setUniform("diffuse_intensity", glm::vec3(0.5f));
                shader.setUniform("specular_intensity", glm::vec3(0.2f));
                shader.setUniform("specular_shinines", 10.0f);
                // set spotlight
                shader.setUniform("spotlight_direction", camera.Front);
                shader.setUniform("cut_off", glm::cos(glm::radians(15.0f)));
            }

            // FIRST PART - draw all non-transparent in any order
            for (auto& m : scene) {
                if (!m.second.transparent) {
                    Mesh mesh = m.second;
                    mesh.draw();
                }
                else
                    transparent.emplace_back(&m.second); // save pointer for painters algorithm
            }

            // SECOND PART - draw only transparent - painter's algorithm (sort by distance from camera, from far to near)
            std::sort(transparent.begin(), transparent.end(), [&](Mesh const* a, Mesh const* b) {
                glm::vec3 translation_a = glm::vec3(a->model_matrix[3]);  // get 3 values from last column of model matrix = translation
                glm::vec3 translation_b = glm::vec3(b->model_matrix[3]);  // dtto for model B
                return glm::distance(camera.Position, translation_a) < glm::distance(camera.Position, translation_b); // sort by distance from camera
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
            //else {
            //    ImGui_ImplOpenGL3_NewFrame();
            //    ImGui_ImplGlfw_NewFrame();
            //    ImGui::NewFrame();
            //    ImGui::SetNextWindowPos(ImVec2(10, 10));
            //    ImGui::SetNextWindowSize(ImVec2(200, 50));
            //    ImGui::Begin("OpenGL");
            //    ImGui::Text("Aplikace zastavena");
            //    ImGui::End();
            //    ImGui::Render();
            //    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            //}

            //glTextureSubImage2D(mytex, 0, 0, 0, frame.cols, frame.rows, GL_BGR, GL_UNSIGNED_BYTE, frame.data);

             //show texture   
            //ImGui_ImplOpenGL3_NewFrame();
            //ImGui_ImplGlfw_NewFrame();
            //ImGui::NewFrame();
            //ImGui::SetNextWindowPos(ImVec2(10, 10));
            //ImGui::SetNextWindowSize(ImVec2(my_image_width, my_image_height));
            //ImGui::Begin("OpenGL Texture");
            //ImGui::Text("FPS: %.1f", 1/ last_frame_time);
            //ImGui::Image((ImTextureID)(intptr_t)mytex, ImVec2(my_image_width, my_image_height));
            //ImGui::End();
            //ImGui::Render();
            //ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

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


