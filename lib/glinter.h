// Builtin leaf for Windows
#include<stdio.h>
#include<malloc.h>
#include"GLFW/glfw3.h"

#define GLFW_INIT_CHECK() if(!GLFW_INITIALIZED){lval_del(a);return lval_err("GLFW is not initialized");}

GLFWwindow** windows_list=NULL;
int next_window_index=1;
GLFWmonitor** monitors_list=NULL;
int next_monitor_index=1;

int GLFW_INITIALIZED=0;

void glinter_error_callback(int error,const char* description){
    printf("Error: [%d] %s\n",error,description);
}

lval* builtin_gl_init(lenv*e,lval* a){
    LASSERT_NUM("gl.init",a,1);
    LASSERT_TYPE("gl.init",a,0,LVAL_SEXPR);
    windows_list=malloc(1*sizeof(GLFWwindow*));
    windows_list[0]=NULL;
    monitors_list=malloc(1*sizeof(GLFWmonitor*));
    monitors_list[0]=NULL;
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
    return lval_sexpr();
}

lval* builtin_gl_create_window(lenv*e,lval* a){
    LASSERT_NUM("gl.create_window",a,5);
    LASSERT_TYPE("gl.create_window",a,0,LVAL_NUM);
    LASSERT_TYPE("gl.create_window",a,1,LVAL_NUM);
    LASSERT_TYPE("gl.create_window",a,2,LVAL_STR);
    LASSERT_TYPE("gl.create_window",a,3,LVAL_NUM);
    LASSERT_TYPE("gl.create_window",a,4,LVAL_NUM);
    GLFW_INIT_CHECK();
    int width=a->cell[0]->num;
    int height=a->cell[1]->num;
    char* title=a->cell[2]->str;
    int monitor_id=a->cell[3]->num;
    int share_id=a->cell[4]->num;
    if(monitor_id>=next_monitor_index){
        lval_del(a);
        return lval_err("Invalid GLFW monitor: %d",monitor_id);
    }if(share_id>=next_window_index){
        lval_del(a);
        return lval_err("Invalid GLFW window: %d",share_id);
    }

    GLFWwindow* window=glfwCreateWindow(width,height,title,monitors_list[monitor_id],windows_list[share_id]);
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

lval* builtin_gl_poll_events(lenv*e,lval* a){
    LASSERT_NUM("gl.poll_events",a,1);
    LASSERT_TYPE("gl.poll_events",a,0,LVAL_SEXPR);
    GLFW_INIT_CHECK();
    glfwPollEvents();
    return lval_sexpr();
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

    lenv_add_builtin(e,"gl.init",builtin_gl_init);
    lenv_add_builtin(e,"gl.terminate",builtin_gl_terminate);
    lenv_add_builtin(e, "gl.create_window",builtin_gl_create_window);
    lenv_add_builtin(e, "gl.destroy_window",builtin_gl_destroy_window);
    lenv_add_builtin(e, "gl.window_hint",builtin_gl_window_hint);
    lenv_add_builtin(e, "gl.make_context_current",builtin_gl_make_context_current);
    lenv_add_builtin(e, "gl.window_should_close",builtin_gl_window_should_close);
    lenv_add_builtin(e, "gl.swap_buffers",builtin_gl_swap_buffers);
    lenv_add_builtin(e, "gl.poll_events",builtin_gl_poll_events);
}