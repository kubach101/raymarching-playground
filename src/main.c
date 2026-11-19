#include "cglm/util.h"
#include "toolkit.h"
#include <stdbool.h>

int main(void) {
  State state = {
      .config =
          {
              .windowTitle = "raymarching playground",
              .windowWidth = 1200,
              .windowHeight = 800,
              .fullscreen = false,
              .apiVersion = VK_API_VERSION_1_3,
              .applicationName = "raymarching playground",
              .engineName = "ray spark",
              .swapchainComponentsMapping = (VkComponentMapping){},
              .swapchainBuffering = SWAPCHAIN_TRIPLE_BUFFERING,
              .backgroundColor = {{0.03f, 0.03f, 0.03f, 1.0f}},
              .fov = 45.0f,
              .boundingRadius = 100.0f,
          },

  };

  init(&state);
  loop(&state);
  cleanup(&state);

  return EXIT_SUCCESS;
}
