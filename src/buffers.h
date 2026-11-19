#pragma once

#include "cglm/types.h"
#include "structs.h"
#include "utils.h"
#include <stdint.h>
#include <stdlib.h>

void createStorageBuffer(State *state, BufferData *buffer, const void *data,
                         size_t objectSize, uint32_t objectCount) {
  buffer->objectCount = objectCount;
  VkDeviceSize size = (VkDeviceSize)objectSize * objectCount;

  PANIC_IF(vkCreateBuffer(state->context.device,
                          &(VkBufferCreateInfo){
                              .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                              .size = size,
                              .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                              .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
                          },
                          state->config.allocator, &buffer->handle),
           "Couldn't create a buffer\n");

  VkMemoryRequirements memoryRequirements;
  vkGetBufferMemoryRequirements(state->context.device, buffer->handle,
                                &memoryRequirements);

  VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
  vkGetPhysicalDeviceMemoryProperties(state->context.physicalDevice,
                                      &physicalDeviceMemoryProperties);

  uint32_t memoryPropertiesIndex = UINT32_MAX;
  VkMemoryPropertyFlags wantedFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount;
       i++) {
    bool allowedByBuffer = (memoryRequirements.memoryTypeBits & (1u << i)) != 0;
    bool hasWantedFlags =
        (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags &
         wantedFlags) == wantedFlags;

    if (allowedByBuffer && hasWantedFlags) {
      memoryPropertiesIndex = i;
      break;
    }
  }
  PANIC_IF(memoryPropertiesIndex == UINT32_MAX,
           "Memory doesn't have propperties needed\n");

  PANIC_IF(vkAllocateMemory(state->context.device,
                            &(VkMemoryAllocateInfo){
                                .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                                .memoryTypeIndex = memoryPropertiesIndex,
                                .allocationSize = memoryRequirements.size},
                            state->config.allocator, &buffer->memory),
           "Couldn't allocate memory\n");

  PANIC_IF(vkBindBufferMemory(state->context.device, buffer->handle,
                              buffer->memory, 0),
           "Couldn't bind buffer\n");
  PANIC_IF(vkMapMemory(state->context.device, buffer->memory, 0, VK_WHOLE_SIZE,
                       0, &buffer->mapped),
           "Couldn't map memory\n");

  memcpy(buffer->mapped, data, (size_t)size);
}
void destroyStorageBuffer(State *state, BufferData *buffer) {
  // Reverse of create: unmap -> destroy buffer -> free the memory behind it
  vkUnmapMemory(state->context.device, buffer->memory);
  vkDestroyBuffer(state->context.device, buffer->handle,
                  state->config.allocator);
  vkFreeMemory(state->context.device, buffer->memory, state->config.allocator);
}

void createSphereBuffer(State *state) {
  Sphere spheres[] = {
      {.center = {0.0f, 0.0f, 0.0f}, .radius = 1.0f},
  };
  createStorageBuffer(state, &state->renderer.sphereData, spheres,
                      sizeof(*spheres), sizeof(spheres) / sizeof(*spheres));
}
void destroySphereBuffer(State *state) {
  destroyStorageBuffer(state, &state->renderer.sphereData);
}

void createDescriptors(State *state) {

  uint32_t imageCount = state->window.swapchain.imageCount;

  VkDescriptorPoolSize poolSizes[] = {
      {.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
       .descriptorCount = imageCount},
      {.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = imageCount},
  };

  PANIC_IF(vkCreateDescriptorPool(
               state->context.device,
               &(VkDescriptorPoolCreateInfo){
                   .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                   .maxSets = imageCount,
                   .poolSizeCount = sizeof(poolSizes) / sizeof(*poolSizes),
                   .pPoolSizes = poolSizes,
               },
               state->config.allocator, &state->renderer.descriptorPool),
           "Couldn't create descriptor pool\n");

  state->renderer.descriptorSets = malloc(imageCount * sizeof(VkDescriptorSet));
  VkDescriptorSetLayout *layouts =
      malloc(imageCount * sizeof(VkDescriptorSetLayout));

  PANIC_IF(!state->renderer.descriptorSets || !layouts,
           "Couldn't allocate descriptor set arrays\n");

  for (uint32_t i = 0; i < imageCount; i++)
    layouts[i] = state->renderer.descriptorSetLayout;

  PANIC_IF(vkAllocateDescriptorSets(
               state->context.device,
               &(VkDescriptorSetAllocateInfo){
                   .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                   .descriptorPool = state->renderer.descriptorPool,
                   .descriptorSetCount = imageCount,
                   .pSetLayouts = layouts},
               state->renderer.descriptorSets),
           "Couldn't allocate descriptor sets\n");
  free(layouts);

  for (uint32_t i = 0; i < imageCount; i++) {
    VkWriteDescriptorSet writes[] = {
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = state->renderer.descriptorSets[i],
         .dstBinding = 0,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
         .pBufferInfo =
             &(VkDescriptorBufferInfo){.buffer =
                                           state->renderer.sphereData.handle,
                                       .range = VK_WHOLE_SIZE}},
        {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
         .dstSet = state->renderer.descriptorSets[i],
         .dstBinding = 1,
         .descriptorCount = 1,
         .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
         .pImageInfo =
             &(VkDescriptorImageInfo){
                 .imageView = state->window.swapchain.imageViews[i],
                 .imageLayout =
                     VK_IMAGE_LAYOUT_GENERAL}}, // storage image = GENERAL
    };
    vkUpdateDescriptorSets(state->context.device, 2, writes, 0, NULL);
  }
}
void destroyDescriptors(State *state) {
  // Destroying the pool also frees every descriptor set allocated from it
  vkDestroyDescriptorPool(state->context.device, state->renderer.descriptorPool,
                          state->config.allocator);
  free(state->renderer.descriptorSets);
}