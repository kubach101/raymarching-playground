#pragma once

#include "GLFW/glfw3.h"
#include "context.h"
#include "input.h"
#include "renderer.h"
#include "structs.h"
#include "utils.h"
#include "window.h"

void init(State *state) {
  setupErrorHandeling();
  logInfo();

  glfwInit();
  createContext(state);
  createWindow(state);
  createRenderer(state);
}
void loop(State *state) {
  glfwSetTime(0.0);
  float dt;
  while (!glfwWindowShouldClose(state->window.handle)) {
    dt = glfwGetTime();
    glfwSetTime(0.0);
    glfwPollEvents();

    handleInput(state, dt);
    acquireSwapchainImage(state);
    recordCommandBuffer(state);
    submitCommandBuffer(state);
    presentSwapchainImage(state);
  }
}
void cleanup(State *state) {
  destroyRenderer(state);
  destroyWindow(state);
  destroyContext(state);
}
