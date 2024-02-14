#include<stddef.h>
#include"raylib.h"

Texture* textures_list=NULL;
int texture_index=0;
Font* fonts_list=NULL;
int font_index=0;
Music* musics_list=NULL;
int music_index=0;
Sound* sounds_list=NULL;
int sound_index=0;

typedef struct lval lval;
typedef struct lenv lenv;

//
enum{
    LVAL_NUM,
    LVAL_ERR,
    LVAL_SYM,//symbol
    LVAL_STR,
    LVAL_FUN,
    LVAL_SEXPR,
    LVAL_QEXPR
};

//
enum{
    LERR_DIV_ZERO,
    LERR_BAD_OP,
    LERR_BAD_NUM
};

typedef lval*(*lbuiltin)(lenv*,lval*);

struct lval {
    int type;//0:num,1:err

    double num;
    //types string message
    char* err;
    char* sym;//symbol
    char* str;

    //function
    lbuiltin builtin;//
    lenv* env;
    lval* foramls;
    lval* body;

    //count and pointer to a list of lval*
    int count;
    struct lval** cell;//not only one
};

struct lenv{
    lenv* par;
    int count;
    char** syms;
    lval** vals;
};

char* ltype_name(int t);

lval* lval_num(double x);
lval* lval_err(char* fmt,...);
lval* lval_sym(char* s);
lval* lval_sexpr();
lval* lval_qexpr();
lval* lval_fun(lbuiltin func);
lval* lval_builtin(lbuiltin func);
lval* lval_lambda(lval* foramls,lval* body);
lval* lval_str(char* s);

void lval_del(lval* v);
lval* lval_add(lval* v,lval* x);

#define LASSERT(args,cond,fmt,...)\
    if(!(cond)){ \
        lval* err=lval_err(fmt,##__VA_ARGS__);\
        lval_del(args);\
        return err;\
    }

#define LASSERT_TYPE(func, args, index, expect) \
      LASSERT(args, args->cell[index]->type == expect, \
        "Function '%s' passed incorrect type for argument %i. Got %s, Expected %s.", \
        func, index, ltype_name(args->cell[index]->type), ltype_name(expect))

#define LASSERT_NUM(func, args, num) \
      LASSERT(args, args->count == num, \
        "Function '%s' passed incorrect number of arguments. Got %i, Expected %i.", \
        func, args->count, num)

#define LASSERT_NOT_EMPTY(func, args, index) \
      LASSERT(args, args->cell[index]->count != 0, \
        "Function '%s' passed {} for argument %i.", func, index);

void lenv_add_builtin(lenv* e,char* name,lbuiltin func);

#define RAYLIB_WINDOW_CHECK() if(!IsWindowReady()){lval_del(a);return lval_err("Raylib not ready");}
#define RAYLIB_AUDIO_CHECK() if(!IsAudioDeviceReady()){lval_del(a);return lval_err("Audio Device not ready");}

lval* lval_color(Color color){
    lval* result=lval_qexpr();
    result=lval_add(result,lval_num(color.r));
    result=lval_add(result,lval_num(color.g));
    result=lval_add(result,lval_num(color.b));
    result=lval_add(result,lval_num(color.a));
    return result;
}

void raylib_hide_logs(int,const char*,va_list){}

lval* builtin_raylib_init(lenv*e,lval*a){
    LASSERT_NUM("raylib.init",a,3);
    LASSERT_TYPE("raylib.init",a,0,LVAL_NUM);
    LASSERT_TYPE("raylib.init",a,1,LVAL_NUM);
    LASSERT_TYPE("raylib.init",a,2,LVAL_STR);
    int width=a->cell[0]->num,height=a->cell[0]->num;
    char* title=a->cell[2]->str;
    InitWindow(width,height,title);
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_raylib_audio_init(lenv*e,lval*a){
    LASSERT_NUM("raylib.audio_init",a,1);
    LASSERT_TYPE("raylib.audio_init",a,0,LVAL_SEXPR);
    InitAudioDevice();
    lval_del(a);
    return lval_sexpr();
}

lval* builitn_raylib_set_fps(lenv*e,lval*a){
    LASSERT_NUM("raylib.set_target_fps",a,1);
    LASSERT_TYPE("raylib.set_target_fps",a,0,LVAL_NUM);
    RAYLIB_WINDOW_CHECK();
    int fps=a->cell[0]->num;
    SetTargetFPS(fps);
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_raylib_window_should_close(lenv*e,lval*a){
    LASSERT_NUM("raylib.window_should_close",a,1);
    LASSERT_TYPE("raylib.window_should_close",a,0,LVAL_SEXPR);
    RAYLIB_WINDOW_CHECK();
    int result=WindowShouldClose();
    lval_del(a);
    return lval_num(result);
}

lval* builtin_raylib_begin_drawing(lenv*e,lval*a){
    LASSERT_NUM("raylib.begin_drawing",a,1);
    LASSERT_TYPE("raylib.begin_drawing",a,0,LVAL_SEXPR);
    RAYLIB_WINDOW_CHECK();
    BeginDrawing();
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_raylib_end_drawing(lenv*e,lval*a){
    LASSERT_NUM("raylib.end_drawing",a,1);
    LASSERT_TYPE("raylib.end_drawing",a,0,LVAL_SEXPR);
    RAYLIB_WINDOW_CHECK();
    EndDrawing();
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_raylib_clear_background(lenv*e,lval*a){
    LASSERT_NUM("raylib.clear_background",a,1);
    LASSERT_TYPE("raylib.clear_background",a,0,LVAL_QEXPR);
    RAYLIB_WINDOW_CHECK();
    Color color;
    color.r=a->cell[0]->cell[0]->num;
    color.g=a->cell[0]->cell[1]->num;
    color.b=a->cell[0]->cell[2]->num;
    color.a=a->cell[0]->cell[3]->num;
    ClearBackground(color);
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_raylib_close(lenv*e,lval*a){
    LASSERT_NUM("raylib.close",a,1);
    LASSERT_TYPE("raylib.close",a,0,LVAL_SEXPR);
    RAYLIB_WINDOW_CHECK();
    CloseWindow();
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_raylib_audio_close(lenv*e,lval*a){
    LASSERT_NUM("raylib.audio_close",a,1);
    LASSERT_TYPE("raylib.audio_close",a,0,LVAL_SEXPR);
    RAYLIB_AUDIO_CHECK();
    CloseAudioDevice();
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_raylib_title(lenv*e,lval*a){
    LASSERT_NUM("raylib.title",a,1);
    LASSERT_TYPE("raylib.title",a,0,LVAL_STR);
    RAYLIB_WINDOW_CHECK();
    char* title=a->cell[0]->str;
    SetWindowTitle(title);
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_raylib_icon(lenv*e,lval*a){
    LASSERT_NUM("raylib.icon",a,1);
    LASSERT_TYPE("raylib.icon",a,0,LVAL_STR);
    RAYLIB_WINDOW_CHECK();
    char* path=a->cell[0]->str;
    Image icon=LoadImage(path);
    if(!icon.data){
        lval_del(a);
        return lval_err("Couldn't load icon: %s",path);
    }
    SetWindowIcon(icon);
    UnloadImage(icon);
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_raylib_text(lenv*e,lval*a){
    LASSERT_NUM("raylib.text",a,5);
    LASSERT_TYPE("raylib.text",a,0,LVAL_STR);
    LASSERT_TYPE("raylib.text",a,1,LVAL_NUM);
    LASSERT_TYPE("raylib.text",a,2,LVAL_NUM);
    LASSERT_TYPE("raylib.text",a,3,LVAL_NUM);
    LASSERT_TYPE("raylib.text",a,4,LVAL_QEXPR);
    RAYLIB_WINDOW_CHECK();
    char* text=a->cell[0]->str;
    int x=a->cell[1]->num;
    int y=a->cell[2]->num;
    int font_size=a->cell[3]->num;
    Color color;
    color.r=a->cell[4]->cell[0]->num;
    color.g=a->cell[4]->cell[1]->num;
    color.b=a->cell[4]->cell[2]->num;
    color.a=a->cell[4]->cell[3]->num;
    DrawText(text,x,y,font_size,color);
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_raylib_text_ex(lenv*e,lval*a){
    LASSERT_NUM("raylib.text_ex",a,7);
    LASSERT_TYPE("raylib.text_ex",a,0,LVAL_NUM);
    LASSERT_TYPE("raylib.text_ex",a,1,LVAL_STR);
    LASSERT_TYPE("raylib.text_ex",a,2,LVAL_NUM);
    LASSERT_TYPE("raylib.text_ex",a,3,LVAL_NUM);
    LASSERT_TYPE("raylib.text_ex",a,4,LVAL_NUM);
    LASSERT_TYPE("raylib.text_ex",a,5,LVAL_NUM);
    LASSERT_TYPE("raylib.text_ex",a,6,LVAL_QEXPR);
    RAYLIB_WINDOW_CHECK();
    int font_id=a->cell[0]->num;
    if(font_id>=font_index){
        lval_del(a);
        return lval_err("Invalid font: %d",font_id);
    }
    char* text=a->cell[1]->str;
    Vector2 pos={.x=a->cell[2]->num,.y=a->cell[3]->num};
    int size=a->cell[4]->num,spacing=a->cell[5]->num;
    Color color;
    color.r=a->cell[6]->cell[0]->num;
    color.g=a->cell[6]->cell[1]->num;
    color.b=a->cell[6]->cell[2]->num;
    color.a=a->cell[6]->cell[3]->num;
    DrawTextEx(fonts_list[font_id],text,pos,size,spacing,color);
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_raylib_hide_logs(lenv*e,lval*a){
    LASSERT_NUM("raylib.hide_logs",a,1);
    LASSERT_TYPE("raylib.hide_logs",a,0,LVAL_SEXPR);
    SetTraceLogCallback(raylib_hide_logs);
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_raylib_show_logs(lenv*e,lval*a){
    LASSERT_NUM("raylib.show_logs",a,1);
    LASSERT_TYPE("raylib.show_logs",a,0,LVAL_SEXPR);
    SetTraceLogCallback(NULL);
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_raylib_time(lenv*e,lval*a){
    LASSERT_NUM("raylib.time",a,1);
    LASSERT_TYPE("raylib.time",a,0,LVAL_SEXPR);
    RAYLIB_WINDOW_CHECK();
    double time=GetTime();
    lval_del(a);
    return lval_num(time);
}

lval* builtin_raylib_is_ready(lenv*e,lval*a){
    LASSERT_NUM("raylib.is_ready",a,1);
    LASSERT_TYPE("raylib.is_ready",a,0,LVAL_SEXPR);
    int result=IsWindowReady();
    lval_del(a);
    return lval_num(result);
}

lval* builtin_raylib_rectangle(lenv*e,lval*a){
    LASSERT_NUM("raylib.rectangle",a,5);
    LASSERT_TYPE("raylib.rectangle",a,0,LVAL_NUM);
    LASSERT_TYPE("raylib.rectangle",a,1,LVAL_NUM);
    LASSERT_TYPE("raylib.rectangle",a,2,LVAL_NUM);
    LASSERT_TYPE("raylib.rectangle",a,3,LVAL_NUM);
    LASSERT_TYPE("raylib.rectangle",a,4,LVAL_QEXPR);
    RAYLIB_WINDOW_CHECK();
    int x=a->cell[0]->num,y=a->cell[1]->num,
        width=a->cell[2]->num,height=a->cell[3]->num;
    Color color;
    color.r=a->cell[4]->cell[0]->num;
    color.g=a->cell[4]->cell[1]->num;
    color.b=a->cell[4]->cell[2]->num;
    color.a=a->cell[4]->cell[3]->num;
    DrawRectangle(x,y,width,height,color);
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_raylib_get_size(lenv*e,lval*a){
    LASSERT_NUM("raylib.get_size",a,1);
    LASSERT_TYPE("raylib.get_size",a,0,LVAL_SEXPR);
    RAYLIB_WINDOW_CHECK();
    lval* result=lval_qexpr();
    result=lval_add(result,lval_num(GetScreenWidth()));
    result=lval_add(result,lval_num(GetScreenHeight()));
    lval_del(a);
    return result;
}

lval* builtin_raylib_set_size(lenv*e,lval*a){
    LASSERT_NUM("raylib.set_size",a,1);
    LASSERT_TYPE("raylib.set_size",a,0,LVAL_NUM);
    LASSERT_TYPE("raylib.set_size",a,1,LVAL_NUM);
    RAYLIB_WINDOW_CHECK();
    int width=a->cell[0]->num,height=a->cell[1]->num;
    SetWindowSize(width,height);
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_raylib_is_resized(lenv*e,lval*a){
    LASSERT_NUM("raylib.is_resized",a,1);
    LASSERT_TYPE("raylib.is_resized",a,0,LVAL_SEXPR);
    RAYLIB_WINDOW_CHECK();
    int result=IsWindowResized();
    lval_del(a);
    return lval_num(result);
}

lval* builtin_raylib_set_state(lenv*e,lval*a){
    LASSERT_NUM("raylib.set_state",a,1);
    LASSERT_TYPE("raylib.set_state",a,0,LVAL_NUM);
    RAYLIB_WINDOW_CHECK();
    int flag=a->cell[0]->num;
    SetWindowState(flag);
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_raylib_fullscreen(lenv*e,lval*a){
    LASSERT_NUM("raylib.fullscreen",a,1);
    LASSERT_TYPE("raylib.fullscreen",a,0,LVAL_SEXPR);
    RAYLIB_WINDOW_CHECK();
    ToggleFullscreen();
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_raylib_get_monitor_size(lenv*e,lval*a){
    LASSERT_NUM("raylib.get_monitor_size",a,1);
    LASSERT_TYPE("raylib.get_monitor_size",a,0,LVAL_NUM);
    int monitor=a->cell[0]->num;
    int width=GetMonitorWidth(monitor),height=GetMonitorHeight(monitor);
    lval* result=lval_qexpr();
    result=lval_add(result,lval_num(width));
    result=lval_add(result,lval_num(height));
    lval_del(a);
    return result;
}

lval* builtin_raylib_maximize(lenv*e,lval*a){
    LASSERT_NUM("raylib.maximize",a,1);
    LASSERT_TYPE("raylib.maximize",a,0,LVAL_SEXPR);
    RAYLIB_WINDOW_CHECK();
    MaximizeWindow();
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_raylib_load_font(lenv*e,lval*a){
    LASSERT_NUM("raylib.load_font",a,1);
    LASSERT_TYPE("raylib.load_font",a,0,LVAL_STR);
    RAYLIB_WINDOW_CHECK();
    char* path=a->cell[0]->str;
    Font font=LoadFont(path);
    fonts_list=realloc(fonts_list,(font_index+1)*sizeof(Font));
    fonts_list[font_index]=font;
    lval_del(a);
    return lval_num(font_index++);
}

lval* builtin_raylib_unload_font(lenv*e,lval*a){
    LASSERT_NUM("raylib.unload_font",a,1);
    LASSERT_TYPE("raylib.unload_font",a,0,LVAL_NUM);
    RAYLIB_WINDOW_CHECK();
    int font_id=a->cell[0]->num;
    if(font_id>=font_index){
        lval_del(a);
        return lval_err("Invalid font: %d",font_id);
    }
    Font font=fonts_list[font_id];
    UnloadFont(font);
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_raylib_load_texture(lenv*e,lval*a){
    LASSERT_NUM("raylib.load_texture",a,3);
    LASSERT_TYPE("raylib.load_texture",a,0,LVAL_STR);
    LASSERT_TYPE("raylib.load_texture",a,1,LVAL_NUM);
    LASSERT_TYPE("raylib.load_texture",a,1,LVAL_NUM);
    RAYLIB_WINDOW_CHECK();
    char* path=a->cell[0]->str;
    int width=a->cell[1]->num,height=a->cell[2]->num;
    Image image=LoadImage(path);
    if(!image.data){
        lval_del(a);
        UnloadImage(image);
        return lval_err("Couldn't load image");
    }
    if(width==-1) width=image.width;
    if(height==-1) height=image.height;
    ImageResize(&image,width,height);
    Texture texture=LoadTextureFromImage(image);
    if(!IsTextureReady(texture)){
        lval_del(a);
        UnloadImage(image);
        UnloadTexture(texture);
        return lval_err("Couldn't load texture");
    }UnloadImage(image);
    textures_list=realloc(textures_list,(texture_index+1)*sizeof(Texture));
    textures_list[texture_index]=texture;
    lval_del(a);
    return lval_num(texture_index++);
}

lval* builtin_raylib_paint_texture(lenv*e,lval*a){
    LASSERT_NUM("raylib.paint_texture",a,4);
    LASSERT_TYPE("raylib.paint_texture",a,0,LVAL_NUM);
    LASSERT_TYPE("raylib.paint_texture",a,1,LVAL_NUM);
    LASSERT_TYPE("raylib.paint_texture",a,2,LVAL_NUM);
    LASSERT_TYPE("raylib.paint_texture",a,3,LVAL_QEXPR);
    RAYLIB_WINDOW_CHECK();
    int texture_id=a->cell[0]->num;
    if(texture_id>=texture_index){
        lval_del(a);
        return lval_err("Invalid texture: %d",texture_id);
    }
    Texture texture=textures_list[texture_id];
    int x=a->cell[1]->num,y=a->cell[2]->num;
    Color color;
    color.r=a->cell[3]->cell[0]->num;
    color.g=a->cell[3]->cell[1]->num;
    color.b=a->cell[3]->cell[2]->num;
    color.a=a->cell[3]->cell[3]->num;
    DrawTexture(texture,x,y,color);
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_raylib_paint_texture_ex(lenv*e,lval*a){
    LASSERT_NUM("raylib.paint_texture_ex",a,6);
    LASSERT_TYPE("raylib.paint_texture_ex",a,0,LVAL_NUM);
    LASSERT_TYPE("raylib.paint_texture_ex",a,1,LVAL_NUM);
    LASSERT_TYPE("raylib.paint_texture_ex",a,2,LVAL_NUM);
    LASSERT_TYPE("raylib.paint_texture_ex",a,3,LVAL_NUM);
    LASSERT_TYPE("raylib.paint_texture_ex",a,4,LVAL_NUM);
    LASSERT_TYPE("raylib.paint_texture_ex",a,5,LVAL_QEXPR);
    RAYLIB_WINDOW_CHECK();
    int texture_id=a->cell[0]->num;
    if(texture_id>=texture_index){
        lval_del(a);
        return lval_err("Invalid texture: %d",texture_id);
    }
    Texture texture=textures_list[texture_id];
    int x=a->cell[1]->num,y=a->cell[2]->num;
    Vector2 pos={.x=x,.y=y};
    double rotation=a->cell[3]->num,scale=a->cell[4]->num;
    Color color;
    color.r=a->cell[5]->cell[0]->num;
    color.g=a->cell[5]->cell[1]->num;
    color.b=a->cell[5]->cell[2]->num;
    color.a=a->cell[5]->cell[3]->num;
    DrawTextureEx(texture,pos,rotation,scale,color);
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_raylib_unload_texture(lenv*e,lval*a){
    LASSERT_NUM("raylib.unload_texture",a,1);
    LASSERT_TYPE("raylib.unload_texture",a,0,LVAL_NUM);
    RAYLIB_WINDOW_CHECK();
    int texture_id=a->cell[0]->num;
    if(texture_id>=texture_index){
        lval_del(a);
        return lval_err("Invalid texture: %d",texture_id);
    }
    Texture texture=textures_list[texture_id];
    UnloadTexture(texture);
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_raylib_is_key_pressed(lenv*e,lval*a){
    LASSERT_NUM("raylib.is_key_pressed",a,1);
    LASSERT_TYPE("raylib.is_key_pressed",a,0,LVAL_NUM);
    RAYLIB_WINDOW_CHECK();
    int key=a->cell[0]->num;
    int result=IsKeyPressed(key);
    lval_del(a);
    return lval_num(result);
}

lval* builitn_raylib_is_key_pressed_repeat(lenv*e,lval*a){
    LASSERT_NUM("raylib.is_key_pressed_repeat",a,1);
    LASSERT_TYPE("raylib.is_key_pressed_repeat",a,0,LVAL_NUM);
    RAYLIB_WINDOW_CHECK();
    int key=a->cell[0]->num;
    int result=IsKeyPressedRepeat(key);
    lval_del(a);
    return lval_num(result);
}

lval* builtin_raylib_is_key_down(lenv*e,lval*a){
    LASSERT_NUM("raylib.is_key_down",a,1);
    LASSERT_TYPE("raylib.is_key_down",a,0,LVAL_NUM);
    RAYLIB_WINDOW_CHECK();
    int key=a->cell[0]->num;
    int result=IsKeyDown(key);
    lval_del(a);
    return lval_num(result);
}

lval* builtin_raylib_set_cursor(lenv*e,lval*a){
    LASSERT_NUM("raylib.set_cursor",a,1);
    LASSERT_TYPE("raylib.set_cursor",a,0,LVAL_NUM);
    RAYLIB_WINDOW_CHECK();
    int cursor=a->cell[0]->num;
    SetMouseCursor(cursor);
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_raylib_get_key_pressed(lenv*e,lval*a){
    LASSERT_NUM("raylib.get_key_pressed",a,1);
    LASSERT_TYPE("raylib.get_key_pressed",a,0,LVAL_SEXPR);
    RAYLIB_WINDOW_CHECK();
    int result=GetKeyPressed();
    lval_del(a);
    return lval_num(result);
}

lval* builtin_raylib_is_mouse_button_down(lenv*e,lval*a){
    LASSERT_NUM("raylib.is_mouse_button_down",a,1);
    LASSERT_TYPE("raylib.is_mouse_button_down",a,0,LVAL_NUM);
    RAYLIB_WINDOW_CHECK();
    int mouse=a->cell[0]->num;
    int result=IsMouseButtonDown(mouse);
    lval_del(a);
    return lval_num(result);
}

lval* builtin_raylib_is_mouse_button_up(lenv*e,lval*a){
    LASSERT_NUM("raylib.is_mouse_button_up",a,1);
    LASSERT_TYPE("raylib.is_mouse_button_up",a,0,LVAL_NUM);
    RAYLIB_WINDOW_CHECK();
    int mouse=a->cell[0]->num;
    int result=IsMouseButtonUp(mouse);
    lval_del(a);
    return lval_num(result);
}

lval* builtin_raylib_is_mouse_button_pressed(lenv*e,lval*a){
    LASSERT_NUM("raylib.is_mouse_button_pressed",a,1);
    LASSERT_TYPE("raylib.is_mouse_button_pressed",a,0,LVAL_NUM);
    RAYLIB_WINDOW_CHECK();
    int mouse=a->cell[0]->num;
    int result=IsMouseButtonPressed(mouse);
    lval_del(a);
    return lval_num(result);
}
lval* builtin_raylib_is_mouse_button_released(lenv*e,lval*a){
    LASSERT_NUM("raylib.is_mouse_button_released",a,1);
    LASSERT_TYPE("raylib.is_mouse_button_released",a,0,LVAL_NUM);
    RAYLIB_WINDOW_CHECK();
    int mouse=a->cell[0]->num;
    int result=IsMouseButtonReleased(mouse);
    lval_del(a);
    return lval_num(result);
}

lval* builtin_raylib_get_mouse_pos(lenv*e,lval*a){
    LASSERT_NUM("raylib.get_mouse_pos",a,1);
    LASSERT_TYPE("raylib.get_mouse_pos",a,0,LVAL_SEXPR);
    RAYLIB_WINDOW_CHECK();
    Vector2 pos=GetMousePosition();
    lval* result=lval_qexpr();
    result=lval_add(result,lval_num(pos.x));
    result=lval_add(result,lval_num(pos.y));
    lval_del(a);
    return result;
}

lval* builtin_raylib_load_music(lenv*e,lval*a){
    LASSERT_NUM("raylib.load_music",a,1);
    LASSERT_TYPE("raylib.load_music",a,0,LVAL_STR);
    RAYLIB_AUDIO_CHECK();
    char* path=a->cell[0]->str;
    Music music=LoadMusicStream(path);
    if(!IsMusicReady(music)){
        UnloadMusicStream(music);
        lval_del(a);
        return lval_err("Couldn't load music");
    }
    musics_list=realloc(musics_list,(music_index+1)*sizeof(Music));
    musics_list[music_index]=music;
    lval_del(a);
    return lval_num(music_index++);
}

lval* builtin_raylib_unload_music(lenv*e,lval*a){
    LASSERT_NUM("raylib.unload_music",a,1);
    LASSERT_TYPE("raylib.unload_music",a,0,LVAL_NUM);
    RAYLIB_AUDIO_CHECK();
    int music_id=a->cell[0]->num;
    if(music_id>=music_index){
        lval_del(a);
        return lval_err("Invalid music: %d",music_id);
    }UnloadMusicStream(musics_list[music_id]);
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_raylib_play_music(lenv*e,lval*a){
    LASSERT_NUM("raylib.play_music",a,1);
    LASSERT_TYPE("raylib.play_music",a,0,LVAL_NUM);
    RAYLIB_AUDIO_CHECK();
    int music_id=a->cell[0]->num;
    if(music_id>=music_index){
        lval_del(a);
        return lval_err("Invalid music: %d",music_id);
    }PlayMusicStream(musics_list[music_id]);
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_raylib_stop_music(lenv*e,lval*a){
    LASSERT_NUM("raylib.stop_music",a,1);
    LASSERT_TYPE("raylib.stop_music",a,0,LVAL_NUM);
    RAYLIB_AUDIO_CHECK();
    int music_id=a->cell[0]->num;
    if(music_id>=music_index){
        lval_del(a);
        return lval_err("Invalid music: %d",music_id);
    }StopMusicStream(musics_list[music_id]);
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_raylib_update_music(lenv*e,lval*a){
    LASSERT_NUM("raylib.update_music",a,1);
    LASSERT_TYPE("raylib.update_music",a,0,LVAL_NUM);
    RAYLIB_AUDIO_CHECK();
    int music_id=a->cell[0]->num;
    if(music_id>=music_index){
        lval_del(a);
        return lval_err("Invalid music: %d",music_id);
    }UpdateMusicStream(musics_list[music_id]);
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_raylib_pause_music(lenv*e,lval*a){
    LASSERT_NUM("raylib.pause_music",a,1);
    LASSERT_TYPE("raylib.pause_music",a,0,LVAL_NUM);
    RAYLIB_AUDIO_CHECK();
    int music_id=a->cell[0]->num;
    if(music_id>=music_index){
        lval_del(a);
        return lval_err("Invalid music: %d",music_id);
    }PauseMusicStream(musics_list[music_id]);
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_raylib_resume_music(lenv*e,lval*a){
    LASSERT_NUM("raylib.resume_music",a,1);
    LASSERT_TYPE("raylib.resume_music",a,0,LVAL_NUM);
    RAYLIB_AUDIO_CHECK();
    int music_id=a->cell[0]->num;
    if(music_id>=music_index){
        lval_del(a);
        return lval_err("Invalid music: %d",music_id);
    }ResumeMusicStream(musics_list[music_id]);
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_raylib_is_music_playing(lenv*e,lval*a){
    LASSERT_NUM("raylib.is_music_playing",a,1);
    LASSERT_TYPE("raylib.is_music_playing",a,0,LVAL_NUM);
    RAYLIB_AUDIO_CHECK();
    int music_id=a->cell[0]->num;
    if(music_id>=music_index){
        lval_del(a);
        return lval_err("Invalid music: %d",music_id);
    }int is_music_playing=IsMusicStreamPlaying(musics_list[music_id]);
    lval_del(a);
    return lval_num(is_music_playing);
}

lval* builtin_raylib_load_sound(lenv*e,lval*a){
    LASSERT_NUM("raylib.load_sound",a,1);
    LASSERT_TYPE("raylib.load_sound",a,0,LVAL_STR);
    RAYLIB_AUDIO_CHECK();
    char* path=a->cell[0]->str;
    Sound sound=LoadSound(path);
    if(!IsSoundReady){
        UnloadSound(sound);
        lval_del(a);
        return lval_err("Couldn't load sound");
    }
    sounds_list=realloc(sounds_list,(sound_index+1)*sizeof(Sound));
    sounds_list[sound_index]=sound;
    lval_del(a);
    return lval_num(sound_index++);
}

lval* builtin_raylib_unload_sound(lenv*e,lval*a){
    LASSERT_NUM("raylib.unload_sound",a,1);
    LASSERT_TYPE("raylib.unload_sound",a,0,LVAL_NUM);
    RAYLIB_AUDIO_CHECK();
    int sound_id=a->cell[0]->num;
    if(sound_id>=sound_index){
        lval_del(a);
        return lval_err("Invalid sound: %d",sound_id);
    }UnloadSound(sounds_list[sound_id]);
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_raylib_play_sound(lenv*e,lval*a){
    LASSERT_NUM("raylib.play_sound",a,1);
    LASSERT_TYPE("raylib.play_sound",a,0,LVAL_NUM);
    RAYLIB_AUDIO_CHECK();
    int sound_id=a->cell[0]->num;
    if(sound_id>=sound_index){
        lval_del(a);
        return lval_err("Invalid sound: %d",sound_id);
    }PlaySound(sounds_list[sound_id]);
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_raylib_stop_sound(lenv*e,lval*a){
    LASSERT_NUM("raylib.stop_sound",a,1);
    LASSERT_TYPE("raylib.stop_sound",a,0,LVAL_NUM);
    RAYLIB_AUDIO_CHECK();
    int sound_id=a->cell[0]->num;
    if(sound_id>=sound_index){
        lval_del(a);
        return lval_err("Invalid sound: %d",sound_id);
    }StopSound(sounds_list[sound_id]);
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_raylib_is_sound_playing(lenv*e,lval*a){
    LASSERT_NUM("raylib.is_sound_playing",a,1);
    LASSERT_TYPE("raylib.is_sound_playing",a,0,LVAL_NUM);
    RAYLIB_AUDIO_CHECK();
    int sound_id=a->cell[0]->num;
    if(sound_id>=sound_index){
        lval_del(a);
        return lval_err("Invalid sound: %d",sound_id);
    }int is_sound_playing=IsSoundPlaying(sounds_list[sound_id]);
    lval_del(a);
    return lval_num(is_sound_playing);
}

void raylib_init(lenv* e){
    textures_list=malloc(0);
    fonts_list=malloc(0);
    musics_list=malloc(0);
    sounds_list=malloc(0);

    lval_constant(e,"raylib.RAYLIB_VERSION_MAJOR",lval_num(RAYLIB_VERSION_MAJOR));
    lval_constant(e,"raylib.RAYLIB_VERSION_MINOR",lval_num(RAYLIB_VERSION_MINOR));
    lval_constant(e,"raylib.RAYLIB_VERSION_PATCH",lval_num(RAYLIB_VERSION_PATCH));
    lval_constant(e,"raylib.RAYLIB_VERSION",lval_str(RAYLIB_VERSION));

    lval_constant(e,"raylib.LIGHTGRAY",lval_color(LIGHTGRAY));
    lval_constant(e,"raylib.GRAY",lval_color(GRAY));
    lval_constant(e,"raylib.DARKGRAY",lval_color(DARKGRAY));
    lval_constant(e,"raylib.YELLOW",lval_color(YELLOW));
    lval_constant(e,"raylib.GOLD",lval_color(GOLD));
    lval_constant(e,"raylib.ORANGE",lval_color(ORANGE));
    lval_constant(e,"raylib.PINK",lval_color(PINK));
    lval_constant(e,"raylib.RED",lval_color(RED));
    lval_constant(e,"raylib.MAROON",lval_color(MAROON));
    lval_constant(e,"raylib.GREEN",lval_color(GREEN));
    lval_constant(e,"raylib.LIME",lval_color(LIME));
    lval_constant(e,"raylib.DARKGREEN",lval_color(DARKGREEN));
    lval_constant(e,"raylib.SKYBLUE",lval_color(SKYBLUE));
    lval_constant(e,"raylib.BLUE",lval_color(BLUE));
    lval_constant(e,"raylib.DARKBLUE",lval_color(DARKBLUE));
    lval_constant(e,"raylib.PURPLE",lval_color(PURPLE));
    lval_constant(e,"raylib.VIOLET",lval_color(VIOLET));
    lval_constant(e,"raylib.DARKPURPLE",lval_color(DARKPURPLE));
    lval_constant(e,"raylib.BEIGE",lval_color(BEIGE));
    lval_constant(e,"raylib.BROWN",lval_color(BROWN));
    lval_constant(e,"raylib.DARKBROWN",lval_color(DARKBROWN));
    lval_constant(e,"raylib.WHITE",lval_color(WHITE));
    lval_constant(e,"raylib.BLACK",lval_color(BLACK));
    lval_constant(e,"raylib.BLANK",lval_color(BLANK));
    lval_constant(e,"raylib.MAGENTA",lval_color(MAGENTA));
    lval_constant(e,"raylib.RAYWHITE",lval_color(RAYWHITE));

    lval_constant(e,"raylib.KEY_NULL",lval_num(KEY_NULL));
    lval_constant(e,"raylib.KEY_APOSTROPHE",lval_num(KEY_APOSTROPHE));
    lval_constant(e,"raylib.KEY_COMMA",lval_num(KEY_COMMA));
    lval_constant(e,"raylib.KEY_MINUS",lval_num(KEY_MINUS));
    lval_constant(e,"raylib.KEY_PERIOD",lval_num(KEY_PERIOD));
    lval_constant(e,"raylib.KEY_SLASH",lval_num(KEY_SLASH));
    lval_constant(e,"raylib.KEY_ZERO",lval_num(KEY_ZERO));
    lval_constant(e,"raylib.KEY_ONE",lval_num(KEY_ONE));
    lval_constant(e,"raylib.KEY_TWO",lval_num(KEY_TWO));
    lval_constant(e,"raylib.KEY_THREE",lval_num(KEY_THREE));
    lval_constant(e,"raylib.KEY_FOUR",lval_num(KEY_FOUR));
    lval_constant(e,"raylib.KEY_FIVE",lval_num(KEY_FIVE));
    lval_constant(e,"raylib.KEY_SIX",lval_num(KEY_SIX));
    lval_constant(e,"raylib.KEY_SEVEN",lval_num(KEY_SEVEN));
    lval_constant(e,"raylib.KEY_EIGHT",lval_num(KEY_EIGHT));
    lval_constant(e,"raylib.KEY_NINE",lval_num(KEY_NINE));
    lval_constant(e,"raylib.KEY_SEMICOLON",lval_num(KEY_SEMICOLON));
    lval_constant(e,"raylib.KEY_EQUAL",lval_num(KEY_EQUAL));
    lval_constant(e,"raylib.KEY_A",lval_num(KEY_A));
    lval_constant(e,"raylib.KEY_B",lval_num(KEY_B));
    lval_constant(e,"raylib.KEY_C",lval_num(KEY_C));
    lval_constant(e,"raylib.KEY_D",lval_num(KEY_D));
    lval_constant(e,"raylib.KEY_E",lval_num(KEY_E));
    lval_constant(e,"raylib.KEY_F",lval_num(KEY_F));
    lval_constant(e,"raylib.KEY_G",lval_num(KEY_G));
    lval_constant(e,"raylib.KEY_H",lval_num(KEY_H));
    lval_constant(e,"raylib.KEY_I",lval_num(KEY_I));
    lval_constant(e,"raylib.KEY_J",lval_num(KEY_J));
    lval_constant(e,"raylib.KEY_K",lval_num(KEY_K));
    lval_constant(e,"raylib.KEY_L",lval_num(KEY_L));
    lval_constant(e,"raylib.KEY_M",lval_num(KEY_M));
    lval_constant(e,"raylib.KEY_N",lval_num(KEY_N));
    lval_constant(e,"raylib.KEY_O",lval_num(KEY_O));
    lval_constant(e,"raylib.KEY_P",lval_num(KEY_P));
    lval_constant(e,"raylib.KEY_Q",lval_num(KEY_Q));
    lval_constant(e,"raylib.KEY_R",lval_num(KEY_R));
    lval_constant(e,"raylib.KEY_S",lval_num(KEY_S));
    lval_constant(e,"raylib.KEY_T",lval_num(KEY_T));
    lval_constant(e,"raylib.KEY_U",lval_num(KEY_U));
    lval_constant(e,"raylib.KEY_V",lval_num(KEY_V));
    lval_constant(e,"raylib.KEY_W",lval_num(KEY_W));
    lval_constant(e,"raylib.KEY_X",lval_num(KEY_X));
    lval_constant(e,"raylib.KEY_Y",lval_num(KEY_Y));
    lval_constant(e,"raylib.KEY_Z",lval_num(KEY_Z));
    lval_constant(e,"raylib.KEY_LEFT_BRACKET",lval_num(KEY_LEFT_BRACKET));
    lval_constant(e,"raylib.KEY_BACKSLASH",lval_num(KEY_BACKSLASH));
    lval_constant(e,"raylib.KEY_RIGHT_BRACKET",lval_num(KEY_RIGHT_BRACKET));
    lval_constant(e,"raylib.KEY_GRAVE",lval_num(KEY_GRAVE));
    lval_constant(e,"raylib.KEY_SPACE",lval_num(KEY_SPACE));
    lval_constant(e,"raylib.KEY_ESCAPE",lval_num(KEY_ESCAPE));
    lval_constant(e,"raylib.KEY_ENTER",lval_num(KEY_ENTER));
    lval_constant(e,"raylib.KEY_TAB",lval_num(KEY_TAB));
    lval_constant(e,"raylib.KEY_BACKSPACE",lval_num(KEY_BACKSPACE));
    lval_constant(e,"raylib.KEY_INSERT",lval_num(KEY_INSERT));
    lval_constant(e,"raylib.KEY_DELETE",lval_num(KEY_DELETE));
    lval_constant(e,"raylib.KEY_RIGHT",lval_num(KEY_RIGHT));
    lval_constant(e,"raylib.KEY_LEFT",lval_num(KEY_LEFT));
    lval_constant(e,"raylib.KEY_DOWN",lval_num(KEY_DOWN));
    lval_constant(e,"raylib.KEY_UP",lval_num(KEY_UP));
    lval_constant(e,"raylib.KEY_PAGE_UP",lval_num(KEY_PAGE_UP));
    lval_constant(e,"raylib.KEY_PAGE_DOWN",lval_num(KEY_PAGE_DOWN));
    lval_constant(e,"raylib.KEY_HOME",lval_num(KEY_HOME));
    lval_constant(e,"raylib.KEY_END",lval_num(KEY_END));
    lval_constant(e,"raylib.KEY_CAPS_LOCK",lval_num(KEY_CAPS_LOCK));
    lval_constant(e,"raylib.KEY_SCROLL_LOCK",lval_num(KEY_SCROLL_LOCK));
    lval_constant(e,"raylib.KEY_NUM_LOCK",lval_num(KEY_NUM_LOCK));
    lval_constant(e,"raylib.KEY_PRINT_SCREEN",lval_num(KEY_PRINT_SCREEN));
    lval_constant(e,"raylib.KEY_PAUSE",lval_num(KEY_PAUSE));
    lval_constant(e,"raylib.KEY_F1",lval_num(KEY_F1));
    lval_constant(e,"raylib.KEY_F2",lval_num(KEY_F2));
    lval_constant(e,"raylib.KEY_F3",lval_num(KEY_F3));
    lval_constant(e,"raylib.KEY_F4",lval_num(KEY_F4));
    lval_constant(e,"raylib.KEY_F5",lval_num(KEY_F5));
    lval_constant(e,"raylib.KEY_F6",lval_num(KEY_F6));
    lval_constant(e,"raylib.KEY_F7",lval_num(KEY_F7));
    lval_constant(e,"raylib.KEY_F8",lval_num(KEY_F8));
    lval_constant(e,"raylib.KEY_F9",lval_num(KEY_F9));
    lval_constant(e,"raylib.KEY_F10",lval_num(KEY_F10));
    lval_constant(e,"raylib.KEY_F11",lval_num(KEY_F11));
    lval_constant(e,"raylib.KEY_F12",lval_num(KEY_F12));
    lval_constant(e,"raylib.KEY_LEFT_SHIFT",lval_num(KEY_LEFT_SHIFT));
    lval_constant(e,"raylib.KEY_LEFT_CONTROL",lval_num(KEY_LEFT_CONTROL));
    lval_constant(e,"raylib.KEY_LEFT_ALT",lval_num(KEY_LEFT_ALT));
    lval_constant(e,"raylib.KEY_LEFT_SUPER",lval_num(KEY_LEFT_SUPER));
    lval_constant(e,"raylib.KEY_RIGHT_SHIFT",lval_num(KEY_RIGHT_SHIFT));
    lval_constant(e,"raylib.KEY_RIGHT_CONTROL",lval_num(KEY_RIGHT_CONTROL));
    lval_constant(e,"raylib.KEY_RIGHT_ALT",lval_num(KEY_RIGHT_ALT));
    lval_constant(e,"raylib.KEY_RIGHT_SUPER",lval_num(KEY_RIGHT_SUPER));
    lval_constant(e,"raylib.KEY_KB_MENU",lval_num(KEY_KB_MENU));
    lval_constant(e,"raylib.KEY_KP_0",lval_num(KEY_KP_0));
    lval_constant(e,"raylib.KEY_KP_1",lval_num(KEY_KP_1));
    lval_constant(e,"raylib.KEY_KP_2",lval_num(KEY_KP_2));
    lval_constant(e,"raylib.KEY_KP_3",lval_num(KEY_KP_3));
    lval_constant(e,"raylib.KEY_KP_4",lval_num(KEY_KP_4));
    lval_constant(e,"raylib.KEY_KP_5",lval_num(KEY_KP_5));
    lval_constant(e,"raylib.KEY_KP_6",lval_num(KEY_KP_6));
    lval_constant(e,"raylib.KEY_KP_7",lval_num(KEY_KP_7));
    lval_constant(e,"raylib.KEY_KP_8",lval_num(KEY_KP_8));
    lval_constant(e,"raylib.KEY_KP_9",lval_num(KEY_KP_9));
    lval_constant(e,"raylib.KEY_KP_DECIMAL",lval_num(KEY_KP_DECIMAL));
    lval_constant(e,"raylib.KEY_KP_DIVIDE",lval_num(KEY_KP_DIVIDE));
    lval_constant(e,"raylib.KEY_KP_MULTIPLY",lval_num(KEY_KP_MULTIPLY));
    lval_constant(e,"raylib.KEY_KP_SUBTRACT",lval_num(KEY_KP_SUBTRACT));
    lval_constant(e,"raylib.KEY_KP_ADD",lval_num(KEY_KP_ADD));
    lval_constant(e,"raylib.KEY_KP_ENTER",lval_num(KEY_KP_ENTER));
    lval_constant(e,"raylib.KEY_KP_EQUAL",lval_num(KEY_KP_EQUAL));

    lval_constant(e,"raylib.MOUSE_BUTTON_LEFT",lval_num(MOUSE_BUTTON_LEFT));
    lval_constant(e,"raylib.MOUSE_BUTTON_RIGHT",lval_num(MOUSE_BUTTON_RIGHT));
    lval_constant(e,"raylib.MOUSE_BUTTON_MIDDLE",lval_num(MOUSE_BUTTON_MIDDLE));
    lval_constant(e,"raylib.MOUSE_BUTTON_SIDE",lval_num(MOUSE_BUTTON_SIDE));
    lval_constant(e,"raylib.MOUSE_BUTTON_EXTRA",lval_num(MOUSE_BUTTON_EXTRA));
    lval_constant(e,"raylib.MOUSE_BUTTON_FORWARD",lval_num(MOUSE_BUTTON_FORWARD));
    lval_constant(e,"raylib.MOUSE_BUTTON_BACK",lval_num(MOUSE_BUTTON_BACK));

    lval_constant(e,"raylib.FLAG_VSYNC_HINT",lval_num(FLAG_VSYNC_HINT));
    lval_constant(e,"raylib.FLAG_FULLSCREEN_MODE",lval_num(FLAG_FULLSCREEN_MODE));
    lval_constant(e,"raylib.FLAG_WINDOW_RESIZABLE",lval_num(FLAG_WINDOW_RESIZABLE));
    lval_constant(e,"raylib.FLAG_WINDOW_UNDECORATED",lval_num(FLAG_WINDOW_UNDECORATED));
    lval_constant(e,"raylib.FLAG_WINDOW_HIDDEN",lval_num(FLAG_WINDOW_HIDDEN));
    lval_constant(e,"raylib.FLAG_WINDOW_MINIMIZED",lval_num(FLAG_WINDOW_MINIMIZED));
    lval_constant(e,"raylib.FLAG_WINDOW_MAXIMIZED",lval_num(FLAG_WINDOW_MAXIMIZED));
    lval_constant(e,"raylib.FLAG_WINDOW_UNFOCUSED",lval_num(FLAG_WINDOW_UNFOCUSED));
    lval_constant(e,"raylib.FLAG_WINDOW_TOPMOST",lval_num(FLAG_WINDOW_TOPMOST));
    lval_constant(e,"raylib.FLAG_WINDOW_ALWAYS_RUN",lval_num(FLAG_WINDOW_ALWAYS_RUN));
    lval_constant(e,"raylib.FLAG_WINDOW_TRANSPARENT",lval_num(FLAG_WINDOW_TRANSPARENT));
    lval_constant(e,"raylib.FLAG_WINDOW_HIGHDPI",lval_num(FLAG_WINDOW_HIGHDPI));
    lval_constant(e,"raylib.FLAG_WINDOW_MOUSE_PASSTHROUGH",lval_num(FLAG_WINDOW_MOUSE_PASSTHROUGH));
    lval_constant(e,"raylib.FLAG_BORDERLESS_WINDOWED_MODE",lval_num(FLAG_BORDERLESS_WINDOWED_MODE));
    lval_constant(e,"raylib.FLAG_MSAA_4X_HINT",lval_num(FLAG_MSAA_4X_HINT));
    lval_constant(e,"raylib.FLAG_INTERLACED_HINT",lval_num(FLAG_INTERLACED_HINT));

    lval_constant(e,"raylib.MOUSE_CURSOR_DEFAULT",lval_num(MOUSE_CURSOR_DEFAULT));
    lval_constant(e,"raylib.MOUSE_CURSOR_ARROW",lval_num(MOUSE_CURSOR_ARROW));
    lval_constant(e,"raylib.MOUSE_CURSOR_IBEAM",lval_num(MOUSE_CURSOR_IBEAM));
    lval_constant(e,"raylib.MOUSE_CURSOR_CROSSHAIR",lval_num(MOUSE_CURSOR_CROSSHAIR));
    lval_constant(e,"raylib.MOUSE_CURSOR_POINTING_HAND",lval_num(MOUSE_CURSOR_POINTING_HAND));
    lval_constant(e,"raylib.MOUSE_CURSOR_RESIZE_EW",lval_num(MOUSE_CURSOR_RESIZE_EW));
    lval_constant(e,"raylib.MOUSE_CURSOR_RESIZE_NS",lval_num(MOUSE_CURSOR_RESIZE_NS));
    lval_constant(e,"raylib.MOUSE_CURSOR_RESIZE_NWSE",lval_num(MOUSE_CURSOR_RESIZE_NWSE));
    lval_constant(e,"raylib.MOUSE_CURSOR_RESIZE_NESW",lval_num(MOUSE_CURSOR_RESIZE_NESW));
    lval_constant(e,"raylib.MOUSE_CURSOR_RESIZE_ALL,",lval_num(MOUSE_CURSOR_RESIZE_ALL));
    lval_constant(e,"raylib.MOUSE_CURSOR_NOT_ALLOWED,",lval_num(MOUSE_CURSOR_NOT_ALLOWED));

    lenv_add_builtin(e,"raylib.init",builtin_raylib_init);
    lenv_add_builtin(e,"raylib.audio_init",builtin_raylib_audio_init);
    lenv_add_builtin(e,"raylib.set_fps",builitn_raylib_set_fps);
    lenv_add_builtin(e,"raylib.window_should_close",builtin_raylib_window_should_close);
    lenv_add_builtin(e,"raylib.begin_drawing",builtin_raylib_begin_drawing);
    lenv_add_builtin(e,"raylib.end_drawing",builtin_raylib_end_drawing);
    lenv_add_builtin(e,"raylib.clear_background",builtin_raylib_clear_background);
    lenv_add_builtin(e,"raylib.close",builtin_raylib_close);
    lenv_add_builtin(e,"raylib.audio_close",builtin_raylib_audio_close);
    lenv_add_builtin(e,"raylib.title",builtin_raylib_title);
    lenv_add_builtin(e,"raylib.icon",builtin_raylib_icon);
    lenv_add_builtin(e,"raylib.text",builtin_raylib_text);
    lenv_add_builtin(e,"raylib.text_ex",builtin_raylib_text_ex);
    lenv_add_builtin(e,"raylib.hide_logs",builtin_raylib_hide_logs);
    lenv_add_builtin(e,"raylib.show_logs",builtin_raylib_show_logs);
    lenv_add_builtin(e,"raylib.time",builtin_raylib_time);
    lenv_add_builtin(e,"raylib.is_ready",builtin_raylib_is_ready);
    lenv_add_builtin(e,"raylib.rectangle",builtin_raylib_rectangle);
    lenv_add_builtin(e,"raylib.get_size",builtin_raylib_get_size);
    lenv_add_builtin(e,"raylib.set_size",builtin_raylib_set_size);
    lenv_add_builtin(e,"raylib.is_resized",builtin_raylib_is_resized);
    lenv_add_builtin(e,"raylib.set_state",builtin_raylib_set_state);
    lenv_add_builtin(e,"raylib.fullscreen",builtin_raylib_fullscreen);
    lenv_add_builtin(e,"raylib.get_monitor_size",builtin_raylib_get_monitor_size);
    lenv_add_builtin(e,"raylib.maximize",builtin_raylib_maximize);
    lenv_add_builtin(e,"raylib.load_font",builtin_raylib_load_font);
    lenv_add_builtin(e,"raylib.unload_font",builtin_raylib_unload_font);
    lenv_add_builtin(e,"raylib.load_texture",builtin_raylib_load_texture);
    lenv_add_builtin(e,"raylib.paint_texture",builtin_raylib_paint_texture);
    lenv_add_builtin(e,"raylib.paint_texture_ex",builtin_raylib_paint_texture_ex);
    lenv_add_builtin(e,"raylib.unload_texture",builtin_raylib_unload_texture);
    lenv_add_builtin(e,"raylib.is_key_down",builtin_raylib_is_key_down);
    lenv_add_builtin(e,"raylib.is_key_pressed",builtin_raylib_is_key_pressed);
    lenv_add_builtin(e,"raylib.is_key_pressed_repeat",builitn_raylib_is_key_pressed_repeat);
    lenv_add_builtin(e,"raylib.get_key_pressed",builtin_raylib_get_key_pressed);
    lenv_add_builtin(e,"raylib.set_cursor",builtin_raylib_set_cursor);
    lenv_add_builtin(e,"raylib.is_mouse_button_down",builtin_raylib_is_mouse_button_down);
    lenv_add_builtin(e,"raylib.is_mouse_button_up",builtin_raylib_is_mouse_button_up);
    lenv_add_builtin(e,"raylib.is_mouse_button_pressed",builtin_raylib_is_mouse_button_pressed);
    lenv_add_builtin(e,"raylib.is_mouse_button_released",builtin_raylib_is_mouse_button_released);
    lenv_add_builtin(e,"raylib.get_mouse_pos",builtin_raylib_get_mouse_pos);
    lenv_add_builtin(e,"raylib.load_music",builtin_raylib_load_music);
    lenv_add_builtin(e,"raylib.unload_music",builtin_raylib_unload_music);
    lenv_add_builtin(e,"raylib.play_music",builtin_raylib_play_music);
    lenv_add_builtin(e,"raylib.stop_music",builtin_raylib_stop_music);
    lenv_add_builtin(e,"raylib.update_music",builtin_raylib_update_music);
    lenv_add_builtin(e,"raylib.pause_music",builtin_raylib_pause_music);
    lenv_add_builtin(e,"raylib.resume_music",builtin_raylib_resume_music);
    lenv_add_builtin(e,"raylib.is_music_playing",builtin_raylib_is_music_playing);
    lenv_add_builtin(e,"raylib.load_sound",builtin_raylib_load_sound);
    lenv_add_builtin(e,"raylib.unload_sound",builtin_raylib_unload_sound);
    lenv_add_builtin(e,"raylib.play_sound",builtin_raylib_play_sound);
    lenv_add_builtin(e,"raylib.stop_sound",builtin_raylib_stop_sound);
    lenv_add_builtin(e,"raylib.is_sound_playing",builtin_raylib_is_sound_playing);
}