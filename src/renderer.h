#pragma once
#include "buffers.h"
#include "cglm/types.h"
#include "structs.h"
#include "utils.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void createGraphicsPipeline(State *state) {
  size_t vertexShaderSize, fragmentShaderSize;
  uint32_t *vertexShaderCode =
      readSpirvFile(SHADER_DIR "shader.vert.spv", &vertexShaderSize);
  uint32_t *fragmentShaderCode =
      readSpirvFile(SHADER_DIR "raymarching.frag.spv", &fragmentShaderSize);

  // Vertex shader
  VkShaderModule vertexShaderModule;
  PANIC_IF(vkCreateShaderModule(
               state->context.device,
               &(VkShaderModuleCreateInfo){
                   .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                   .pCode = vertexShaderCode,
                   .codeSize = vertexShaderSize},
               state->config.allocator, &vertexShaderModule),
           "Couldn't create vertex shader module\n");

  // Fragment shader
  VkShaderModule fragmentShaderModule;
  PANIC_IF(vkCreateShaderModule(
               state->context.device,
               &(VkShaderModuleCreateInfo){
                   .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                   .pCode = fragmentShaderCode,
                   .codeSize = fragmentShaderSize},
               state->config.allocator, &fragmentShaderModule),
           "Couldn't create fragment shader module\n");

  PANIC_IF(vkCreateDescriptorSetLayout(
               state->context.device,
               &(VkDescriptorSetLayoutCreateInfo){
                   .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                   .bindingCount = 1,
                   .pBindings =
                       &(VkDescriptorSetLayoutBinding){
                           .binding = 0,
                           .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                           .descriptorCount = 1,
                           .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT}},
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
                                                  VK_SHADER_STAGE_FRAGMENT_BIT,
                                              .offset = 0,
                                              .size = sizeof(PushConstants)}},
               state->config.allocator,
               &state->renderer.graphicsPipelineLayout),
           "Couldn't create pipeline layout\n");

  const char *shaderEntryFunctionName = "main";
  VkPipelineShaderStageCreateInfo shaderStagesCreateInfo[] = {
      {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .module = vertexShaderModule,
          .stage = VK_SHADER_STAGE_VERTEX_BIT,
          .pName = shaderEntryFunctionName,
      },
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
       .module = fragmentShaderModule,
       .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
       .pName = shaderEntryFunctionName}};

  VkExtent2D imageExtent = state->window.swapchain.imageExtend;

  VkViewport viewports[] = {
      {.width = (float)imageExtent.width, .height = (float)imageExtent.height}};

  VkRect2D scissors[] = {{
      .extent = imageExtent,
  }};

  VkPipelineColorBlendAttachmentState colorBlendAttachmentStates[] = {
      {.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT}};

  PANIC_IF(
      vkCreateGraphicsPipelines(
          state->context.device, NULL, 1,
          &(VkGraphicsPipelineCreateInfo){
              .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
              .pStages = shaderStagesCreateInfo,
              .stageCount = sizeof(shaderStagesCreateInfo) /
                            sizeof(*shaderStagesCreateInfo),

              .pVertexInputState =
                  &(VkPipelineVertexInputStateCreateInfo){
                      .sType =
                          VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                  },

              .pInputAssemblyState =
                  &(VkPipelineInputAssemblyStateCreateInfo){
                      .sType =
                          VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                  },

              .pViewportState =
                  &(VkPipelineViewportStateCreateInfo){
                      .sType =
                          VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                      .pViewports = viewports,
                      .viewportCount = sizeof(viewports) / sizeof(*viewports),
                      .pScissors = scissors,
                      .scissorCount = sizeof(scissors) / sizeof(*scissors),
                  },

              .pRasterizationState =
                  &(VkPipelineRasterizationStateCreateInfo){
                      .sType =
                          VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                      .lineWidth = 1.0f,
                      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
                      .cullMode = VK_CULL_MODE_NONE,
                      .polygonMode = VK_POLYGON_MODE_FILL},

              .pMultisampleState =
                  &(VkPipelineMultisampleStateCreateInfo){
                      .sType =
                          VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT},

              .pColorBlendState =
                  &(VkPipelineColorBlendStateCreateInfo){
                      .sType =
                          VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                      .pAttachments = colorBlendAttachmentStates,
                      .attachmentCount = sizeof(colorBlendAttachmentStates) /
                                         sizeof(*colorBlendAttachmentStates)},

              .layout = state->renderer.graphicsPipelineLayout,
              .renderPass = state->renderer.renderPass,
              .subpass = 0,
          },
          state->config.allocator, &state->renderer.graphicsPipeline),
      "Couldn't create a graphics pipeline\n");

  vkDestroyShaderModule(state->context.device, vertexShaderModule,
                        state->config.allocator);

  vkDestroyShaderModule(state->context.device, fragmentShaderModule,
                        state->config.allocator);
  free(vertexShaderCode);
  free(fragmentShaderCode);
}
void destroyGraphicsPipeline(State *state) {
  vkDestroyPipeline(state->context.device, state->renderer.graphicsPipeline,
                    state->config.allocator);
  vkDestroyPipelineLayout(state->context.device,
                          state->renderer.graphicsPipelineLayout,
                          state->config.allocator);
  vkDestroyDescriptorSetLayout(state->context.device,
                               state->renderer.descriptorSetLayout,
                               state->config.allocator);
}

void createRenderPass(State *state) {
  VkFormat imageFormat = state->window.swapchain.format;

  VkAttachmentReference colorAttachmentReferences[] = {{
      .attachment = 0,
      .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  }};
  VkSubpassDescription subpassDescriptions[] = {{
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .pColorAttachments = colorAttachmentReferences,
      .colorAttachmentCount = 1,
  }};

  VkAttachmentDescription attachmentDescriptions[] = {
      {.format = imageFormat,
       .samples = VK_SAMPLE_COUNT_1_BIT,
       .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
       .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
       .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
       .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
       .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
       .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE}};

  PANIC_IF(
      vkCreateRenderPass(
          state->context.device,
          &(VkRenderPassCreateInfo){
              .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
              .pSubpasses = subpassDescriptions,
              .subpassCount =
                  sizeof(subpassDescriptions) / sizeof(*subpassDescriptions),
              .pAttachments = attachmentDescriptions,
              .attachmentCount = sizeof(attachmentDescriptions) /
                                 sizeof(*attachmentDescriptions),

              .dependencyCount = 1,
              .pDependencies =
                  &(VkSubpassDependency){
                      .srcSubpass = VK_SUBPASS_EXTERNAL,
                      .dstSubpass = 0,

                      .srcAccessMask = 0,
                      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,

                      .srcStageMask =
                          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                      .dstStageMask =
                          VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                  },
          },
          state->config.allocator, &state->renderer.renderPass),
      "Couldn't create render pass\n");
}
void destroyRenderPass(State *state) {
  vkDestroyRenderPass(state->context.device, state->renderer.renderPass,
                      state->config.allocator);
}

void createFramebuffers(State *state) {
  uint32_t framebufferCount = state->window.swapchain.imageCount;
  state->renderer.framebuffers =
      malloc(framebufferCount * sizeof(VkFramebuffer));
  PANIC_IF(!state->renderer.framebuffers,
           "Couldn't allocate framebuffers memory\n");

  uint32_t framebufferWidth = state->window.swapchain.imageExtend.width;
  uint32_t framebufferHeight = state->window.swapchain.imageExtend.height;

  for (int framebufferIndex = 0; framebufferIndex < framebufferCount;
       framebufferIndex++) {

    PANIC_IF(vkCreateFramebuffer(
                 state->context.device,
                 &(VkFramebufferCreateInfo){
                     .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                     .layers = 1,
                     .renderPass = state->renderer.renderPass,
                     .width = framebufferWidth,
                     .height = framebufferHeight,
                     .attachmentCount = 1,
                     .pAttachments =
                         &state->window.swapchain.imageViews[framebufferIndex],
                 },
                 state->config.allocator,
                 &state->renderer.framebuffers[framebufferIndex]),
             "Couldn't create framebuffer at index: %i\n", framebufferIndex);
  }
}
void destroyFramebuffers(State *state) {
  uint32_t framebufferCount = state->window.swapchain.imageCount;
  for (int framebufferIndex = 0; framebufferIndex < framebufferCount;
       framebufferIndex++) {
    vkDestroyFramebuffer(state->context.device,
                         state->renderer.framebuffers[framebufferIndex],
                         state->config.allocator);
  }
  free(state->renderer.framebuffers);
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
  createRenderPass(state);
  createGraphicsPipeline(state);

  createSphereBuffer(state);
  createDescriptors(state);

  createFramebuffers(state);
  createCommandPool(state);
  allocateCommandBuffer(state);
  createSyncObjects(state);
}
void destroyRenderer(State *state) {

  vkQueueWaitIdle(state->context.queue);

  destroySyncObjects(state);
  destroyCommandPool(state);
  free(state->renderer.commandBuffers);
  destroyFramebuffers(state);
  destroyDescriptors(state);
  destroySphereBuffer(state);
  destroyGraphicsPipeline(state);
  destroyRenderPass(state);
}

void recordCommandBuffer(State *state) {

  VkCommandBuffer commandBuffer = *state->renderer.commandBuffers;
  uint32_t imageAcquiredIndex = state->window.swapchain.imageAcquiredIndex;

  VkExtent2D extent = state->window.swapchain.imageExtend;

  PushConstants pushConstants = {
      .player = {0.0f, 0.0f, 0.0f}, // camera position
      .fov = glm_rad(60.0f),        // shader expects radians (cglm helper)
      .resolution = {(float)extent.width, (float)extent.height},
      .boundingRadius = 25.0f, // max ray distance
      .sphereCount = state->renderer.sphereData.objectCount,
  };

  PANIC_IF(vkBeginCommandBuffer(
               commandBuffer,
               &(VkCommandBufferBeginInfo){
                   .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
               }),
           "Couldn't begin command buffer\n");
  /* ofr frame: %i\n",
  state->window.swapchain.imageAcquiredIndex);*/
  VkClearValue clearValues[] = {
      state->config.backgroundColor,
  };

  vkCmdBeginRenderPass(
      commandBuffer,
      &(VkRenderPassBeginInfo){
          .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
          .renderPass = state->renderer.renderPass,
          .framebuffer = state->renderer.framebuffers[imageAcquiredIndex],
          .renderArea =
              (VkRect2D){
                  .extent = state->window.swapchain.imageExtend,
              },
          .pClearValues = clearValues,
          .clearValueCount = 1,
      },
      VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    state->renderer.graphicsPipeline);
  vkCmdPushConstants(commandBuffer, state->renderer.graphicsPipelineLayout,
                     VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants),
                     &pushConstants);
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          state->renderer.graphicsPipelineLayout, 0, 1,
                          &state->renderer.descriptorSet, 0, NULL);
  vkCmdDraw(commandBuffer, 3, 1, 0, 0);
  vkCmdEndRenderPass(commandBuffer);

  PANIC_IF(vkEndCommandBuffer(commandBuffer), "Couldn't end command buffer\n");
}

void submitCommandBuffer(State *state) {
  vkQueueSubmit(state->context.queue, 1,
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
                        (VkPipelineStageFlags[]){
                            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        }},

                state->renderer.inFlightFence);
}