#pragma once

#include "headers.h"

#define PANIC_IF(ERROR, FORMAT, ...)                                           \
  do {                                                                         \
    int panic_err_ = (ERROR);                                                  \
    if (panic_err_) {                                                          \
      fprintf(stderr,                                                          \
              "\033[1;31m%s -> %s -> %i -> Error(%i): " FORMAT "\033[0m\n",    \
              __FILE__, __func__, __LINE__, panic_err_, ##__VA_ARGS__);        \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)

// Reads a whole binary file (e.g. SPIR-V). Caller frees the returned buffer.
uint32_t *readSpirvFile(const char *path, size_t *outSize) {
  FILE *file = fopen(path, "rb");
  PANIC_IF(!file, "Couldn't open file: %s\n", path);
  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  fseek(file, 0, SEEK_SET);
  PANIC_IF(size <= 0 || size % 4 != 0, "Invalid SPIR-V file: %s\n", path);

  uint32_t *code = malloc((size_t)size);
  PANIC_IF(!code, "Couldn't allocate memory for file: %s\n", path);
  PANIC_IF(fread(code, 1, (size_t)size, file) != (size_t)size,
           "Couldn't read file: %s\n", path);
  fclose(file);

  *outSize = (size_t)size;
  return code;
}

uint32_t clamp(uint32_t val, uint32_t min, uint32_t max) {
  if (val < min)
    return min;
  else if (val > max)
    return max;
  else
    return val;
}

void logInfo() {
  uint32_t instanceApiVersion;
  PANIC_IF(vkEnumerateInstanceVersion(&instanceApiVersion),
           "Couldn't enumerate VkInstance\n");
  uint32_t apiVersionVariant = VK_API_VERSION_VARIANT(instanceApiVersion);
  uint32_t apiVersionMajor = VK_API_VERSION_MAJOR(instanceApiVersion);
  uint32_t apiVersionMinor = VK_API_VERSION_MINOR(instanceApiVersion);
  uint32_t apiVersionPatch = VK_API_VERSION_PATCH(instanceApiVersion);
  printf("Vulkan API: %i.%i.%i.%i\n", apiVersionVariant, apiVersionMajor,
         apiVersionMinor, apiVersionPatch);
  printf("GLFW: %s\n", glfwGetVersionString());
}

void glfwErrorCallback(int error, const char *description) {
  PANIC_IF(error, "GLFW Error: %s\n", description);
}
void exitCallback() { glfwTerminate(); }
void setupErrorHandeling() {
  glfwSetErrorCallback(glfwErrorCallback);
  atexit(exitCallback);
}
