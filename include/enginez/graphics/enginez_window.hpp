#pragma once
#include "GLFW/glfw3.h"
#include <string>
#include <utility>
namespace enginez::graphics {
    class EnginezWindow {
      public:
        virtual void cleanUp() = 0;
        virtual void update() = 0;

        EnginezWindow(std::string title, int width, int height) : title(title), width(width), height(height){

        }

        GLFWwindow* glfwWindow;

        // -------- get sets -------- //
        const std::string& getTitle() const {
            return title;
        }
        virtual void setTitle(std::string val) {
            title = std::move(val);
        }

        const int getWidth() const {
            return width;
        }
        virtual void setWidth(int val) = 0;

        const int getHeight() const {
            return height;
        }
        virtual void setHeight(int val) = 0;

        const bool& getClosed() const { return closed; }
        // -------------------------- //
      protected:
        std::string title;
        int width, height;

        bool closed = false;
    };
} // namespace enginez::graphics