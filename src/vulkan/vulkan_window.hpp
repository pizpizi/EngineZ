#pragma once

#include "enginez/graphics/enginez_window.hpp"
#include "logz/logger.hpp"
#include "vulkan/vulkan_core.h"
#include <string>

class VulkanWindow : public enginez::graphics::EnginezWindow {
  public:
    VulkanWindow(VkInstance& instance, std::string name, int width, int height);
    void cleanUp() override;
    void update() override;

    void setHeight(int val) override;
    void setWidth(int val) override;
    void setTitle(std::string val) override;

  private:
    logz::DefaultLogger& logger;

    void setupLogger();
};