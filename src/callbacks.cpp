#include <iostream>
#include <ctime>

#include  "App.h"

void App::error_callback(int error, const char* description) {
    std::cerr << "Error: " << description << std::endl;
}


void App::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    auto this_inst = static_cast<App*>(glfwGetWindowUserPointer(window));
    this_inst->fov += 10 * yoffset; // yoffset is mostly +1 or -1; one degree difference in fov is not visible
    this_inst->fov = std::clamp(this_inst->fov, 20.0f, 170.0f); // limit FOV to reasonable values...

    this_inst->update_projection_matrix();
}

bool cursor_visibility = false;
void App::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto this_inst = static_cast<App*>(glfwGetWindowUserPointer(window));

    if ((action == GLFW_PRESS) || (action == GLFW_REPEAT))
    {
        switch (key)
        {
        case GLFW_KEY_H:
            this_inst->show_imgui = !this_inst->show_imgui;
            break;
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        case GLFW_KEY_V:
            this_inst->vsync = this_inst->vsync == 1 ? 0 : 1;
            glfwSwapInterval(this_inst->vsync);
            break;
        case GLFW_KEY_L:
            this_inst->spotlight_on = !this_inst->spotlight_on;
            break;
        case GLFW_KEY_P: // Take screenshot
            {
                BYTE* pixels = new BYTE[3 * this_inst->width * this_inst->height];
                glReadPixels(0, 0, this_inst->width, this_inst->height, GL_RGB, GL_UNSIGNED_BYTE, pixels);
                cv::Mat screenshot = cv::Mat(this_inst->height, this_inst->width, CV_8UC3, pixels);

                // Get filename 
                time_t t = time(0);
                std::tm* now = std::localtime(&t);
                std::string time = std::to_string(now->tm_year+1900) + '-' + std::to_string(now->tm_mon + 1) + '-' + std::to_string(now->tm_mday) + "_"
                    + std::to_string(now->tm_hour) + "-" + std::to_string(now->tm_min) + "-"+ std::to_string(now->tm_sec);
                std::string  filename = "screenshot_" + time + ".jpg";
                std::cout << filename << std::endl;

                cv::Mat dst;
                cv::flip(screenshot, dst, 0);
                cv::cvtColor(dst, dst, cv::COLOR_BGR2RGB);
                cv::imwrite(filename, dst);
                break;
            }
        case GLFW_KEY_C: { // TOGGLE CURSOR
            cursor_visibility = !cursor_visibility;
            if (cursor_visibility) {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
            else {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                glfwGetCursorPos(window, &(this_inst->cursorLastX), &(this_inst->cursorLastY));
            }
            break;
        }
        case GLFW_KEY_M: { // PAUSE/PLAY SOUND
            if (this_inst->music) {
                if (this_inst->music->getIsPaused()) {
                    this_inst->music->setIsPaused(false);
                }
                else {
                    this_inst->music->setIsPaused(true);
                }
            }
            break;
        }
        case GLFW_KEY_F: { // TOGGLE FULLSCREEN/WINDOW
            this_inst->fullscreen_switch();
            break;
        }
        default:
            break;
        }
    }
}

void App::fbsize_callback(GLFWwindow* window, int width, int height)
{
    auto this_inst = static_cast<App*>(glfwGetWindowUserPointer(window));
    this_inst->width = width;
    this_inst->height = height;

    // set viewport
    glViewport(0, 0, width, height);
    //now your canvas has [0,0] in bottom left corner, and its size is [width x height] 

    this_inst->update_projection_matrix();
};

void App::cursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
    if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
    auto app = static_cast<App*>(glfwGetWindowUserPointer(window));

    app->camera.ProcessMouseMovement(xpos - app->cursorLastX, (ypos - app->cursorLastY) * -1.0);
    app->cursorLastX = xpos;
    app->cursorLastY = ypos;
    }
}

void App::fullscreen_switch() {
    is_fullscreen = !is_fullscreen;

    if (is_fullscreen) {
        glfwGetWindowPos(window, &window_pos_x, &window_pos_y);
        glfwGetWindowSize(window, &width, &height);

        monitor = glfwGetPrimaryMonitor();
        mode = glfwGetVideoMode(monitor);

        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    }
    else {
        glfwSetWindowMonitor(window, nullptr, window_pos_x, window_pos_y, windowed_width, windowed_height, GLFW_DONT_CARE);
    }
}