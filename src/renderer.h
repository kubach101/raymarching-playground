#pragma once
#include "buffers.h"
#include "cglm/types.h"
#include "structs.h"
#include "utils.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void createComputePipeline(State *state) {
  size_t shaderSize;
  uint32_t *shaderCode =
      readSpirvFile(SHADER_DIR "raymarching.comp.spv", &shaderSize);

  VkShaderModule shaderModule;
  PANIC_IF(vkCreateShaderModule(
               state->context.device,
               &(VkShaderModuleCreateInfo){
                   .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                   .pCode = shaderCode,
                   .codeSize = shaderSize},
               state->config.allocator, &shaderModule),
           "Couldn't create compute shader module\n");

  VkDescriptorSetLayoutBinding bindings[] = {
      {.binding = 0,
       .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT},
      {.binding = 1,
       .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT},
  };
  PANIC_IF(vkCreateDescriptorSetLayout(
               state->context.device,
               &(VkDescriptorSetLayoutCreateInfo){
                   .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                   .bindingCount = sizeof(bindings) / sizeof(*bindings),
                   .pBindings = bindings},
               state->config.allocator, &state->renderer.descriptorSetLayout),
           "Couldn't create descriptor set layout\n");

  PANIC_IF(vkCreatePipelineLayout(
               state->context.device,
               &(VkPipelineLayoutCreateInfo){
                   .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                   .setLayoutCount = 1,
                   .pSetLayouts = &state->renderer.descriptorSetLayout,
                   .pushConstantRangeCount = 1,
                   .pPushConstantRanges =
                       &(VkPushConstantRange){.stageFlags =
                                                  VK_SHADER_STAGE_COMPUTE_BIT,
                                              .size = sizeof(PushConstants)}},
               state->config.allocator, &state->renderer.computePipelineLayout),
           "Couldn't create pipeline layout\n");

  PANIC_IF(
      vkCreateComputePipelines(
          state->context.device, NULL, 1,
          &(VkComputePipelineCreateInfo){
              .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
              .stage = {.sType =
                            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
                        .module = shaderModule,
                        .pName = "main"},
              .layout = state->renderer.computePipelineLayout},
          state->config.allocator, &state->renderer.computePipeline),
      "Couldn't create compute pipeline\n");

  vkDestroyShaderModule(state->context.device, shaderModule,
                        state->config.allocator);
  free(shaderCode);
}
void destroyComputePipeline(State *state) {
  vkDestroyPipeline(state->context.device, state->renderer.computePipeline,
                    state->config.allocator);
  vkDestroyPipelineLayout(state->context.device,
                          state->renderer.computePipelineLayout,
                          state->config.allocator);
  vkDestroyDescriptorSetLayout(state->context.device,
                               state->renderer.descriptorSetLayout,
                               state->config.allocator);
}

void createCommandPool(State *state) {
  PANIC_IF(vkCreateCommandPool(
               state->context.device,
               &(VkCommandPoolCreateInfo){
                   .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                   .queueFamilyIndex = state->context.queueFamily,
                   .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT},
               state->config.allocator, &state->renderer.commandPool),
           "Couldn't create command pool\n");
}
void destroyCommandPool(State *state) {
  vkDestroyCommandPool(state->context.device, state->renderer.commandPool,
                       state->config.allocator);
}

void allocateCommandBuffer(State *state) {
  state->renderer.commandBuffers = malloc(sizeof(VkCommandBuffer));
  PANIC_IF(!state->renderer.commandBuffers,
           "Couldn't allocate command buffers memory\n");

  PANIC_IF(vkAllocateCommandBuffers(
               state->context.device,
               &(VkCommandBufferAllocateInfo){
                   .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                   .commandPool = state->renderer.commandPool,
                   .commandBufferCount = 1,
                   .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
               },
               state->renderer.commandBuffers),
           "Couldn't allocate command buffers\n");
}

void createSyncObjects(State *state) {
  PANIC_IF(
      vkCreateSemaphore(state->context.device,
                        &(VkSemaphoreCreateInfo){
                            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                        },
                        state->config.allocator,
                        &state->renderer.imageAcquiredSemaphore),
      "Couldn't create image acquired semaphore\n");

  uint32_t imageCount = state->window.swapchain.imageCount;
  state->renderer.renderFinishedSemaphores =
      malloc(imageCount * sizeof(VkSemaphore));
  PANIC_IF(!state->renderer.renderFinishedSemaphores,
           "Couldn't allocate render finished semaphores memory\n");

  for (uint32_t i = 0; i < imageCount; i++) {
    PANIC_IF(
        vkCreateSemaphore(state->context.device,
                          &(VkSemaphoreCreateInfo){
                              .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                          },
                          state->config.allocator,
                          &state->renderer.renderFinishedSemaphores[i]),
        "Couldn't create render finished semaphore at index: %u\n", i);
  }

  PANIC_IF(vkCreateFence(state->context.device,
                         &(VkFenceCreateInfo){
                             .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                         },
                         state->config.allocator,
                         &state->renderer.inFlightFence),
           "Couldn't create in flight fence\n");
}
void destroySyncObjects(State *state) {
  vkDestroyFence(state->context.device, state->renderer.inFlightFence,
                 state->config.allocator);

  for (uint32_t i = 0; i < state->window.swapchain.imageCount; i++) {
    vkDestroySemaphore(state->context.device,
                       state->renderer.renderFinishedSemaphores[i],
                       state->config.allocator);
  }
  free(state->renderer.renderFinishedSemaphores);

  vkDestroySemaphore(state->context.device,
                     state->renderer.imageAcquiredSemaphore,
                     state->config.allocator);
}

void createRenderer(State *state) {
  createComputePipeline(state);

  createSphereBuffer(state);
  createDescriptors(state);

  createCommandPool(state);
  allocateCommandBuffer(state);
  createSyncObjects(state);
}
void destroyRenderer(State *state) {

  vkQueueWaitIdle(state->context.queue);

  destroySyncObjects(state);
  destroyCommandPool(state);
  free(state->renderer.commandBuffers);
  destroyDescriptors(state);
  destroySphereBuffer(state);
  destroyComputePipeline(state);
}

void recordCommandBuffer(State *state) {

  VkCommandBuffer commandBuffer = *state->renderer.commandBuffers;
  uint32_t imageIndex = state->window.swapchain.imageAcquiredIndex;
  VkImage image = state->window.swapchain.images[imageIndex];
  VkExtent2D extent = state->window.swapchain.imageExtend;

  PushConstants pushConstants = {
      .cameraData = {(float)extent.width, (float)extent.height,
                     glm_rad(state->config.fov)}, // shader expects radians
      .boundingRadius = state->config.boundingRadius,
      .sphereCount = state->renderer.sphereData.objectCount,
  };
  glm_vec3_copy(state->player.position, pushConstants.playerPosition);
  glm_vec3_copy(state->player.rotation, pushConstants.playerRotation);

  PANIC_IF(vkBeginCommandBuffer(
               commandBuffer,
               &(VkCommandBufferBeginInfo){
                   .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
               }),
           "Couldn't begin command buffer\n");

  VkImageSubresourceRange range = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .levelCount = 1,
      .layerCount = 1,
  };

  vkCmdPipelineBarrier(
      commandBuffer,
      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, // łączy się z wait stage semafora
      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, NULL, 0, NULL, 1,
      &(VkImageMemoryBarrier){
          .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
          .srcAccessMask = 0,
          .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
          .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED, // starą zawartość wyrzucamy
          .newLayout = VK_IMAGE_LAYOUT_GENERAL,
          .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .image = image,
          .subresourceRange = range});
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                    state->renderer.computePipeline);
  vkCmdPushConstants(commandBuffer, state->renderer.computePipelineLayout,
                     VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConstants),
                     &pushConstants);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                          state->renderer.computePipelineLayout, 0, 1,
                          &state->renderer.descriptorSets[imageIndex], 0, NULL);

  vkCmdDispatch(commandBuffer, (extent.width + 7) / 8, (extent.height + 7) / 8,
                1);

  vkCmdPipelineBarrier(
      commandBuffer,
      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, // czekaj, aż compute skończy pisać
      VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1,
      &(VkImageMemoryBarrier){
          .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
          .srcAccessMask =
              VK_ACCESS_SHADER_WRITE_BIT, // zapisy shadera mają być widoczne
          .dstAccessMask = 0,             // present synchronizuje semafor
          .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
          .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
          .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
          .image = image,
          .subresourceRange = range});

  PANIC_IF(vkEndCommandBuffer(commandBuffer), "Couldn't end command buffer\n");
}

void submitCommandBuffer(State *state) {
  vkQueueSubmit(
      state->context.queue, 1,
      &(VkSubmitInfo){
          .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
          .commandBufferCount = 1,
          .pCommandBuffers = state->renderer.commandBuffers,

          .waitSemaphoreCount = 1,
          .pWaitSemaphores = &state->renderer.imageAcquiredSemaphore,

          .signalSemaphoreCount = 1,
          .pSignalSemaphores =
              &state->renderer.renderFinishedSemaphores
                   [state->window.swapchain.imageAcquiredIndex],

          .pWaitDstStageMask =
              (VkPipelineStageFlags[]){VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT},
      },
      state->renderer.inFlightFence);
}