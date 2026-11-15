#pragma once

#include "headers.h"

typedef struct {
  const char *applicationName;
  const char *engineName;
  const char *windowTitle;

  int windowWidth, windowHeight;
  bool fullscreen;

  uint32_t apiVersion;
  VkAllocationCallbacks *allocator;
  VkComponentMapping swapchainComponentsMapping;
  uint32_t swapchainBuffering;

  VkClearValue backgroundColor;

} Config;
typedef struct {
  uint32_t imageCount;
  VkSwapchainKHR handle;
  VkImage *images;
  VkImageView *imageViews;
  VkFormat format;
  VkColorSpaceKHR colorSpace;
  VkExtent2D imageExtend;
  uint32_t imageAcquiredIndex;

} Swapchain;
typedef struct {
  GLFWwindow *handle;
  VkSurfaceKHR surface;
  Swapchain swapchain;

} Window;
typedef struct {
  uint32_t queueFamily;
  VkInstance instance;
  VkPhysicalDevice physicalDevice;
  VkDevice device;
  VkQueue queue;

} Context;

// A host-visible, persistently mapped GPU buffer holding `objectCount` items
typedef struct {
  VkBuffer handle;
  VkDeviceMemory memory;
  void *mapped;
  uint32_t objectCount;
} BufferData;
typedef struct {
  VkPipeline graphicsPipeline;
  VkPipelineLayout graphicsPipelineLayout;
  VkRenderPass renderPass;
  VkFramebuffer *framebuffers;
  VkCommandPool commandPool;
  VkCommandBuffer *commandBuffers;
  VkSemaphore imageAcquiredSemaphore;
  VkSemaphore *renderFinishedSemaphores; // one per swapchain image
  VkFence inFlightFence;

  // Describes the shader's resource interface, shared by all buffers
  VkDescriptorSetLayout descriptorSetLayout;
  VkDescriptorPool descriptorPool;
  VkDescriptorSet descriptorSet;

  BufferData sphereData;
} Renderer;
typedef struct {
  Config config;
  Window window;
  Context context;
  Renderer renderer;

} State;

// Must match the push_constant block in raymarching.frag
typedef struct {
  float player[3];      // offset 0
  float fov;            // offset 12 (radians)
  float resolution[2];  // offset 16
  float boundingRadius; // offset 24
  uint32_t sphereCount; // offset 28
} PushConstants;        // 32 bytes

// Must match the SphereBuffer element in raymarching.frag (vec4)
typedef struct {
  float center[3];
  float radius;
} Sphere;

enum swapchainBuffering {
  SWAPCHAIN_DOUBLE_BUFFERING = 2,
  SWAPCHAIN_TRIPLE_BUFFERING = 3,
};

enum bodyType {
  SPHERE = 0,
  CUBE = 1,
};