#pragma once

#include "cglm/types.h"
#include "headers.h"
#include <stddef.h>

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

  float fov;
  float boundingRadius;

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
  VkPipeline computePipeline;
  VkPipelineLayout computePipelineLayout;
  VkCommandPool commandPool;
  VkCommandBuffer *commandBuffers;
  VkSemaphore imageAcquiredSemaphore;
  VkSemaphore *renderFinishedSemaphores;
  VkFence inFlightFence;

  VkDescriptorSetLayout descriptorSetLayout;
  VkDescriptorPool descriptorPool;
  VkDescriptorSet *descriptorSets; // jeden na obraz swapchaina

  BufferData sphereData;
} Renderer;

typedef struct {
  vec3 position;
  vec3 rotation;
  vec3 up;
} Player;

typedef struct {
  Config config;
  Window window;
  Context context;
  Renderer renderer;
  Player player;
} State;

// Must match the push_constant block in raymarching.frag (std430: vec3 aligns
// to 16 bytes, so each vec3 that is followed by another vec3 needs padding)
typedef struct {
  vec3 playerPosition;  // offset 0
  float _pad0;          // offset 12
  vec3 playerRotation;  // offset 16 (radians: pitch, yaw, roll)
  float _pad1;          // offset 28
  vec3 cameraData;      // offset 32 (xy = resolution, z = fov in radians)
  float boundingRadius; // offset 44
  uint32_t sphereCount; // offset 48
} PushConstants;        // 52 bytes

_Static_assert(offsetof(PushConstants, playerRotation) == 16, "PC layout");
_Static_assert(offsetof(PushConstants, cameraData) == 32, "PC layout");
_Static_assert(offsetof(PushConstants, boundingRadius) == 44, "PC layout");
_Static_assert(offsetof(PushConstants, sphereCount) == 48, "PC layout");

typedef struct {
  float center[3];
  float radius;
} Sphere;

enum swapchainBuffering {
  SWAPCHAIN_DOUBLE_BUFFERING = 2,
  SWAPCHAIN_TRIPLE_BUFFERING = 3,
};
