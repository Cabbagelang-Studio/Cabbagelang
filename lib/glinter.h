// Builtin leaf for Windows
#include<stdio.h>
#include<malloc.h>
#include"GLFW/glfw3.h"
#define STB_IMAGE_IMPLEMENTATION
#include"stb_image.h"

#define GLFW_INIT_CHECK() if(!GLFW_INITIALIZED){lval_del(a);return lval_err("GLFW is not initialized");}

GLFWwindow** windows_list=NULL;
int next_window_index=1;
unsigned char** images_list=NULL;
int image_index=0;

int GLFW_INITIALIZED=0;

void glinter_error_callback(int error,const char* description){
    printf("Error: [%d] %s\n",error,description);
}

lval* builtin_gl_init(lenv*e,lval* a){
    LASSERT_NUM("gl.init",a,1);
    LASSERT_TYPE("gl.init",a,0,LVAL_SEXPR);
    windows_list=malloc(1*sizeof(GLFWwindow*));
    windows_list[0]=NULL;
    images_list=malloc(1*sizeof(unsigned char*));
    int return_code=glfwInit();
    if(return_code){
        GLFW_INITIALIZED=1;
    }
    return lval_num(return_code);
}

lval* builtin_gl_terminate(lenv*e,lval* a){
    LASSERT_NUM("gl.terminate",a,1);
    LASSERT_TYPE("gl.terminate",a,0,LVAL_SEXPR);
    GLFW_INITIALIZED=0;
    glfwTerminate();
    for(int i=0;i<image_index;i++){
        stbi_image_free(images_list[i]);
    }
    return lval_sexpr();
}

lval* builtin_gl_create_window(lenv*e,lval* a){
    LASSERT_NUM("gl.create_window",a,4);
    LASSERT_TYPE("gl.create_window",a,0,LVAL_NUM);
    LASSERT_TYPE("gl.create_window",a,1,LVAL_NUM);
    LASSERT_TYPE("gl.create_window",a,2,LVAL_STR);
    LASSERT_TYPE("gl.create_window",a,3,LVAL_NUM);
    GLFW_INIT_CHECK();
    int width=a->cell[0]->num;
    int height=a->cell[1]->num;
    char* title=a->cell[2]->str;
    int share_id=a->cell[3]->num;
    if(share_id>=next_window_index){
        lval_del(a);
        return lval_err("Invalid GLFW window: %d",share_id);
    }

    GLFWwindow* window=glfwCreateWindow(width,height,title,NULL,windows_list[share_id]);
    if(!window){
        lval_del(a);
        return lval_err("Failed to create window.");
    }
    windows_list=realloc(windows_list,(++next_window_index)*sizeof(GLFWwindow*));
    windows_list[next_window_index-1]=window;
    lval_del(a);
    return lval_num(next_window_index-1);
}

lval* builtin_gl_destroy_window(lenv*e,lval* a){
    LASSERT_NUM("gl.destroy_window",a,1);
    LASSERT_TYPE("gl.destroy_window",a,0,LVAL_NUM);
    GLFW_INIT_CHECK();
    int window_id=a->cell[0]->num;
    if(window_id>=next_window_index){
        lval_del(a);
        return lval_err("Invalid GLFW window: %d",window_id);
    }
    glfwDestroyWindow(windows_list[window_id]);
    return lval_sexpr();
}

lval* builtin_gl_window_hint(lenv* e,lval* a){
    LASSERT_NUM("gl.window_hint",a,2);
    LASSERT_TYPE("gl.window_hint",a,0,LVAL_NUM);
    LASSERT_TYPE("gl.window_hint",a,1,LVAL_NUM);
    GLFW_INIT_CHECK();
    int hint=a->cell[0]->num;
    int value=a->cell[1]->num;
    glfwWindowHint(hint,value);
    return lval_sexpr();
}

lval* builtin_gl_make_context_current(lenv*e,lval* a){
    LASSERT_NUM("gl.make_context_current",a,1);
    LASSERT_TYPE("gl.make_context_current",a,0,LVAL_NUM);
    GLFW_INIT_CHECK();
    int window_id=a->cell[0]->num;
    if(window_id>=next_window_index){
        lval_del(a);
        return lval_err("Invalid GLFW window: %d",window_id);
    }
    glfwMakeContextCurrent(windows_list[window_id]);
    return lval_sexpr();
}

lval* builtin_gl_window_should_close(lenv*e,lval* a){
    LASSERT_NUM("gl.window_should_close",a,1);
    LASSERT_TYPE("gl.window_should_close",a,0,LVAL_NUM);
    GLFW_INIT_CHECK();
    int window_id=a->cell[0]->num;
    if(window_id>=next_window_index){
        lval_del(a);
        return lval_err("Invalid GLFW window: %d",window_id);
    }
    int result=glfwWindowShouldClose(windows_list[window_id]);
    return lval_num(result);
}

lval* builtin_gl_swap_buffers(lenv*e,lval* a){
    LASSERT_NUM("gl.swap_buffers",a,1);
    LASSERT_TYPE("gl.swap_buffers",a,0,LVAL_NUM);
    GLFW_INIT_CHECK();
    int window_id=a->cell[0]->num;
    if(window_id>=next_window_index){
        lval_del(a);
        return lval_err("Invalid GLFW window: %d",window_id);
    }
    glfwSwapBuffers(windows_list[window_id]);
    return lval_sexpr();
}

lval* builtin_gl_swap_interval(lenv*e,lval* a){
    LASSERT_NUM("gl.swap_interval",a,1);
    LASSERT_TYPE("gl.swap_interval",a,0,LVAL_NUM);
    GLFW_INIT_CHECK();
    int interval=a->cell[0]->num;
    glfwSwapInterval(interval);
    return lval_sexpr();
}

lval* builtin_gl_poll_events(lenv*e,lval* a){
    LASSERT_NUM("gl.poll_events",a,1);
    LASSERT_TYPE("gl.poll_events",a,0,LVAL_SEXPR);
    GLFW_INIT_CHECK();
    glfwPollEvents();
    return lval_sexpr();
}

lval* builtin_gl_get_key(lenv*e,lval* a){
    LASSERT_NUM("gl.get_key",a,2);
    LASSERT_TYPE("gl.get_key",a,0,LVAL_NUM);
    LASSERT_TYPE("gl.get_key",a,1,LVAL_NUM);
    GLFW_INIT_CHECK();
    int window_id=a->cell[0]->num;
    int key_id=a->cell[1]->num;
    if(window_id>=next_window_index){
        lval_del(a);
        return lval_err("Invalid GLFW window: %d",window_id);
    }int keyResult=glfwGetKey(windows_list[window_id],key_id);
    lval_del(a);
    return lval_num(keyResult);
}

lval* builtin_gl_set_window_should_close(lenv*e,lval* a){
    LASSERT_NUM("gl.set_window_should_close",a,2);
    LASSERT_TYPE("gl.set_window_should_close",a,0,LVAL_NUM);
    LASSERT_TYPE("gl.set_window_should_close",a,1,LVAL_NUM);
    GLFW_INIT_CHECK();
    int window_id=a->cell[0]->num;
    int close=a->cell[1]->num;
    if(window_id>=next_window_index){
        lval_del(a);
        return lval_err("Invalid GLFW window: %d",window_id);
    }
    glfwSetWindowShouldClose(windows_list[window_id],close);
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_gl_get_time(lenv*e,lval* a){
    LASSERT_NUM("gl.get_time",a,1);
    LASSERT_TYPE("gl.get_time",a,0,LVAL_SEXPR);
    GLFW_INIT_CHECK();
    double time=glfwGetTime();
    return lval_num(time);
}

lval* builtin_gl_set_time(lenv*e,lval* a){
    LASSERT_NUM("gl.set_time",a,1);
    LASSERT_TYPE("gl.set_time",a,0,LVAL_NUM);
    GLFW_INIT_CHECK();
    glfwSetTime(a->cell[0]->num);
    return lval_sexpr();
}

lval* builtin_gl_get_window_size(lenv*e,lval* a){
    LASSERT_NUM("gl.get_window_size",a,1);
    LASSERT_TYPE("gl.get_window_size",a,0,LVAL_NUM);
    GLFW_INIT_CHECK();
    int window_id=a->cell[0]->num;
    if(window_id>=next_window_index){
        lval_del(a);
        return lval_err("Invalid GLFW window: %d",window_id);
    }
    int width,height;
    glfwGetWindowSize(windows_list[window_id],&width,&height);
    lval* result=lval_sexpr();
    result=lval_add(result,lval_num(width));
    result=lval_add(result,lval_num(height));
    return result;
}

lval* builtin_gl_load_image(lenv*e,lval* a){
    LASSERT_NUM("gl.load_image",a,1);
    LASSERT_TYPE("gl.load_image",a,0,LVAL_STR);
    GLFW_INIT_CHECK();
    int img_width, img_height, img_channels;
    char* _image_name=a->cell[0]->str;
    char* image_name=a->cell[0]->str;
    unsigned char* img_data = stbi_load(_image_name, &img_width, &img_height, &img_channels, 0);
    if(!img_data){
        lval_del(a);
        return lval_err("Failed to open image: %s",image_name);
    }
    images_list[image_index]=img_data;
    image_index++;
    images_list=realloc(images_list,(image_index+1)*sizeof(unsigned char*));
    return lval_num(image_index-1);
}

void glinter_init(lenv* e){
    glfwSetErrorCallback(glinter_error_callback);

    lval_constant(e,"gl.GLFW_TRUE",lval_num(GLFW_TRUE));
    lval_constant(e,"gl.GLFW_FALSE",lval_num(GLFW_FALSE));

    lval_constant(e,"gl.INT_MAX",lval_num(INT_MAX));
    lval_constant(e,"gl.INT_MIN",lval_num(INT_MIN));
    lval_constant(e,"gl.GLFW_DONT_CARE",lval_num(GLFW_DONT_CARE));

    lval_constant(e,"gl.GLFW_VERSION_MAJOR",lval_num(GLFW_VERSION_MAJOR));
    lval_constant(e,"gl.GLFW_VERSION_MINOR",lval_num(GLFW_VERSION_MINOR));
    lval_constant(e,"gl.GLFW_VERSION_REVISION",lval_num(GLFW_VERSION_REVISION));

    lval_constant(e,"gl.GLFW_RELEASE",lval_num(GLFW_RELEASE));
    lval_constant(e,"gl.GLFW_PRESS",lval_num(GLFW_PRESS));
    lval_constant(e,"gl.GLFW_REPEAT",lval_num(GLFW_REPEAT));

    lval_constant(e,"gl.GLFW_HAT_CENTERED",lval_num(GLFW_HAT_CENTERED));
    lval_constant(e,"gl.GLFW_HAT_UP",lval_num(GLFW_HAT_UP));
    lval_constant(e,"gl.GLFW_HAT_RIGHT",lval_num(GLFW_HAT_RIGHT));
    lval_constant(e,"gl.GLFW_HAT_DOWN",lval_num(GLFW_HAT_DOWN));
    lval_constant(e,"gl.GLFW_HAT_LEFT",lval_num(GLFW_HAT_LEFT));
    lval_constant(e,"gl.GLFW_HAT_RIGHT_UP",lval_num(GLFW_HAT_RIGHT_UP));
    lval_constant(e,"gl.GLFW_HAT_RIGHT_DOWN",lval_num(GLFW_HAT_RIGHT_DOWN));
    lval_constant(e,"gl.GLFW_HAT_LEFT_UP",lval_num(GLFW_HAT_LEFT_UP));
    lval_constant(e,"gl.GLFW_HAT_LEFT_DOWN",lval_num(GLFW_HAT_LEFT_DOWN));

    lval_constant(e,"gl.GLFW_STEREO",lval_num(GLFW_STEREO));
    lval_constant(e,"gl.GLFW_DOUBLEBUFFER",lval_num(GLFW_DOUBLEBUFFER));
    lval_constant(e,"gl.GLFW_CLIENT_API",lval_num(GLFW_CLIENT_API));
    lval_constant(e,"gl.GLFW_CONTEXT_CREATION_API",lval_num(GLFW_CONTEXT_CREATION_API));
    lval_constant(e,"gl.GLFW_OPENGL_FORWARD_COMPAT",lval_num(GLFW_OPENGL_FORWARD_COMPAT));
    lval_constant(e,"gl.GLFW_OPENGL_PROFILE",lval_num(GLFW_OPENGL_PROFILE));

    lval_constant(e,"gl.GLFW_OPENGL_API",lval_num(GLFW_OPENGL_API));
    lval_constant(e,"gl.GLFW_OPENGL_ES_API",lval_num(GLFW_OPENGL_ES_API));
    lval_constant(e,"gl.GLFW_NO_API",lval_num(GLFW_NO_API));
    lval_constant(e,"gl.GLFW_NATIVE_CONTEXT_API",lval_num(GLFW_NATIVE_CONTEXT_API));
    lval_constant(e,"gl.GLFW_EGL_CONTEXT_API",lval_num(GLFW_EGL_CONTEXT_API));
    lval_constant(e,"gl.GLFW_OSMESA_CONTEXT_API",lval_num(GLFW_OSMESA_CONTEXT_API));
    lval_constant(e,"gl.GLFW_NO_ROBUSTNESS",lval_num(GLFW_NO_ROBUSTNESS));
    lval_constant(e,"gl.GLFW_NO_RESET_NOTIFICATION",lval_num(GLFW_NO_RESET_NOTIFICATION));
    lval_constant(e,"gl.GLFW_LOSE_CONTEXT_ON_RESET",lval_num(GLFW_LOSE_CONTEXT_ON_RESET));
    lval_constant(e,"gl.GLFW_ANY_RELEASE_BEHAVIOR",lval_num(GLFW_ANY_RELEASE_BEHAVIOR));
    lval_constant(e,"gl.GLFW_RELEASE_BEHAVIOR_FLUSH",lval_num(GLFW_RELEASE_BEHAVIOR_FLUSH));
    lval_constant(e,"gl.GLFW_RELEASE_BEHAVIOR_NONE",lval_num(GLFW_RELEASE_BEHAVIOR_NONE));
    lval_constant(e,"gl.GLFW_OPENGL_ANY_PROFILE",lval_num(GLFW_OPENGL_ANY_PROFILE));
    lval_constant(e,"gl.GLFW_OPENGL_COMPAT_PROFILE",lval_num(GLFW_OPENGL_COMPAT_PROFILE));
    lval_constant(e,"gl.GLFW_OPENGL_CORE_PROFILE",lval_num(GLFW_OPENGL_CORE_PROFILE));

    lval_constant(e,"gl.GLFW_RESIZABLE",lval_num(GLFW_RESIZABLE));
    lval_constant(e,"gl.GLFW_VISIBLE",lval_num(GLFW_VISIBLE));
    lval_constant(e,"gl.GLFW_DECORATED",lval_num(GLFW_DECORATED));
    lval_constant(e,"gl.GLFW_FOCUSED",lval_num(GLFW_FOCUSED));
    lval_constant(e,"gl.GLFW_AUTO_ICONIFY",lval_num(GLFW_AUTO_ICONIFY));
    lval_constant(e,"gl.GLFW_FLOATING",lval_num(GLFW_FLOATING));
    lval_constant(e,"gl.GLFW_MAXIMIZED",lval_num(GLFW_MAXIMIZED));
    lval_constant(e,"gl.GLFW_CENTER_CURSOR",lval_num(GLFW_CENTER_CURSOR));
    lval_constant(e,"gl.GLFW_TRANSPARENT_FRAMEBUFFER",lval_num(GLFW_TRANSPARENT_FRAMEBUFFER));
    lval_constant(e,"gl.GLFW_FOCUS_ON_SHOW",lval_num(GLFW_FOCUS_ON_SHOW));
    lval_constant(e,"gl.GLFW_SCALE_TO_MONITOR",lval_num(GLFW_SCALE_TO_MONITOR));
    lval_constant(e,"gl.GLFW_RED_BITS",lval_num(GLFW_RED_BITS));
    lval_constant(e,"gl.GLFW_GREEN_BITS",lval_num(GLFW_GREEN_BITS));
    lval_constant(e,"gl.GLFW_BLUE_BITS",lval_num(GLFW_BLUE_BITS));
    lval_constant(e,"gl.GLFW_ALPHA_BITS",lval_num(GLFW_ALPHA_BITS));
    lval_constant(e,"gl.GLFW_DEPTH_BITS",lval_num(GLFW_DEPTH_BITS));
    lval_constant(e,"gl.GLFW_STENCIL_BITS",lval_num(GLFW_STENCIL_BITS));
    lval_constant(e,"gl.GLFW_ACCUM_RED_BITS",lval_num(GLFW_ACCUM_RED_BITS));
    lval_constant(e,"gl.GLFW_ACCUM_GREEN_BITS",lval_num(GLFW_ACCUM_GREEN_BITS));
    lval_constant(e,"gl.GLFW_ACCUM_BLUE_BITS",lval_num(GLFW_ACCUM_BLUE_BITS));
    lval_constant(e,"gl.GLFW_ACCUM_ALPHA_BITS",lval_num(GLFW_ACCUM_ALPHA_BITS));
    lval_constant(e,"gl.GLFW_AUX_BUFFERS",lval_num(GLFW_AUX_BUFFERS));
    lval_constant(e,"gl.GLFW_SAMPLES",lval_num(GLFW_SAMPLES));
    lval_constant(e,"gl.GLFW_REFRESH_RATE",lval_num(GLFW_REFRESH_RATE));
    lval_constant(e,"gl.GLFW_SRGB_CAPABLE",lval_num(GLFW_SRGB_CAPABLE));
    lval_constant(e,"gl.GLFW_CONTEXT_VERSION_MAJOR",lval_num(GLFW_CONTEXT_VERSION_MAJOR));
    lval_constant(e,"gl.GLFW_CONTEXT_VERSION_MINOR",lval_num(GLFW_CONTEXT_VERSION_MINOR));
    lval_constant(e,"gl.GLFW_CONTEXT_ROBUSTNESS",lval_num(GLFW_CONTEXT_ROBUSTNESS));
    lval_constant(e,"gl.GLFW_CONTEXT_RELEASE_BEHAVIOR",lval_num(GLFW_CONTEXT_RELEASE_BEHAVIOR));
    lval_constant(e,"gl.GLFW_OPENGL_DEBUG_CONTEXT",lval_num(GLFW_OPENGL_DEBUG_CONTEXT));
    lval_constant(e,"gl.GLFW_COCOA_RETINA_FRAMEBUFFER",lval_num(GLFW_COCOA_RETINA_FRAMEBUFFER));
    lval_constant(e,"gl.GLFW_COCOA_FRAME_NAME",lval_num(GLFW_COCOA_FRAME_NAME));
    lval_constant(e,"gl.GLFW_COCOA_GRAPHICS_SWITCHING",lval_num(GLFW_COCOA_GRAPHICS_SWITCHING));
    lval_constant(e,"gl.GLFW_X11_CLASS_NAME",lval_num(GLFW_X11_CLASS_NAME));
    lval_constant(e,"gl.GLFW_X11_INSTANCE_NAME",lval_num(GLFW_X11_INSTANCE_NAME));

    lval_constant(e,"gl.GLFW_KEY_SPACE",lval_num(GLFW_KEY_SPACE));
    lval_constant(e,"gl.GLFW_KEY_APOSTROPHE",lval_num(GLFW_KEY_APOSTROPHE));
    lval_constant(e,"gl.GLFW_KEY_COMMA",lval_num(GLFW_KEY_COMMA));
    lval_constant(e,"gl.GLFW_KEY_MINUS",lval_num(GLFW_KEY_MINUS));
    lval_constant(e,"gl.GLFW_KEY_PERIOD",lval_num(GLFW_KEY_PERIOD));
    lval_constant(e,"gl.GLFW_KEY_SLASH",lval_num(GLFW_KEY_SLASH));
    lval_constant(e,"gl.GLFW_KEY_0",lval_num(GLFW_KEY_0));
    lval_constant(e,"gl.GLFW_KEY_1",lval_num(GLFW_KEY_1));
    lval_constant(e,"gl.GLFW_KEY_2",lval_num(GLFW_KEY_2));
    lval_constant(e,"gl.GLFW_KEY_3",lval_num(GLFW_KEY_3));
    lval_constant(e,"gl.GLFW_KEY_4",lval_num(GLFW_KEY_4));
    lval_constant(e,"gl.GLFW_KEY_5",lval_num(GLFW_KEY_5));
    lval_constant(e,"gl.GLFW_KEY_6",lval_num(GLFW_KEY_6));
    lval_constant(e,"gl.GLFW_KEY_7",lval_num(GLFW_KEY_7));
    lval_constant(e,"gl.GLFW_KEY_8",lval_num(GLFW_KEY_8));
    lval_constant(e,"gl.GLFW_KEY_9",lval_num(GLFW_KEY_9));
    lval_constant(e,"gl.GLFW_KEY_SEMICOLON",lval_num(GLFW_KEY_SEMICOLON));
    lval_constant(e,"gl.GLFW_KEY_EQUAL",lval_num(GLFW_KEY_EQUAL));
    lval_constant(e,"gl.GLFW_KEY_A",lval_num(GLFW_KEY_A));
    lval_constant(e,"gl.GLFW_KEY_B",lval_num(GLFW_KEY_B));
    lval_constant(e,"gl.GLFW_KEY_C",lval_num(GLFW_KEY_C));
    lval_constant(e,"gl.GLFW_KEY_D",lval_num(GLFW_KEY_D));
    lval_constant(e,"gl.GLFW_KEY_E",lval_num(GLFW_KEY_E));
    lval_constant(e,"gl.GLFW_KEY_F",lval_num(GLFW_KEY_F));
    lval_constant(e,"gl.GLFW_KEY_G",lval_num(GLFW_KEY_G));
    lval_constant(e,"gl.GLFW_KEY_H",lval_num(GLFW_KEY_H));
    lval_constant(e,"gl.GLFW_KEY_I",lval_num(GLFW_KEY_I));
    lval_constant(e,"gl.GLFW_KEY_J",lval_num(GLFW_KEY_J));
    lval_constant(e,"gl.GLFW_KEY_K",lval_num(GLFW_KEY_K));
    lval_constant(e,"gl.GLFW_KEY_L",lval_num(GLFW_KEY_L));
    lval_constant(e,"gl.GLFW_KEY_M",lval_num(GLFW_KEY_M));
    lval_constant(e,"gl.GLFW_KEY_N",lval_num(GLFW_KEY_N));
    lval_constant(e,"gl.GLFW_KEY_O",lval_num(GLFW_KEY_O));
    lval_constant(e,"gl.GLFW_KEY_P",lval_num(GLFW_KEY_P));
    lval_constant(e,"gl.GLFW_KEY_Q",lval_num(GLFW_KEY_Q));
    lval_constant(e,"gl.GLFW_KEY_R",lval_num(GLFW_KEY_R));
    lval_constant(e,"gl.GLFW_KEY_S",lval_num(GLFW_KEY_S));
    lval_constant(e,"gl.GLFW_KEY_T",lval_num(GLFW_KEY_T));
    lval_constant(e,"gl.GLFW_KEY_U",lval_num(GLFW_KEY_U));
    lval_constant(e,"gl.GLFW_KEY_V",lval_num(GLFW_KEY_V));
    lval_constant(e,"gl.GLFW_KEY_W",lval_num(GLFW_KEY_W));
    lval_constant(e,"gl.GLFW_KEY_X",lval_num(GLFW_KEY_X));
    lval_constant(e,"gl.GLFW_KEY_Y",lval_num(GLFW_KEY_Y));
    lval_constant(e,"gl.GLFW_KEY_Z",lval_num(GLFW_KEY_Z));
    lval_constant(e,"gl.GLFW_KEY_LEFT_BRACKET",lval_num(GLFW_KEY_LEFT_BRACKET));
    lval_constant(e,"gl.GLFW_KEY_BACKSLASH",lval_num(GLFW_KEY_BACKSLASH));
    lval_constant(e,"gl.GLFW_KEY_RIGHT_BRACKET",lval_num(GLFW_KEY_RIGHT_BRACKET));
    lval_constant(e,"gl.GLFW_KEY_GRAVE_ACCENT",lval_num(GLFW_KEY_GRAVE_ACCENT));
    lval_constant(e,"gl.GLFW_KEY_WORLD_1",lval_num(GLFW_KEY_WORLD_1));
    lval_constant(e,"gl.GLFW_KEY_WORLD_2",lval_num(GLFW_KEY_WORLD_2));
    lval_constant(e,"gl.GLFW_KEY_ESCAPE",lval_num(GLFW_KEY_ESCAPE));
    lval_constant(e,"gl.GLFW_KEY_ENTER",lval_num(GLFW_KEY_ENTER));
    lval_constant(e,"gl.GLFW_KEY_TAB",lval_num(GLFW_KEY_TAB));
    lval_constant(e,"gl.GLFW_KEY_BACKSPACE",lval_num(GLFW_KEY_BACKSPACE));
    lval_constant(e,"gl.GLFW_KEY_INSERT",lval_num(GLFW_KEY_INSERT));
    lval_constant(e,"gl.GLFW_KEY_DELETE",lval_num(GLFW_KEY_DELETE));
    lval_constant(e,"gl.GLFW_KEY_RIGHT",lval_num(GLFW_KEY_RIGHT));
    lval_constant(e,"gl.GLFW_KEY_LEFT",lval_num(GLFW_KEY_LEFT));
    lval_constant(e,"gl.GLFW_KEY_DOWN",lval_num(GLFW_KEY_DOWN));
    lval_constant(e,"gl.GLFW_KEY_UP",lval_num(GLFW_KEY_UP));
    lval_constant(e,"gl.GLFW_KEY_PAGE_UP",lval_num(GLFW_KEY_PAGE_UP));
    lval_constant(e,"gl.GLFW_KEY_PAGE_DOWN",lval_num(GLFW_KEY_PAGE_DOWN));
    lval_constant(e,"gl.GLFW_KEY_HOME",lval_num(GLFW_KEY_HOME));
    lval_constant(e,"gl.GLFW_KEY_END",lval_num(GLFW_KEY_END));
    lval_constant(e,"gl.GLFW_KEY_CAPS_LOCK",lval_num(GLFW_KEY_CAPS_LOCK));
    lval_constant(e,"gl.GLFW_KEY_SCROLL_LOCK",lval_num(GLFW_KEY_SCROLL_LOCK));
    lval_constant(e,"gl.GLFW_KEY_NUM_LOCK",lval_num(GLFW_KEY_NUM_LOCK));
    lval_constant(e,"gl.GLFW_KEY_PRINT_SCREEN",lval_num(GLFW_KEY_PRINT_SCREEN));
    lval_constant(e,"gl.GLFW_KEY_PAUSE",lval_num(GLFW_KEY_PAUSE));
    lval_constant(e,"gl.GLFW_KEY_F1",lval_num(GLFW_KEY_F1));
    lval_constant(e,"gl.GLFW_KEY_F2",lval_num(GLFW_KEY_F2));
    lval_constant(e,"gl.GLFW_KEY_F3",lval_num(GLFW_KEY_F3));
    lval_constant(e,"gl.GLFW_KEY_F4",lval_num(GLFW_KEY_F4));
    lval_constant(e,"gl.GLFW_KEY_F5",lval_num(GLFW_KEY_F5));
    lval_constant(e,"gl.GLFW_KEY_F6",lval_num(GLFW_KEY_F6));
    lval_constant(e,"gl.GLFW_KEY_F7",lval_num(GLFW_KEY_F7));
    lval_constant(e,"gl.GLFW_KEY_F8",lval_num(GLFW_KEY_F8));
    lval_constant(e,"gl.GLFW_KEY_F9",lval_num(GLFW_KEY_F9));
    lval_constant(e,"gl.GLFW_KEY_F10",lval_num(GLFW_KEY_F10));
    lval_constant(e,"gl.GLFW_KEY_F11",lval_num(GLFW_KEY_F11));
    lval_constant(e,"gl.GLFW_KEY_F12",lval_num(GLFW_KEY_F12));
    lval_constant(e,"gl.GLFW_KEY_F13",lval_num(GLFW_KEY_F13));
    lval_constant(e,"gl.GLFW_KEY_F14",lval_num(GLFW_KEY_F14));
    lval_constant(e,"gl.GLFW_KEY_F15",lval_num(GLFW_KEY_F15));
    lval_constant(e,"gl.GLFW_KEY_F16",lval_num(GLFW_KEY_F16));
    lval_constant(e,"gl.GLFW_KEY_F17",lval_num(GLFW_KEY_F17));
    lval_constant(e,"gl.GLFW_KEY_F18",lval_num(GLFW_KEY_F18));
    lval_constant(e,"gl.GLFW_KEY_F19",lval_num(GLFW_KEY_F19));
    lval_constant(e,"gl.GLFW_KEY_F20",lval_num(GLFW_KEY_F20));
    lval_constant(e,"gl.GLFW_KEY_F21",lval_num(GLFW_KEY_F21));
    lval_constant(e,"gl.GLFW_KEY_F22",lval_num(GLFW_KEY_F22));
    lval_constant(e,"gl.GLFW_KEY_F23",lval_num(GLFW_KEY_F23));
    lval_constant(e,"gl.GLFW_KEY_F24",lval_num(GLFW_KEY_F24));
    lval_constant(e,"gl.GLFW_KEY_F25",lval_num(GLFW_KEY_F25));
    lval_constant(e,"gl.GLFW_KEY_KP_0",lval_num(GLFW_KEY_KP_0));
    lval_constant(e,"gl.GLFW_KEY_KP_1",lval_num(GLFW_KEY_KP_1));
    lval_constant(e,"gl.GLFW_KEY_KP_2",lval_num(GLFW_KEY_KP_2));
    lval_constant(e,"gl.GLFW_KEY_KP_3",lval_num(GLFW_KEY_KP_3));
    lval_constant(e,"gl.GLFW_KEY_KP_4",lval_num(GLFW_KEY_KP_4));
    lval_constant(e,"gl.GLFW_KEY_KP_5",lval_num(GLFW_KEY_KP_5));
    lval_constant(e,"gl.GLFW_KEY_KP_6",lval_num(GLFW_KEY_KP_6));
    lval_constant(e,"gl.GLFW_KEY_KP_7",lval_num(GLFW_KEY_KP_7));
    lval_constant(e,"gl.GLFW_KEY_KP_8",lval_num(GLFW_KEY_KP_8));
    lval_constant(e,"gl.GLFW_KEY_KP_9",lval_num(GLFW_KEY_KP_9));
    lval_constant(e,"gl.GLFW_KEY_KP_DECIMAL",lval_num(GLFW_KEY_KP_DECIMAL));
    lval_constant(e,"gl.GLFW_KEY_KP_DIVIDE",lval_num(GLFW_KEY_KP_DIVIDE));
    lval_constant(e,"gl.GLFW_KEY_KP_MULTIPLY",lval_num(GLFW_KEY_KP_MULTIPLY));
    lval_constant(e,"gl.GLFW_KEY_KP_SUBTRACT",lval_num(GLFW_KEY_KP_SUBTRACT));
    lval_constant(e,"gl.GLFW_KEY_KP_ADD",lval_num(GLFW_KEY_KP_ADD));
    lval_constant(e,"gl.GLFW_KEY_KP_ENTER",lval_num(GLFW_KEY_KP_ENTER));
    lval_constant(e,"gl.GLFW_KEY_KP_EQUAL",lval_num(GLFW_KEY_KP_EQUAL));
    lval_constant(e,"gl.GLFW_KEY_LEFT_SHIFT",lval_num(GLFW_KEY_LEFT_SHIFT));
    lval_constant(e,"gl.GLFW_KEY_LEFT_CONTROL",lval_num(GLFW_KEY_LEFT_CONTROL));
    lval_constant(e,"gl.GLFW_KEY_LEFT_ALT",lval_num(GLFW_KEY_LEFT_ALT));
    lval_constant(e,"gl.GLFW_KEY_LEFT_SUPER",lval_num(GLFW_KEY_LEFT_SUPER));
    lval_constant(e,"gl.GLFW_KEY_RIGHT_SHIFT",lval_num(GLFW_KEY_RIGHT_SHIFT));
    lval_constant(e,"gl.GLFW_KEY_RIGHT_CONTROL",lval_num(GLFW_KEY_RIGHT_CONTROL));
    lval_constant(e,"gl.GLFW_KEY_RIGHT_ALT",lval_num(GLFW_KEY_RIGHT_ALT));
    lval_constant(e,"gl.GLFW_KEY_RIGHT_SUPER",lval_num(GLFW_KEY_RIGHT_SUPER));
    lval_constant(e,"gl.GLFW_KEY_MENU",lval_num(GLFW_KEY_MENU));
    lval_constant(e,"gl.GLFW_KEY_LAST",lval_num(GLFW_KEY_LAST));


    lenv_add_builtin(e,"gl.init",builtin_gl_init);
    lenv_add_builtin(e,"gl.terminate",builtin_gl_terminate);
    lenv_add_builtin(e, "gl.create_window",builtin_gl_create_window);
    lenv_add_builtin(e, "gl.destroy_window",builtin_gl_destroy_window);
    lenv_add_builtin(e, "gl.window_hint",builtin_gl_window_hint);
    lenv_add_builtin(e, "gl.make_context_current",builtin_gl_make_context_current);
    lenv_add_builtin(e, "gl.window_should_close",builtin_gl_window_should_close);
    lenv_add_builtin(e, "gl.swap_buffers",builtin_gl_swap_buffers);
    lenv_add_builtin(e, "gl.swap_interval",builtin_gl_swap_interval);
    lenv_add_builtin(e, "gl.poll_events",builtin_gl_poll_events);
    lenv_add_builtin(e, "gl.get_key",builtin_gl_get_key);
    lenv_add_builtin(e, "gl.set_window_should_close",builtin_gl_set_window_should_close);
    lenv_add_builtin(e, "gl.get_time",builtin_gl_get_time);
    lenv_add_builtin(e, "gl.set_time",builtin_gl_set_time);
    lenv_add_builtin(e, "gl.get_window_size",builtin_gl_get_window_size);
    lenv_add_builtin(e, "gl.load_image",builtin_gl_load_image);
}