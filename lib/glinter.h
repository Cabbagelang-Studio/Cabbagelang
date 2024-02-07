// Builtin leaf for Windows
#include<stdio.h>
#include"GLFW/glfw3.h"
void glinter_error_callback(int error,const char* description){
    printf("Error: [%d] %s\n");
}

void glinter_init(lenv* e){
    glfwSetErrorCallback(glinter_error_callback);
    
}