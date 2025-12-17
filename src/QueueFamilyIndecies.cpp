#include "QueueFamilyIndecies.hpp"

QueueFamilyIndices::QueueFamilyIndices(VkPhysicalDevice device){
    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> families = std::vector<VkQueueFamilyProperties>(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, families.data());

    for(int i = 0; i < families.size(); i++){
        if(families[i].queueFlags & VK_QUEUE_COMPUTE_BIT){
            computeQueues.push_back(i);
        }
        if(families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT){
            graphicQueues.push_back(i);
        }
        if(families[i].queueFlags & VK_QUEUE_TRANSFER_BIT){
            transferQueues.push_back(i);
        }
        if(families[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT){
            sparseBindingQueues.push_back(i);
        }
    }
}