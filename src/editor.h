#ifndef EDITOR_H
#define EDITOR_H

#include <GLFW/glfw3.h>

void editor_draw();
void drop_callback(GLFWwindow* window, int count, const char** paths);

#endif