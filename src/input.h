#pragma once

#include "GLFW/glfw3.h"
#include "cglm/util.h"
#include "cglm/vec3.h"
#include "structs.h"
#include <math.h>

void handleInput(State *state, float dt) {
  GLFWwindow *w = state->window.handle;
  Player *p = &state->player;

  float speed = 3.0f;              // units per second
  float rotSpeed = glm_rad(60.0f); // radians per second

  // Rotation (x = pitch, y = yaw, z = roll), matches cameraRotation() in shader
  if (glfwGetKey(w, GLFW_KEY_UP) == GLFW_PRESS)
    p->rotation[0] += rotSpeed * dt;
  if (glfwGetKey(w, GLFW_KEY_DOWN) == GLFW_PRESS)
    p->rotation[0] -= rotSpeed * dt;
  if (glfwGetKey(w, GLFW_KEY_RIGHT) == GLFW_PRESS)
    p->rotation[1] += rotSpeed * dt;
  if (glfwGetKey(w, GLFW_KEY_LEFT) == GLFW_PRESS)
    p->rotation[1] -= rotSpeed * dt;
  p->rotation[0] = glm_clamp(p->rotation[0], glm_rad(-89.0f), glm_rad(89.0f));

  // Movement on the horizontal plane (yaw only, so looking up doesn't fly you
  // up). Derived from shader: yaw * (0,0,1) and yaw * (1,0,0)
  float yaw = p->rotation[1];
  vec3 forward = {sinf(yaw), 0.0f, cosf(yaw)};
  vec3 right = {cosf(yaw), 0.0f, -sinf(yaw)};
  glm_vec3_scale(forward, speed * dt, forward);
  glm_vec3_scale(right, speed * dt, right);

  if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS)
    glm_vec3_add(p->position, forward, p->position);
  if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS)
    glm_vec3_sub(p->position, forward, p->position);
  if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS)
    glm_vec3_add(p->position, right, p->position);
  if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS)
    glm_vec3_sub(p->position, right, p->position);
}
