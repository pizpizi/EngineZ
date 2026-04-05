#include "vulkan_window.hpp"
#include "GLFW/glfw3.h"
#include <format>
#include <new>
#include <stdexcept>
#include <string>
#include "GLFW/glfw3.h"
#include "logz/logger.hpp"

using namespace enginez::graphics;

void VulkanWindow::setupLogger() {
    logger.addConsoleSink(true, logz::DEBUG);
    logger.addFileSink("log.txt", logz::DEBUG);
}

VulkanWindow::VulkanWindow(VkInstance& instance, std::string title, int width, int height)
    : EnginezWindow(title, width, height), logger(logz::createDefaultLogger(logz::SINCE_PROGRAM_START, title)) {
    setupLogger();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    glfwWindow = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!glfwWindow) {
        logger.error(std::format("failed to initialize glfw window {}", title));
    }
}

void VulkanWindow::cleanUp() {
    glfwDestroyWindow(glfwWindow);
    logger.info(std::format("cleaned up window {}", title));
}

void VulkanWindow::update() {
    if(glfwWindowShouldClose(glfwWindow)){
        closed = true;
        logger.debug(std::format("window {} marked as closed", title));
    }
}


void VulkanWindow::setHeight(int val){
    //TODO
    throw std::runtime_error("not implemented");
}
void VulkanWindow::setWidth(int val){
    //TODO
    throw std::runtime_error("not implemented");
}
void VulkanWindow::setTitle(std::string val){
    //TODO
    throw std::runtime_error("not implemented");
}