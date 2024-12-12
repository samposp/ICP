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
            this_inst->vsync = this_inst->vsync == 1 ? 0 : 1;
            glfwSwapInterval(this_inst->vsync);
            break;
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
            break;
        case GLFW_KEY_KP_9:
            this_inst->b += 0.1;
            this_inst->b = std::clamp(this_inst->b, 0.0f, 1.0f);
            break;
        case GLFW_KEY_KP_6:
            this_inst->b -= 0.1;
            this_inst->b = std::clamp(this_inst->b, 0.0f, 1.0f);
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
    auto app = static_cast<App*>(glfwGetWindowUserPointer(window));

    app->camera.ProcessMouseMovement(xpos - app->cursorLastX, (ypos - app->cursorLastY) * -1.0);
    app->cursorLastX = xpos;
    app->cursorLastY = ypos;
}