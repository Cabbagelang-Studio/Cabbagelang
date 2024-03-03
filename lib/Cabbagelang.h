/*
MIT License

Copyright (c) 2024 Kangbo Hua

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifndef _CABBAGELANG_H
#define _CABBAGELANG_H 1

#define CABBAGELANG_VERSION "3.5.0"

#if !_WIN32
#if _POSIX_C_SOURCE < 200809L
#define _POSIX_C_SOURCE 200809L
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <malloc.h>
#include <stdint.h>
#include"mpc.h"

#if _WIN32
#ifndef setenv
int setenv(const char* name, const char* value, int overwrite) {
    if((!getenv(name))&&(!overwrite)) return 0;
    char*command=malloc(4+1+strlen(name)+1+strlen(value)+1);
    command[0]='\0';
    strcat(command,"setx ");
    strcat(command,name);
    strcat(command," ");
    strcat(command,value);
    freopen("nul","w",stdout);
    int result=system(command);
    freopen("con","w",stdout);
    free(command);
    return result;
}
#endif
#ifndef unsetenv
int unsetenv(const char* name) {
    if(!getenv(name)) return -1;
    return setenv(name,"\"\"",1);
}
#endif
#endif

int argc_glob;
int env_length=0;
char** argv_list;
char** env_list;

#ifdef _WIN32
#include<string.h>

static char buffer[2048];
char* readline(char* prompt){
    fputs(prompt,stdout);
    fgets(buffer,2048,stdin);
    char* cpy=malloc(strlen(buffer)+1);
    strcpy(cpy,buffer);
    cpy[strlen(cpy)-1]='\0';
    return cpy;
}
void add_history(char* unused){}

#else
#include <editline/readline.h>
#endif

//parser
mpc_parser_t* Int;
mpc_parser_t* Float;
mpc_parser_t* Number;
mpc_parser_t* Symbol;
mpc_parser_t* Sexpr;
mpc_parser_t* Qexpr;
mpc_parser_t* Expr;
mpc_parser_t* String;
mpc_parser_t* Comment;
mpc_parser_t* Lispy;

/**/
struct lval;
struct lenv;
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

lenv* CABBAGELANG_DEFAULT_ENVIRONMENT; 

#define  THREAD_IMPLEMENTATION
#include"thread.h"

thread_ptr_t* thread_list=NULL;
int thread_list_index=0;

#include"mongoose.h"

char* ltype_name(int t){
    switch(t){
        case LVAL_FUN: return "Function";
        case LVAL_NUM: return "Number";
        case LVAL_ERR: return "Error";
        case LVAL_SYM: return "Symbol";
        case LVAL_SEXPR: return "S-Expression";
        case LVAL_QEXPR: return "Q-Expression";
        case LVAL_STR: return "String";
        default: return "Unknown";
    }
}

void lval_print_str(lval*);
void lval_expr_print(lval*,char,char);
void lval_print(lval*);
void lval_constant(lenv*,char*,lval*);
lenv* lenv_new();
lenv* lenv_copy(lenv*);

/*
 * construct lval
*/
lval* lval_num(double x){
    lval* v=(lval* )malloc(sizeof(lval));
    v->type=LVAL_NUM;
    v->num=x;
    return v;
}

lval* lval_err(char* fmt,...){
    lval* v=(lval* )malloc(sizeof(lval));
    v->type=LVAL_ERR;

    va_list va;
    va_start(va,fmt);

    v->err=(char*)malloc(512);
    vsnprintf(v->err,511,fmt,va);
    v->err=(char*)realloc(v->err,strlen(v->err)+1);

    va_end(va);
    return v;
}

lval* lval_sym(char* s){
    lval* v=(lval* )malloc(sizeof(lval));
    v->type=LVAL_SYM;
    v->sym=(char* )malloc(strlen(s)+1);
    strcpy(v->sym,s);
    return v;
}

lval* lval_sexpr(){
    lval* v=(lval* )malloc(sizeof(lval));
    v->type=LVAL_SEXPR;
    v->count=0;
    v->cell=NULL;
    return v;
}

lval* lval_qexpr(){
    lval* v=(lval*)malloc(sizeof(lval));
    v->type=LVAL_QEXPR;
    v->count=0;
    v->cell=NULL;
    return v;
}

lval* lval_fun(lbuiltin func){
    lval *v=(lval*)malloc(sizeof(lval));
    v->type=LVAL_FUN;
    v->builtin=func;
    return v;
}

lval* lval_builtin(lbuiltin func){
    lval* v=(lval*)malloc(sizeof(lval));
    v->type=LVAL_FUN;
    v->builtin=func;
    return v;
}

lval* lval_lambda(lval* foramls,lval* body){
    lval* v=(lval*)malloc(sizeof(lval));
    v->type=LVAL_FUN;
    v->builtin=NULL;
    v->env=lenv_new();
    v->foramls=foramls;
    v->body=body;
    return v;
}

lval* lval_str(char* s){
    lval* v=(lval*)malloc(sizeof(lval));
    v->type=LVAL_STR;
    v->str=(char*)malloc(strlen(s)+1);
    strcpy(v->str,s);
    return v;
}

/*
 * construct lenv
 */

lenv* lenv_new(){
    lenv* e=(lenv*)malloc(sizeof(lenv));
    e->par=NULL;
    e->count=0;
    e->syms=NULL;
    e->vals=NULL;
    return e;
}

/*
 * delete
 */
void lenv_del(lenv*);

void lval_del(lval* v){
    switch(v->type){
        case LVAL_FUN:
            if(!v->builtin){
                lenv_del(v->env);
                lval_del(v->foramls);
                lval_del(v->body);
            }
            break;
        case LVAL_NUM:break;
        case LVAL_ERR:free(v->err);break;
        case LVAL_SYM:free(v->sym);break;
        case LVAL_STR:free(v->str);break;
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            for(int i=0;i<v->count;i++){
                lval_del(v->cell[i]);
            }
            free(v->cell);
            break;
    }
    free(v);
}

void lenv_del(lenv* e){
    for(int i=0;i<e->count;i++){
        free(e->syms[i]);
        lval_del(e->vals[i]);
    }
    free(e->syms);
    free(e->vals);
    free(e);
}

//basic func

int lval_eq(lval* x,lval* y){
    if(x->type!=y->type){
        return 0;
    }

    switch(x->type){
        case LVAL_NUM:return (x->num==y->num);
        case LVAL_ERR:return (strcmp(x->err,y->err)==0);
        case LVAL_SYM:return (strcmp(x->sym,y->sym)==0);
        case LVAL_STR:return (strcmp(x->str,y->str)==0);
        case LVAL_FUN:
            if(x->builtin||y->builtin){
                return x->builtin==y->builtin;
            }else{
                return lval_eq(x->foramls,y->foramls)&&
                    lval_eq(x->body,y->body);
            }
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            if(x->count!=y->count)
                return 0;
            for(int i=0;i<x->count;i++)
                if(!lval_eq(x->cell[i],y->cell[i]))
                    return 0;
            return 1;
        break;
    }
    return 0;
}

lval* lval_read_num(mpc_ast_t* t){
    errno=0;
    double x=strtod(t->contents,NULL);
    return errno!=ERANGE?
        lval_num(x):lval_err("invalid number");
}

//
lval* lval_read_str(mpc_ast_t* t){
    t->contents[strlen(t->contents)-1]='\0';
    char* unescaped=(char*)malloc(strlen(t->contents+1)+1);//1
    strcpy(unescaped,t->contents+1);
    unescaped=(char*)mpcf_unescape(unescaped);
    lval* str=lval_str(unescaped);
    free(unescaped);
    return str;
}

lval* lval_add(lval* v,lval* x){
    v->count++;
    v->cell=(struct lval**)realloc(v->cell,sizeof(lval*)*v->count);
    v->cell[v->count-1]=x;
    return v;
}

lval* lval_read(mpc_ast_t* t){
    if(strstr(t->tag,"number")){return lval_read_num(t);}
    if(strstr(t->tag,"symbol")){return lval_sym(t->contents);}
    if(strstr(t->tag,"string")){return lval_read_str(t);}

    lval* x=NULL;
    //if root (>) or sexpr, create empty list
    if(strcmp(t->tag,">")==0)   {x=lval_sexpr();}
    if(strcmp(t->tag,"sexpr"))  {x=lval_sexpr();}
    if(strstr(t->tag,"qexpr"))  {x=lval_qexpr();}

    //fill the list with any valid expression ontained within
    for(int i=0;i<t->children_num;i++){
        char* content=t->children[i]->contents;
        if(strcmp(content,"(")==0){continue;}
        if(strcmp(content,")")==0){continue;}
        if(strcmp(content,"{")==0){continue;}
        if(strcmp(content,"}")==0){continue;}
        if(strstr(t->children[i]->tag,"comment")){continue;}
        if(strcmp(t->children[i]->tag,"regex")==0){continue;}
        x=lval_add(x,lval_read(t->children[i]));
    }

    return x;
}

/*
 * environment
 * numberfunc
 * 
*/
lval* lval_copy(lval* v){
    lval* x=(lval*)malloc(sizeof(lval));
    x->type=v->type;

    switch(v->type){

        case LVAL_NUM:x->num=v->num;break;
        case LVAL_FUN:
            if(v->builtin){
                x->builtin=v->builtin;
            }else{
                x->builtin=NULL;
                x->env=lenv_copy(v->env);
                x->foramls=lval_copy(v->foramls);
                x->body=lval_copy(v->body);
            }
            break;

        case LVAL_ERR:
            x->err=(char*)malloc(strlen(v->err)+1);
            strcpy(x->err,v->err);
            break;

        case LVAL_SYM:
            x->sym=(char*)malloc(strlen(v->sym)+1);
            strcpy(x->sym,v->sym);
            break;

        case LVAL_STR:
            x->str=(char*)malloc(strlen(v->str)+1);
            strcpy(x->str,v->str);
            break;

        case LVAL_SEXPR:
        case LVAL_QEXPR:
            x->count=v->count;
            x->cell=(lval**)malloc(sizeof(lval*)*x->count);
            for(int i=0;i<x->count;i++)
                x->cell[i]=lval_copy(v->cell[i]);
            break;
    }
    return x;
}

lenv* lenv_copy(lenv* e){
    lenv* n=(lenv*)malloc(sizeof(lenv));
    n->par=e->par;
    n->count=e->count;
    n->syms=(char**)malloc(sizeof(char*)*n->count);
    n->vals=(lval**)malloc(sizeof(lval*)*n->count);

    for(int i=0;i<e->count;i++){
        n->syms[i]=(char*)malloc(strlen(e->syms[i])+1);
        strcpy(n->syms[i],e->syms[i]);
        n->vals[i]=lval_copy(e->vals[i]);
    }

    return n;
}

//local
void lenv_put(lenv* e,lval* k,lval* v){

    for(int i=0;i<e->count;i++){
        //v
        if(strcmp(e->syms[i],k->sym)==0){
            lval_del(e->vals[i]);
            e->vals[i]=lval_copy(v);
            return;
        }
    }

    //
    e->count++;
    e->vals=(lval**)realloc(e->vals,sizeof(lval*)*e->count);
    e->syms=(char**)realloc(e->syms,sizeof(char*)*e->count);

    e->vals[e->count-1]=lval_copy(v);
    e->syms[e->count-1]=(char*)malloc(strlen(k->sym)+1);
    strcpy(e->syms[e->count-1],k->sym);
}

//global
void lenv_def(lenv* e,lval* k,lval* v){
    while(e->par){e=e->par;}
    lenv_put(e,k,v);
}


lval* lenv_get(lenv* e,lval* k){
    for(int i=0;i<e->count;i++){
        if(strcmp(e->syms[i],k->sym)==0)
            return lval_copy(e->vals[i]);
    }

    //
    if(e->par){
        return lenv_get(e->par,k);
    }else{
    	if(k->sym[0]=='#') return lval_sexpr();
        return lval_err("Unbound Symbol '%s'",k->sym);
    }
}

void lval_print(lval* v){
    switch(v->type){
        case LVAL_FUN:
            if(v->builtin){
                printf("<builtin>");
            }else{
                printf("(\\ ");
                lval_print(v->foramls);
                putchar(' ');
                lval_print(v->body);
                putchar(')');
            }
            break;
        case LVAL_NUM:printf("%f",v->num);break;
        case LVAL_ERR:printf("Error : %s",v->err);break;
        case LVAL_SYM:printf("%s",v->sym);break;
        case LVAL_STR:lval_print_str(v);break;
        case LVAL_SEXPR:lval_expr_print(v,'(',')');break;
        case LVAL_QEXPR:lval_expr_print(v,'{','}');break;
    }
}

void lval_expr_print(lval* v,char open,char close){
    putchar(open);
    for(int i=0;i<v->count;i++){
        lval_print(v->cell[i]);
        if(i!=(v->count-1)){
            putchar(' ');
        }
    }
    putchar(close);
}


int string_count(char *original_data, char *key)
{
	int count = 0;
	int key_len = strlen (key);
	char *pos_start = original_data, *pos_end = NULL;
 
	while (NULL != 
		   (pos_end = strstr (pos_start, key)))
	{
		pos_start = pos_end + key_len;
		count++;
	}
 
	return count;
}

char *lval_string_replace(char *original_data, char *replaced, char *to)
{
	int rep_len = strlen (replaced);
	int to_len = strlen (to);
 	int counts = string_count (original_data, replaced);
	int m = strlen(original_data) + counts * (to_len - rep_len);
	char *new_buf = NULL;
		
	new_buf = (char *) malloc (m + 1);
	if (NULL == new_buf)
	{
		printf ("malloc error\n");
		return NULL;
	}
 
	memset (new_buf, 0, m + 1);
 
	char *pos_start = original_data, *pos_end = NULL, *pbuf = new_buf;
	int copy_len = 0;
 
	while (NULL != (pos_end = strstr (pos_start, replaced)))
	{
		copy_len = pos_end - pos_start;		
		strncpy (pbuf, pos_start, copy_len);
		pbuf += copy_len;					
		strncpy (pbuf, to, strlen(to));     
		pbuf += to_len;						
		pos_start = pos_end + rep_len;		
	}
	strncpy (pbuf, pos_start, strlen(pos_start) + 1);
 
	return new_buf;
}

void lval_print_str(lval* v){
    char* escaped=(char*)malloc(strlen(v->str)+1);
    strcpy(escaped,v->str);
    printf("%s",escaped);
    free(escaped);
}

void lval_println(lval* v){
    lval_print(v);
    putchar('\n');
}

/*******lval*******/
lval* lval_eval(lenv*,lval*);
lval* lval_eval_sexpr(lenv*,lval*);
lval* lval_pop(lval*,int);//Si
lval* lval_take(lval*,int);//
//lval* builtin_op(lenv*,lval*,char*);
//lval* builtin(lval*,char*);
lval* builtin_eval(lenv*,lval*);
lval* builtin_list(lenv* ,lval* );
lval* lval_call(lenv*,lval*,lval*);

lval* lval_pop(lval* v,int i){
    lval* x=v->cell[i];
    memmove(&v->cell[i],&v->cell[i+1],
            sizeof(lval*)*(v->count-i-1));
    v->count--;
    v->cell=(lval**)realloc(v->cell,sizeof(lval*)*v->count);
    return x;
}

lval* lval_take(lval* v,int i){
    lval* x=lval_pop(v,i);
    lval_del(v);
    return x;
}


lval* lval_eval(lenv* e,lval* v){
    //evaluate sexpressions
    if(v->type==LVAL_SYM){
        lval* x=lenv_get(e,v);
        lval_del(v);
        return x;
    }
    if(v->type==LVAL_SEXPR){return lval_eval_sexpr(e,v);}
    //all other lval types reamin the same
    return v;
}

lval* lval_eval_sexpr(lenv* e,lval* v){

    //evaluate children
    for(int i=0;i<v->count;i++){
        v->cell[i]=lval_eval(e,v->cell[i]);
    }
    //error
    for(int i=0;i<v->count;i++){
        if(v->cell[i]->type==LVAL_ERR){
            return lval_take(v,i);
        }
    }

    //empty
    if(v->count==0){return v;}
    //single
    if(v->count==1){
        return lval_eval(e,lval_take(v,0));
    }

    //function
    lval* f=lval_pop(v,0);
    if(f->type==LVAL_SEXPR){
    	return lval_sexpr();
	}
    if(f->type!=LVAL_FUN){
        lval* err = lval_err(
            "S-Expression starts with incorrect type. "
            "Got %s, Expected %s.",
            ltype_name(f->type), ltype_name(LVAL_FUN));
        lval_del(f);
        lval_del(v);
        return err;
    }

    //call function to get result
    lval* result=lval_call(e,f,v);
    lval_del(f);
    return result;
}

lval* lval_call(lenv* e,lval* f,lval* a){
    if(f->builtin){return f->builtin(e,a);}

    int given=a->count;
    int total=f->foramls->count;

    //arguments
    while(a->count){
        if(f->foramls->count==0){
            lval_del(a);
            return lval_err("Function passed too many arguments."
                "Got %i,Expected %i.",given,total);
        }

        lval* sym=lval_pop(f->foramls,0);
        //
        if(strcmp(sym->sym,"&")==0){
            if(f->foramls->count!=1){
                lval_del(a);
                return lval_err("Function format invalid."
                            "Symbol '&' not followed by single symbol");
            }
            lval* nsym=lval_pop(f->foramls,0);
            lenv_put(f->env,nsym,builtin_list(e,a));
            lval_del(sym);
            lval_del(nsym);
            break;
        }
        //
        lval* val=lval_pop(a,0);
        lenv_put(f->env,sym,val);//bind
        lval_del(sym);
        lval_del(val);
    }
    lval_del(a);
    //
    if(f->foramls->count>0&&
       strcmp(f->foramls->cell[0]->sym,"&")==0){
        if(f->foramls->count!=2){
            return lval_err("Function format invalid."
                        "Symbol '&' not followed by single symbol");
        }

        lval_del(lval_pop(f->foramls,0));
        lval* sym=lval_pop(f->foramls,0);
        lval* val=lval_qexpr();

        lenv_put(f->env,sym,val);
        lval_del(sym);
        lval_del(val);
    }

    if(f->foramls->count==0){
        f->env->par=e;
        return builtin_eval(f->env,
                        lval_add(lval_sexpr(),lval_copy(f->body)));
    }else{
        return lval_copy(f);
    }
}

/*******Qexpr*******/
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

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <wininet.h>

lval* http_request(const char* hostname, int port, const char* request) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return lval_err("WSAStartup failed: %d", WSAGetLastError());
    }

    SOCKET Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct hostent* host;
    host = gethostbyname(hostname);

    if (host == NULL) {
        WSACleanup();
        return lval_err("Error resolving host");
    }

    SOCKADDR_IN SockAddr;
    SockAddr.sin_port = htons(port);
    SockAddr.sin_family = AF_INET;
    SockAddr.sin_addr.s_addr = *((unsigned long*)host->h_addr);

    if (connect(Socket, (SOCKADDR*)(&SockAddr), sizeof(SockAddr)) != 0) {
        WSACleanup();
        return lval_err("Error creating socket: %d", WSAGetLastError());
    }

    send(Socket, request, strlen(request), 0);

    char* x = malloc(1);
    x[0] = '\0';

    char buffer[1024];
    int nDataLength;

    while ((nDataLength = recv(Socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[nDataLength] = '\0';

        char* tmp = realloc(x, strlen(x) + nDataLength + 1);
        if (tmp == NULL) {
            free(x);
            return lval_err("Memory allocation failed");
        }
        x = tmp;

        strcat(x, buffer);
    }

    closesocket(Socket);
    WSACleanup();

    lval* result = lval_str(x);


    free(x);

    return result;
}
lval* download_file(const char *url, const char *destination) {
    HINTERNET hInternet, hConnect;

    hInternet = InternetOpen(L"Download", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) {
        return lval_err("InternetOpen failed.");
    }

    hConnect = InternetOpenUrlA(hInternet, url, NULL, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hConnect) {
        InternetCloseHandle(hInternet);
        return lval_err("InternetOpenUrlA failed.");
    }

    FILE *file = fopen(destination, "wb");
    if (!file) {
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return lval_err("Failed to open file %s.",destination);
    }

    char buffer[4096];
    DWORD bytesRead;

    while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        fwrite(buffer, 1, bytesRead, file);
    }

    fclose(file);

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    return lval_sexpr();
}
#else

#include<string.h>

#include<sys/socket.h>
#include<sys/select.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<netdb.h>
#include <fcntl.h>

#define BUFFER_SIZE 4096


char* host_to_ip(const char* hostname){
    struct hostent *host_entry=gethostbyname(hostname);
    if(host_entry){
        return inet_ntoa(*(struct in_addr*)*host_entry->h_addr_list);
    }
    return NULL;
}


int http_create_socket(char* ip,int port){
    int sockfd=socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in sin={0};
    sin.sin_family=AF_INET;
    sin.sin_port=htons(port);
    sin.sin_addr.s_addr=inet_addr(ip);
 
    // int ret=connect(sockfd,(sockaddr*)&sin,sizeof(sockaddr_in));
    // if(ret!=0) return -1;
    if (0 != connect(sockfd, (struct sockaddr*)&sin, sizeof(struct sockaddr_in))) {
        return -1;
    }
 
    fcntl(sockfd,F_SETFL,O_NONBLOCK);
    return sockfd;
}

char* http_request(const char* hostname,int port,const char* request){
    char* ip=host_to_ip(hostname);
    int sockfd=http_create_socket(ip,port);
 
    char buffer[BUFFER_SIZE]={0};
 
    send(sockfd,request,strlen(request),0);
 
    //select
    fd_set fdread;
    FD_ZERO(&fdread);
    FD_SET(sockfd,&fdread);
 
    struct timeval tv;
    tv.tv_sec=5;
    tv.tv_usec=0;
 
 
    char* result=(char*)malloc(sizeof(int));
    memset(result,0,sizeof(int));
    while(1){
        int selection=select(sockfd+1,&fdread,NULL,NULL,&tv);
        if(!selection||!FD_ISSET(sockfd,&fdread)){
            break;
        }else{
            memset(buffer,0,BUFFER_SIZE);
            int len=recv(sockfd,buffer,BUFFER_SIZE,0);
            if(len==0){//disconnect
                break;
            }
            result=(char*)realloc(result,(strlen(result)+len+1)*sizeof(char));
            strncat(result,buffer,len);
        }
    }
    return result;
 
}

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

#include <curl/curl.h>

int download_file(const char *url, const char *output_file) {
    CURL *curl;
    FILE *file;
    CURLcode res;

    curl = curl_easy_init();
    if (!curl) {
        return 1;
    }

    file = fopen(output_file, "wb");
    if (!file) {
        curl_easy_cleanup(curl);
        return 2;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);

    res = curl_easy_perform(curl);

    curl_easy_cleanup(curl);
    fclose(file);

    return res == CURLE_OK ? 0 : 3;
}

#endif

lval* builtin_error(lenv* e,lval* a){
    LASSERT_NUM("throw", a, 1);
    LASSERT_TYPE("throw", a, 0, LVAL_STR);
    lval* err=lval_err(a->cell[0]->str);
    lval_del(a);
    return err;
}

lval* builtin_op(lenv* e,lval* a,char* op){
    for(int i=0;i<a->count;i++){
        LASSERT_TYPE(op, a, i, LVAL_NUM);
    }

    lval* x=lval_pop(a,0);
    if((strcmp(op,"-")==0)&&a->count==0){
        x->num=-x->num;
    }

    while(a->count>0){
        lval* y=lval_pop(a,0);
        if(strcmp(op,"+")==0){x->num+=y->num;}
        if(strcmp(op,"-")==0){x->num-=y->num;}
        if(strcmp(op,"*")==0){x->num*=y->num;}
        if(strcmp(op,"/")==0){
            if(y->num==0){
                lval_del(x);
                lval_del(y);
                x=lval_err("Division by zero");
                break;
            }
            x->num/=y->num;
        }
        if(strcmp(op,"^")==0){x->num=pow(x->num,y->num);}
        if(strcmp(op,"%")==0){
            if(y->num==0){
                lval_del(x);
                lval_del(y);
                x=lval_err("Division by zero");
                break;
            }
            x->num=(int)x->num%(int)y->num;
        }
        if(strcmp(op,"#")==0){
            if(y->num==0){
                lval_del(x);
                lval_del(y);
                x=lval_err("Division by zero");
                break;
            }
            x->num=(int)(x->num/y->num);
        }
        lval_del(y);
    }
    lval_del(a);
    return x;
}

lval* builtin_head(lenv* e,lval* a){

    LASSERT_NUM("head", a, 1);
    LASSERT_TYPE("head", a, 0, LVAL_QEXPR);
    LASSERT_NOT_EMPTY("head", a, 0);

    lval* v=lval_take(a,0);
    while(v->count>1){
        lval_del(lval_pop(v,1));
    }
    return v;
}

lval* builtin_tail(lenv* e,lval* a){

    LASSERT_NUM("tail", a, 1);
    LASSERT_TYPE("tail", a, 0, LVAL_QEXPR);
    LASSERT_NOT_EMPTY("tail", a, 0);

    lval* v=lval_take(a,0);
    lval_del(lval_pop(v,0));
    return v;
}

lval* builtin_list(lenv* e,lval* a){
    a->type=LVAL_QEXPR;
    return a;
}

lval* builtin_eval(lenv* e,lval* a){

    LASSERT_NUM("eval", a, 1);
    LASSERT_TYPE("eval", a, 0, LVAL_QEXPR);

    lval* x=lval_take(a,0);
    x->type=LVAL_SEXPR;
    return lval_eval(e,x);
}


lval* lval_join(lval* x,lval* y){
    for(int i=0;i<y->count;i++){
        x=lval_add(x,y->cell[i]);
    }
    free(y->cell);
    free(y);
    return x;
}

lval* builtin_join(lenv* e,lval* a){
    for(int i=0;i<a->count;i++){
        LASSERT_TYPE("join", a, i, LVAL_QEXPR);
    }

    lval* x=lval_pop(a,0);
    while(a->count){
        lval* y=lval_pop(a,0);
        x=lval_join(x,y);
    }
    lval_del(a);
    return x;
}

lval* builtin_index(lenv* e,lval* a){
	LASSERT_NUM("index",a,2);
	LASSERT_TYPE("index",a,0,LVAL_QEXPR);
	LASSERT_TYPE("index",a,1,LVAL_NUM);
	int num=(int)a->cell[1]->num;
	lval* v=lval_take(a,0);
	if(num>=v->count||abs(num)>v->count){
		return lval_err("Out of range.");
	}if(num<0){
		lval* result=lval_pop(v,v->count+num);
		lval_del(v);
		return result;
	}else{
		lval* result=lval_pop(v,num);
		lval_del(v);
		return result;
	}
	
}

lval* builtin_len(lenv* e,lval* a){
	LASSERT_NUM("len",a,1);
	LASSERT_TYPE("len",a,0,LVAL_QEXPR);
	lval* v=lval_take(a,0);
	int length=v->count;
	lval_del(v);
	return lval_num(length);
}

lval* builtin_take(lenv* e,lval* a){
	LASSERT_NUM("take",a,2);
	LASSERT_TYPE("take",a,0,LVAL_QEXPR);
	LASSERT_TYPE("take",a,1,LVAL_NUM);
	lval* v=lval_pop(a,0);
	int index=lval_pop(a,0)->num;
	if(abs(index)>v->count){
		lval_del(a);
		lval_del(v);
		return lval_err("Invalid index.");
	}
	lval* x=lval_qexpr();
	for(int i=0;i<abs(index);i++){
		lval_add(x,lval_pop(v,index>0?0:(v->count-1)));
	}lval_del(a);
	lval_del(v);
	return x;
}

lval* builtin_drop(lenv* e,lval* a){
	LASSERT_NUM("drop",a,2);
	LASSERT_TYPE("drop",a,0,LVAL_QEXPR);
	LASSERT_TYPE("drop",a,1,LVAL_NUM);
	lval* v=lval_pop(a,0);
	int index=lval_pop(a,0)->num;
	if(abs(index)>v->count){
		lval_del(a);
		lval_del(v);
		return lval_err("Invalid index.");
	}
	for(int i=0;i<abs(index);i++){
		lval_pop(v,index>0?0:(v->count-1));
	}lval_del(a);
	return v;
}

lval* builtin_argv(lenv* e,lval* a){
	LASSERT_NUM("argv",a,1);
	LASSERT_TYPE("argv",a,0,LVAL_NUM);
	
	if(a->cell[0]->num>=argc_glob){
		return lval_sexpr(); 
	}
	char* _=argv_list[(int)a->cell[0]->num];
	_=lval_string_replace(_,"\\","\\\\0");
	lval*x=lval_str(_);
    lval_del(a);
    return x;
}

lval* builtin_env(lenv* e,lval* a){
	LASSERT_NUM("env",a,1);
	LASSERT_TYPE("env",a,0,LVAL_NUM);
	
	if(a->cell[0]->num>=env_length){
		return lval_sexpr(); 
	}
	
	char* _=env_list[(int)a->cell[0]->num];
	_=lval_string_replace(_,"\\","\\\0");
	lval*x=lval_str(_);
    lval_del(a);
    return x;
}

/*
lval* builtin(lval* a,char* func){
    if(strcmp("list",func)==0){return builtin_list(a);}
    if(strcmp("head",func)==0){return builtin_head(a);}
    if(strcmp("tail",func)==0){return builtin_tail(a);}
    if(strcmp("join",func)==0){return builtin_join(a);}
    if(strcmp("eval",func)==0){return builtin_eval(a);}
    if(strstr("+-*",func)){return builtin_op(a,func);}
    lval_del(a);
    return lval_err("Unknown Function");
}

int number_of_nodes(mpc_ast_t* t){
    if(t->children_num == 0)
        return 1;
    int total=1;
    for(int i=0;i<t->children_num;i++){
        total+=number_of_nodes(t->children[i]);
    }
    return total;
}
*/

/*logic*/
lval* builtin_ord(lenv* e,lval* a,char* op){
    LASSERT_NUM(op, a, 2);
    LASSERT_TYPE(op, a, 0, LVAL_NUM);
    LASSERT_TYPE(op, a, 1, LVAL_NUM);

    int r;
    if(strcmp(op,">")==0){
        r=(a->cell[0]->num>a->cell[1]->num);
    }
    if(strcmp(op,"<")==0){
        r=(a->cell[0]->num<a->cell[1]->num);
    }
    if(strcmp(op,">=")==0){
        r=(a->cell[0]->num>=a->cell[1]->num);
    }
    if(strcmp(op,"<=")==0){
        r=(a->cell[0]->num<=a->cell[1]->num);
    }
    lval_del(a);
    return lval_num(r);
}

lval* builtin_cmp(lenv* e,lval* a,char* op){
    LASSERT_NUM(op,a,2);
    int r;
    if(strcmp(op,"==")==0){
        r=lval_eq(a->cell[0],a->cell[1]);
    }
    if(strcmp(op,"!=")==0){
        r=!lval_eq(a->cell[0],a->cell[1]);
    }
    lval_del(a);
    return lval_num(r);
}

lval* builtin_eq(lenv* e,lval* a){
    return builtin_cmp(e,a,"==");
}

lval* builtin_ne(lenv* e,lval* a){
    return builtin_cmp(e,a,"!=");
}

lval* builtin_gt(lenv* e, lval* a) {
    return builtin_ord(e, a, ">");
}

lval* builtin_lt(lenv* e, lval* a) {
    return builtin_ord(e, a, "<");
}

lval* builtin_ge(lenv* e, lval* a) {
    return builtin_ord(e, a, ">=");
}

lval* builtin_le(lenv* e, lval* a) {
    return builtin_ord(e, a, "<=");
}

lval* builtin_if(lenv* e,lval* a){
    LASSERT_NUM("if", a, 3);
    LASSERT_TYPE("if", a, 0, LVAL_NUM);
    LASSERT_TYPE("if", a, 1, LVAL_QEXPR);
    LASSERT_TYPE("if", a, 2, LVAL_QEXPR);

    lval*x;
    a->cell[1]->type=LVAL_SEXPR;
    a->cell[2]->type=LVAL_SEXPR;

    if(a->cell[0]->num){
        x=lval_eval(e,lval_pop(a,1));
    }else{
        x=lval_eval(e,lval_pop(a,2));
    }

    lval_del(a);
    return x;
}

lval* builtin_while(lenv* e,lval* a){
	LASSERT_NUM("while", a, 2);
    LASSERT_TYPE("while", a, 0, LVAL_QEXPR);
    LASSERT_TYPE("while", a, 1, LVAL_QEXPR);
    lval*x=lval_sexpr();
    lval* condition;
	lval* loop;
	
	condition=lval_pop(a,0);
	loop=lval_pop(a,0);
	condition->type=LVAL_SEXPR;
	loop->type=LVAL_SEXPR;
	
	while(lval_eval(e,lval_copy(condition))->num){
		x=lval_eval(e,lval_copy(loop));
	}
	lval_del(condition);
	lval_del(loop);
	lval_del(a);
	return x;
	 
}

/*math*/
lval* builtin_add(lenv* e,lval* a){
    return builtin_op(e,a,"+");
}


lval* builtin_sub(lenv* e,lval* a){
    return builtin_op(e,a,"-");
}


lval* builtin_div(lenv* e,lval* a){
    return builtin_op(e,a,"/");
}

lval* builtin_mul(lenv* e,lval* a){
    return builtin_op(e,a,"*");
}

lval* builtin_pow(lenv* e,lval* a){
    return builtin_op(e,a,"^");
}

lval* builtin_mod(lenv* e,lval* a){
    return builtin_op(e,a,"%");
}

lval* builtin_imul(lenv* e,lval* a){
    return builtin_op(e,a,"#");
}

lval* builtin_min(lenv* e,lval* a){ 
	LASSERT_TYPE("min",a,0,LVAL_NUM);
	double minimal=a->cell[0]->num; 
	for(int i=1;i<a->count;i++){
		LASSERT_TYPE("min",a,i,LVAL_NUM);
		if(a->cell[i]->num<minimal)minimal=a->cell[i]->num; 
	}lval_del(a);
	return lval_num(minimal);
}

lval* builtin_max(lenv* e,lval* a){ 
	LASSERT_TYPE("max",a,0,LVAL_NUM);
	double maximal=a->cell[0]->num; 
	for(int i=1;i<a->count;i++){
		LASSERT_TYPE("max",a,i,LVAL_NUM);
		if(a->cell[i]->num>maximal)maximal=a->cell[i]->num; 
	}lval_del(a);
	return lval_num(maximal);
}

void lenv_add_builtin(lenv* e,char* name,lbuiltin func){
    lval* k=lval_sym(name);
    lval* v=lval_fun(func);
    lenv_put(e,k,v);
    lval_del(k);
    lval_del(v);
}

lval* builtin_var(lenv* e,lval* a,char* func){
    LASSERT_TYPE(func,a,0,LVAL_QEXPR);
    lval* syms = a->cell[0];
    for (int i = 0; i < syms->count; i++) {
        LASSERT(a, (syms->cell[i]->type == LVAL_SYM),
                "Function '%s' cannot define non-symbol. "
                "Got %s, Expected %s.", func,
                ltype_name(syms->cell[i]->type),
                ltype_name(LVAL_SYM)
                )
    }

    LASSERT(a, (syms->count == a->count-1),
        "Function '%s' passed too many arguments for symbols. "
        "Got %i, Expected %i.", func, syms->count, a->count-1);

    for(int i=0;i<syms->count;i++){
        if(strcmp(func,"func")==0){
            lenv_def(e,syms->cell[i],a->cell[i+1]);
        }
        if(strcmp(func,"=")==0){
            lenv_put(e,syms->cell[i],a->cell[i+1]);
        }
    }
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_def(lenv* e,lval* a){
    return builtin_var(e,a,"func");
}

lval* builtin_put(lenv* e,lval* a){
    return builtin_var(e,a,"=");
}

void lval_constant(lenv* e,char* name,lval* value){
    lval* constant_name=lval_qexpr();
    constant_name=lval_add(constant_name,lval_sym(name));
    lval* arguments=lval_sexpr();
    arguments=lval_add(arguments,constant_name);
    arguments=lval_add(arguments,value);
    builtin_def(e,arguments);
}

lval* builtin_lambda(lenv* e,lval* a){
      /* Check Two arguments, each of which are Q-Expressions */
    LASSERT_NUM("\\", a, 2);
    LASSERT_TYPE("\\", a, 0, LVAL_QEXPR);
    LASSERT_TYPE("\\", a, 1, LVAL_QEXPR);

    for(int i=0;i<a->cell[0]->count;i++){
        LASSERT(a,(a->cell[0]->cell[i]->type==LVAL_SYM),
                "Can't define non-symbol. Got %s, Expected %s.",
                ltype_name(a->cell[0]->cell[i]->type),ltype_name(LVAL_SYM));
    }
    lval* foramls=lval_pop(a,0);
    lval* body=lval_pop(a,0);
    lval_del(a);

    return lval_lambda(foramls,body);
}

lval* builtin_print(lenv* e,lval* a){
    for(int i=0;i<a->count;i++){
        lval_print(a->cell[i]);
    }
    lval_del(a);
    return lval_sexpr();
}

#ifdef _CABBAGELANG_BUILTIN_LEAVES
void raylib_init(lenv* e);
#endif

lval* builtin_load(lenv* e,lval* a){
    LASSERT_NUM("import", a, 1);
    LASSERT_TYPE("import", a, 0, LVAL_STR);
    #ifdef _CABBAGELANG_BUILTIN_LEAVES
    //Builtin leaves
    if(!strcmp(a->cell[0]->str,"raylib")){
        raylib_init(e);
        return lval_sexpr();
    }
    #endif

    mpc_result_t r;
    if(mpc_parse_contents(a->cell[0]->str,Lispy,&r)){
        lval* expr=lval_read(r.output);
        mpc_ast_delete(r.output);

        while(expr->count){
            lval* x=lval_eval(e,lval_pop(expr,0));
            if(x->type==LVAL_ERR){
                lval_println(x);
            }
            lval_del(x);
        }
        lval_del(expr);
        lval_del(a);
        return lval_sexpr();
    }else{
        char* err_msg=mpc_err_string(r.error);
        mpc_err_delete(r.error);
        lval* err=lval_err("Could not import Library %s",err_msg);
        free(err_msg);
        lval_del(a);
        return err;
    }
}

lval* builtin_cs(lenv* e,lval* a){
	int length=1;
	for(int i=0;i<a->count;i++){
        LASSERT_TYPE(".",a,i,LVAL_STR);
		length+=strlen(a->cell[i]->str);
    }
    char x[length];
    strcpy(x,a->cell[0]->str);
    for(int i=1;i<a->count;i++){
    	strcat(x,a->cell[i]->str);
	}
	lval_del(a);
	return lval_str(x);
}
lval* builtin_substr(lenv* e,lval* a){
	LASSERT_NUM("!",a,2);
	LASSERT_TYPE("!",a,0,LVAL_STR);
	LASSERT_TYPE("!",a,1,LVAL_NUM);
    char* result;
    result=a->cell[0]->str;
    int index=a->cell[1]->num;
    char* this=result[index];
    lval* x=lval_str(&this);
    lval_del(a);
    return x;
}
lval* builtin_strlen(lenv* e, lval* a){
	LASSERT_NUM("strlen",a,1);
	LASSERT_TYPE("strlen",a,0,LVAL_STR);
    return lval_num(strlen(a->cell[0]->str));
}
lval* builtin_nts(lenv* e, lval* a){
	LASSERT_NUM("nts",a,1);
	LASSERT_TYPE("nts",a,0,LVAL_NUM);
    char x[512]={0};
    sprintf(x,"%lf",a->cell[0]->num);
    lval_del(a);
    return lval_str(x);
}
lval* builtin_stn(lenv* e, lval* a){
	LASSERT_NUM("stn",a,1);
	LASSERT_TYPE("stn",a,0,LVAL_STR);
    return lval_num(atof(a->cell[0]->str));
}
lval* builtin_ats(lenv* e,lval* a){
	LASSERT_NUM("ats",a,1);
	LASSERT_TYPE("ats",a,0,LVAL_QEXPR);
	lval* v=lval_take(a,0);
	char string[v->count];
	for(int i=0;i<v->count;i++){
		char character=v->cell[i]->num;
		string[i]=character;
	}lval_del(v);
	return lval_str(string);
}

lval* builtin_sta(lenv* e,lval* a){
	LASSERT_NUM("sta",a,1);
	LASSERT_TYPE("sta",a,0,LVAL_STR);
	char* string=a->cell[0]->str;
	lval* array=lval_qexpr();
	for(int i=0;i<strlen(string)+1;i++){
		char character=string[i];
		int ch_code=(int)character;
		array=lval_add(array,lval_num(ch_code));
	}lval_del(a);
	return array;
}

char* encodeBase64(char* str,int len){
	char base64[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	int encodeStrLen = 1 + (len/3)*4 ,k=0;
	encodeStrLen += len%3 ? 4 : 0;
    char* encodeStr = (char*)(malloc(sizeof(char)*encodeStrLen));
    for(int i=0;i<len;i++){
    	if(len - i >= 3){
            encodeStr[k++] = base64[(unsigned char)str[i]>>2];
    		encodeStr[k++] = base64[((unsigned char)str[i]&0x03)<<4 | (unsigned char)str[++i]>>4];
    		encodeStr[k++] = base64[((unsigned char)str[i]&0x0f)<<2 | (unsigned char)str[++i]>>6];
            encodeStr[k++] = base64[(unsigned char)str[i]&0x3f];
    	}else if(len-i == 2){
            encodeStr[k++] = base64[(unsigned char)str[i] >> 2];
            encodeStr[k++] = base64[((unsigned char)str[i]&0x03) << 4 | ((unsigned char)str[++i] >> 4)];
            encodeStr[k++] = base64[((unsigned char)str[i]&0x0f) << 2];
            encodeStr[k++] = '=';
    	}else{
    		encodeStr[k++] = base64[(unsigned char)str[i] >> 2];
            encodeStr[k++] = base64[((unsigned char)str[i] & 0x03) << 4];                                                                                                              //ĩβ���������ں�
            encodeStr[k++] = '=';
            encodeStr[k++] = '=';
    	}
    }
    encodeStr[k] = '\0';
    return encodeStr;
}
lval* decodeBase64(char* str,int len){
	char base64[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    char ascill[129];
    int k = 0;
    for(int i=0;i<64;i++){
        ascill[base64[i]] = k++;
    }
	int decodeStrlen = len / 4 * 3 + 1;
	char* decodeStr = (char*)malloc(sizeof(char)*decodeStrlen);
	k = 0;
	for(int i=0;i<len;i++){
        decodeStr[k++] = (ascill[str[i]] << 2) | (ascill[str[++i]] >> 4);
		if(str[i+1] == '='){
			break;
		}
        decodeStr[k++] = (ascill[str[i]] << 4) |  (ascill[str[++i]] >> 2);
		if(str[i+1] == '='){
			break;
		}
        decodeStr[k++] = (ascill[str[i]] << 6) | (ascill[str[++i]]);
	}
	lval* array=lval_qexpr();
	for(int i=0;i<k;i++){
		char character=decodeStr[i];
		int ch_code=(int)character;
		array=lval_add(array,lval_num(ch_code));
	}return array;
}

lval* builitn_b64encode(lenv* e,lval* a){
	LASSERT_NUM("b64encode",a,1);
	LASSERT_TYPE("b64encode",a,0,LVAL_QEXPR);
	lval* v=lval_take(a,0);
	char string[v->count];
	for(int i=0;i<v->count;i++){
		char character=v->cell[i]->num;
		string[i]=character;
	}
	char* encoded=encodeBase64(string,sizeof(string)/sizeof(char));
	lval_del(v);
	return lval_str(encoded);
}

lval* builtin_b64decode(lenv* e,lval* a){
	LASSERT_NUM("b64decode",a,1);
	LASSERT_TYPE("b64decode",a,0,LVAL_STR);
	char* string=a->cell[0]->str;
	lval* result=decodeBase64(string,strlen(string));
	lval_del(a);
	return result;
}
int str_index_of(const char *a, char *b)
{
	char *offset = (char*)strstr(a, b);
	return offset - a;
}
char *str_ndup (const char *str, size_t max)
{
    size_t len = strnlen (str, max);
    char *res = (char*)malloc (len + 1);
    if (res)
    {
        memcpy (res, str, len);
        res[len] = '\0';
    }
    return res;
}
char* get_until(char *haystack, char *until)
{
	int offset = str_index_of(haystack, until);
	return str_ndup(haystack, offset);
}
lval* builtin_getuntil(lenv* e,lval* a){
	LASSERT_NUM("getuntil",a,2);
	LASSERT_TYPE("getuntil",a,0,LVAL_STR);
	LASSERT_TYPE("getuntil",a,1,LVAL_STR);
	return lval_str(get_until(a->cell[0]->str,a->cell[1]->str));
}
lval* builtin_res_body(lenv* e,lval* a){
	LASSERT_NUM("res_body",a,1);
	LASSERT_TYPE("res_body",a,0,LVAL_STR);
	char* response=a->cell[0]->str;
	char *body = strstr(response, "\r\n\r\n");
	body = lval_string_replace(body, "\r\n\r\n", "");
	return lval_str(body);
}
lval* builtin_system(lenv* e,lval* a){
	LASSERT_NUM("system",a,1);
	LASSERT_TYPE("system",a,0,LVAL_STR);
    system(a->cell[0]->str);
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_chdir(lenv* e,lval* a){
	LASSERT_NUM("chdir",a,1);
	LASSERT_TYPE("chdir",a,0,LVAL_STR);
	chdir(a->cell[0]->str);
	lval_del(a);
	return lval_sexpr();
}

lval* builtin_getenv(lenv* e,lval* a){
	LASSERT_NUM("getenv",a,1);
	LASSERT_TYPE("getenv",a,0,LVAL_STR);
	char* value=getenv(a->cell[0]->str);
	if(!value){
        lval_del(a);
        return lval_sexpr();
    }
	lval_del(a);
	return lval_str(value);
} 

lval* builtin_putenv(lenv* e,lval* a){
	LASSERT_NUM("putenv",a,1);
	LASSERT_TYPE("putenv",a,0,LVAL_STR); 
	int result=putenv(a->cell[0]->str);
	lval_del(a);
	return lval_num((double)result);
}

lval* builtin_setenv(lenv* e,lval* a){
	LASSERT_NUM("setenv",a,3);
	LASSERT_TYPE("setenv",a,0,LVAL_STR);
	LASSERT_TYPE("setenv",a,1,LVAL_STR);
	LASSERT_TYPE("setenv",a,2,LVAL_NUM); 
	int result=setenv(a->cell[0]->str,a->cell[1]->str,a->cell[2]->num);
	lval_del(a);
	return lval_num((double)result);
}

lval* builtin_unsetenv(lenv* e,lval* a){
	LASSERT_NUM("unsetenv",a,1);
	LASSERT_TYPE("unsetenv",a,0,LVAL_STR);
	int result=unsetenv(a->cell[0]->str);
	lval_del(a);
	return lval_num((double)result);
}

lval* builtin_kin(lenv* e,lval* a){
	LASSERT_NUM("kin",a,1);
	LASSERT_TYPE("kin",a,0,LVAL_STR);
    char* input=readline(a->cell[0]->str);
    lval_del(a);
    return lval_str(input);
}


lval *readfile(char *path, int *length)
{
	FILE *pfile;
	char *data;
 
	pfile = fopen(path, "rb");
	if (pfile == NULL)
	{
		return lval_err("Unable to open file: %s.",path);
	}
	fseek(pfile, 0, SEEK_END);
	*length = ftell(pfile);
	data = (char *)malloc((*length + 1) * sizeof(char));
	rewind(pfile);
	*length = fread(data, 1, *length, pfile);
	data[*length] = '\0';
	fclose(pfile);
	lval* result=lval_str(data);
	return result;
}

lval* builtin_getall(lenv* e,lval* a){
	LASSERT_NUM("getall",a,1);
	LASSERT_TYPE("getall",a,0,LVAL_STR);
	int buffer=0;
	lval* input=readfile(a->cell[0]->str,&buffer);
	lval_del(a);
	return input;
}
long get_file_size(FILE *stream)
{
	long file_size = -1;
	long cur_offset = ftell(stream);
	if (cur_offset == -1) {
		printf("ftell failed :%s\n", strerror(errno));
		return -1;
	}
	if (fseek(stream, 0, SEEK_END) != 0) {
		printf("fseek failed: %s\n", strerror(errno));
		return -1;
	}
	file_size = ftell(stream);
	if (file_size == -1) {
		printf("ftell failed :%s\n", strerror(errno));
	}
	if (fseek(stream, cur_offset, SEEK_SET) != 0) {
		printf("fseek failed: %s\n", strerror(errno));
		return -1;
	}
	return file_size;
}
lval* builtin_sizeof(lenv* e,lval* a){
	LASSERT_NUM("sizeof",a,1);
	LASSERT_TYPE("sizeof",a,0,LVAL_STR);
	FILE *pFile;
	pFile = fopen(a->cell[0]->str, "rb");
	lval_del(a);
	long file_size=get_file_size(pFile);
	fclose(pFile);
	return lval_num(file_size);
}
lval* builtin_getbin(lenv* e,lval* a){
	LASSERT_NUM("getbin",a,1);
	LASSERT_TYPE("getbin",a,0,LVAL_STR);
	FILE *pFile;
	pFile = fopen(a->cell[0]->str, "rb");
	if(pFile == NULL){
		lval_del(a);
		return lval_err("Unable to open file: %s.",a->cell[0]->str);
	}
	lval* result=lval_qexpr();
	for(int i=0;i<get_file_size(pFile);i++){
		result=lval_add(result,lval_num(fgetc(pFile)));
	}
	fclose(pFile);
	lval_del(a);
	return result;
}
lval* builtin_putbin(lenv* e,lval* a){
	LASSERT_NUM("putbin",a,2);
	LASSERT_TYPE("putbin",a,0,LVAL_STR);
	LASSERT_TYPE("putbin",a,1,LVAL_QEXPR);
	FILE *pFile;
	pFile = fopen(a->cell[0]->str, "wb");
	if(pFile == NULL){
		lval_del(a);
		return lval_err("Unable to open file: %s.",a->cell[0]->str);
	}
	lval* v=lval_take(a,1);
	for(int i=0;i<v->count;i++){
		fputc((int)v->cell[i]->num,pFile);
	}
	fclose(pFile);
	lval_del(v);
	return lval_sexpr();
}
lval* builtin_addbin(lenv* e,lval* a){
	LASSERT_NUM("addbin",a,2);
	LASSERT_TYPE("addbin",a,0,LVAL_STR);
	LASSERT_TYPE("addbin",a,1,LVAL_QEXPR);
	FILE *pFile;
	pFile = fopen(a->cell[0]->str, "ab");
	if(pFile == NULL){
		lval_del(a);
		return lval_err("Unable to open file: %s.",a->cell[0]->str);
	}
	lval* v=lval_take(a,1);
	for(int i=0;i<v->count;i++){
		fputc((int)v->cell[i]->num,pFile);
	}
	fclose(pFile);
	lval_del(v);
	return lval_sexpr();
}

lval* builtin_stdin(lenv* e,lval* a){
	LASSERT_NUM("stdin",a,2);
	LASSERT_TYPE("stdin",a,0,LVAL_STR);
    freopen(a->cell[0]->str,a->cell[1]->str,stdin);
    lval_del(a);
    return lval_sexpr();
}
lval* builtin_stdout(lenv* e,lval* a){
	LASSERT_NUM("stdout",a,2);
	LASSERT_TYPE("stdout",a,0,LVAL_STR);
    freopen(a->cell[0]->str,a->cell[1]->str,stdout);
    lval_del(a);
    return lval_sexpr();
}
lval* builtin_stderr(lenv* e,lval* a){
	LASSERT_NUM("stdout",a,2);
	LASSERT_TYPE("stdout",a,0,LVAL_STR);
	freopen(a->cell[0]->str,a->cell[1]->str,stderr);
    lval_del(a);
    return lval_sexpr();
}
lval* builtin_exit(lenv* e,lval* a){
	LASSERT_NUM("exit",a,1);
	LASSERT_TYPE("exit",a,0,LVAL_NUM);
    exit(a->cell[0]->num);
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_time(lenv* e,lval* a){
	LASSERT_NUM("time",a,1);
	LASSERT_TYPE("time",a,0,LVAL_SEXPR);
    return lval_num(time(NULL));
}
lval* builtin_srand(lenv* e,lval* a){
	LASSERT_NUM("srand",a,1);
	LASSERT_TYPE("srand",a,0,LVAL_NUM);
    srand(a->cell[0]->num);
    lval_del(a);
    return lval_sexpr();
}
lval* builtin_rand(lenv* e,lval* a){
	LASSERT_NUM("rand",a,1);
	LASSERT_TYPE("rand",a,0,LVAL_SEXPR);
	lval_del(a);
	return lval_num(rand());
}
lval* builtin_delay(lenv* e,lval* a){
	LASSERT_NUM("delay",a,1);
	LASSERT_TYPE("delay",a,0,LVAL_NUM);
	sleep((int)a->cell[0]->num);
	lval_del(a);
	return lval_sexpr();
}
#ifdef _WIN32
lval* builtin_request(lenv* e,lval* a){
	LASSERT_NUM("request",a,3);
	LASSERT_TYPE("request",a,0,LVAL_STR);
	LASSERT_TYPE("request",a,1,LVAL_NUM);
	LASSERT_TYPE("request",a,2,LVAL_STR);
	return http_request(a->cell[0]->str,a->cell[1]->num,a->cell[2]->str);
}
lval* builtin_download(lenv* e,lval* a){
	LASSERT_NUM("download",a,2);
	LASSERT_TYPE("download",a,0,LVAL_STR);
	LASSERT_TYPE("download",a,1,LVAL_STR);
	return download_file(a->cell[0]->str,a->cell[1]->str);
}
#else
lval* builtin_request(lenv* e,lval* a){
	LASSERT_NUM("request",a,3);
	LASSERT_TYPE("request",a,0,LVAL_STR);
	LASSERT_TYPE("request",a,1,LVAL_NUM);
	LASSERT_TYPE("request",a,2,LVAL_STR);
	char* hostname=a->cell[0]->str;
	int port=a->cell[1]->num;
	char* request=a->cell[2]->str; 
	char* result=http_request(hostname,port,request);
	return lval_str(result);
}
lval* builtin_download(lenv* e,lval* a){
	LASSERT_NUM("download",a,2);
	LASSERT_TYPE("download",a,0,LVAL_STR);
	LASSERT_TYPE("download",a,1,LVAL_STR);
	int download=download_file(a->cell[0]->str,a->cell[1]->str);
	lval* result=NULL;
	switch(download){
		case 1:
			result=lval_err("Invalid URL format");
			break;
		case 2:
			result=lval_err("Failed to create socket");
			break;
		case 3:
			result=lval_err("Failed to resolve host name");
			break;
		case 4:
			result=lval_err("Failed to connect to the server");
			break;
		case 5:
			result=lval_err("Failed to send request");
			break;
		case 6:
			result=lval_err("Failed to open file %s for writing", a->cell[1]->str);
			break;
		default:
			result=lval_sexpr();
	}return result;
}
#endif
#ifdef _WIN32
lval* builtin_calldl(lenv* e,lval* a){
    if(a->count<2){
        lval_del(a);
        return lval_err("Function 'calldl' passed incorrect number of arguments. Got %d, Expected 2+.",a->count);
    }
	LASSERT_TYPE("calldl",a,0,LVAL_STR);
	LASSERT_TYPE("calldl",a,1,LVAL_STR);
	typedef lval* (*DLFunction)(lenv* e,lval* a);
	HINSTANCE DL=LoadLibrary(a->cell[0]->str);
	lval* arguments=lval_sexpr();
	for(int i=2;i<a->count;i++){
		lval_add(arguments,a->cell[i]);
	}
	DLFunction DLFun=(DLFunction)GetProcAddress(DL,a->cell[1]->str);
	lval* dl_result=DLFun(e,arguments);
	FreeLibrary(DL);
	return dl_result;
}
lval* builtin_extend(lenv*e,lval*a){
    LASSERT_NUM("extend",a,1);
    LASSERT_TYPE("extend",a,0,LVAL_STR);
    typedef void (*DLFunction)(lenv*e);
    char* extension=a->cell[0]->str;
    FILE* fp=fopen(extension,"rb");
    if(fp==NULL){
        char* new_extension=malloc(strlen(getenv("CABBAGELANG_HOME"))+strlen(extension)+8);
  	    sprintf(new_extension,"%s/leaves/%s",getenv("CABBAGELANG_HOME"),extension);
        extension=new_extension;
    }fclose(fp);
    HINSTANCE DL=LoadLibrary(extension);
    DLFunction DLFun=(DLFunction)GetProcAddress(DL,"init");
    DLFun(e);
    FreeLibrary(DL);
    return lval_sexpr();
}
#else
#include<dlfcn.h>

lval* builtin_calldl(lenv* e,lval* a){
    if(a->count<2){
        lval_del(a);
        return lval_err("Function 'calldl' passed incorrect number of arguments. Got %d, Expected 2+.",a->count);
    }
	LASSERT_TYPE("calldl",a,0,LVAL_STR);
	LASSERT_TYPE("calldl",a,1,LVAL_STR);
	lval* (*DLFun)(lenv* e,lval* a);
    char* extension=a->cell[0]->str;
    FILE* fp=fopen(extension,"rb");
    if(fp==NULL){
        char* new_extension=malloc(strlen(getenv("CABBAGELANG_HOME"))+strlen(extension)+8);
  	    sprintf(new_extension,"%s/leaves/%s",getenv("CABBAGELANG_HOME"),extension);
        extension=new_extension;
    }fclose(fp);
	void* DL=dlopen(extension,RTLD_LAZY);
	lval* arguments=lval_sexpr();
	for(int i=2;i<a->count;i++){
		lval_add(arguments,a->cell[i]);
	}
	DLFun=dlsym(DL,a->cell[1]->str);
	lval* dl_result=DLFun(e,arguments);
	dlclose(DL);
	lval* result=dl_result;
	return result;
}
lval* builtin_extend(lenv*e,lval*a){
    LASSERT_NUM("extend",a,1);
    LASSERT_TYPE("extend",a,0,LVAL_STR);
    void (*DLFun)(lenv*e);
    void* DL=dlopen(a->cell[0]->str,RTLD_LAZY);
    DLFun=dlsym(DL,"init");
    DLFun(e);
    dlclose(DL);
    return lval_sexpr();
}
#endif

lval* thread_proc(void* param){
	lval* result=builtin_eval(CABBAGELANG_DEFAULT_ENVIRONMENT,param);
	return result;
}

lval* builtin_cthread(lenv* e,lval* a){
	LASSERT_NUM("cthread",a,1);
	LASSERT_TYPE("cthread",a,0,LVAL_QEXPR);
	thread_ptr_t thread=thread_create(thread_proc, a, THREAD_STACK_SIZE_DEFAULT );
	thread_list=realloc(thread_list,(thread_list_index+1)*sizeof(thread_ptr_t));
	thread_list[thread_list_index]=thread;
	int index=thread_list_index++;
	return lval_num(index);
}

lval* builtin_jthread(lenv* e,lval* a){
	LASSERT_NUM("jthread",a,1);
	LASSERT_TYPE("jthread",a,0,LVAL_NUM);
	if(a->cell[0]->num>=thread_list_index||(int)thread_list[(int)a->cell[0]->num]==NULL) return lval_err("Unable to open thread %d.",(int)a->cell[0]->num);
	lval* result=thread_join(thread_list[(int)a->cell[0]->num]);
	return result;
}

lval* builtin_dthread(lenv* e,lval* a){
	LASSERT_NUM("dthread",a,1);
	LASSERT_TYPE("dthread",a,0,LVAL_NUM);
	if(a->cell[0]->num>=thread_list_index||(int)thread_list[(int)a->cell[0]->num]==NULL) return lval_err("Unable to open thread %d.",(int)a->cell[0]->num);
	thread_destroy(thread_list[(int)a->cell[0]->num]);
	thread_list[(int)a->cell[0]->num]=NULL;
	return lval_sexpr();
}

#include<signal.h>

static int s_debug_level = MG_LL_INFO;
static char *s_root_dir = ".";
static char *s_listening_address = "http://0.0.0.0:8000";
static char *s_enable_hexdump = "no";
static char *s_ssi_pattern = "#.html";

// Handle interrupts, like Ctrl-C
static int s_signo;
static void signal_handler(int signo) {
  s_signo = signo;
}

// Event handler for the listening connection.
// Simply serve static files from `s_root_dir`
static void mg_handle(struct mg_connection *c, int ev, void *ev_data) {
  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = ev_data, tmp = {0};
    struct mg_str unknown = mg_str_n("?", 1), *cl;
    struct mg_http_serve_opts opts = {0};
    opts.root_dir = s_root_dir;
    opts.ssi_pattern = s_ssi_pattern;
    mg_http_serve_dir(c, hm, &opts);
    mg_http_parse((char *) c->send.buf, c->send.len, &tmp);
    cl = mg_http_get_header(&tmp, "Content-Length");
    if (cl == NULL) cl = &unknown;
    MG_INFO(("%.*s %.*s %.*s %.*s", (int) hm->method.len, hm->method.ptr,
             (int) hm->uri.len, hm->uri.ptr, (int) tmp.uri.len, tmp.uri.ptr,
             (int) cl->len, cl->ptr));
  }
}

lval* builtin_mongoose(lenv* e,lval* a){
    LASSERT_NUM("mongoose",a,5);
    LASSERT_TYPE("mongoose",a,0,LVAL_NUM);
    LASSERT_TYPE("mongoose",a,1,LVAL_STR);
    LASSERT_TYPE("mongoose",a,2,LVAL_STR);
    LASSERT_TYPE("mongoose",a,3,LVAL_STR);
    LASSERT_TYPE("mongoose",a,4,LVAL_NUM);

    char path[MG_PATH_MAX] = ".";
    struct mg_mgr mgr;
    struct mg_connection *c;

    s_debug_level = a->cell[4]->num;
    s_root_dir = a->cell[2]->str;
    s_listening_address = a->cell[3]->str;
    s_enable_hexdump = (a->cell[0]->num)?"yes":"no";
    s_ssi_pattern = a->cell[1]->str;
    
    if (strchr(s_root_dir, ',') == NULL) {
        realpath(s_root_dir, path);
        s_root_dir = path;
    }

    // Initialise stuff
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    mg_log_set(s_debug_level);
    mg_mgr_init(&mgr);
    if ((c = mg_http_listen(&mgr, s_listening_address, mg_handle, &mgr)) == NULL) {
        MG_ERROR(("Cannot listen on %s. Use http://ADDR:PORT or :PORT",
                s_listening_address));
        lval_del(a);
        return lval_sexpr();
    }
    if (mg_casecmp(s_enable_hexdump, "yes") == 0) c->is_hexdumping = 1;

    // Start infinite event loop
    MG_INFO(("Cabbagelang: Mongoose started.\n"));
    MG_INFO(("Mongoose version : v%s", MG_VERSION));
    MG_INFO(("Listening on     : %s", s_listening_address));
    MG_INFO(("Web root         : [%s]", s_root_dir));
    while (s_signo == 0) mg_mgr_poll(&mgr, 1000);
    mg_mgr_free(&mgr);
    MG_INFO(("Exiting on signal %d", s_signo));

    lval_del(a);
    return lval_sexpr();
}

lval* builtin_copyright(lenv*e,lval*a){
    LASSERT_NUM("copyright",a,1);
    LASSERT_TYPE("copyright",a,0,LVAL_SEXPR);
    puts("Copyright (c) 2024 Cabbagelang Studio.\nAll rights reserved.");
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_license(lenv*e,lval*a){
    LASSERT_NUM("license",a,1);
    LASSERT_TYPE("license",a,0,LVAL_SEXPR);
    puts("MIT License\n\
\n\
Copyright (c) 2024 Kangbo Hua\n\
\n\
Permission is hereby granted, free of charge, to any person obtaining a copy\n\
of this software and associated documentation files (the \"Software\"), to deal\n\
in the Software without restriction, including without limitation the rights\n\
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n\
copies of the Software, and to permit persons to whom the Software is\n\
furnished to do so, subject to the following conditions:\n\
\n\
The above copyright notice and this permission notice shall be included in all\n\
copies or substantial portions of the Software.\n\
\n\
THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n\
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n\
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n\
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n\
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n\
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n\
SOFTWARE.");
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_exec(lenv*e,lval*a){
    LASSERT_NUM("exec",a,1);
    LASSERT_TYPE("exec",a,0,LVAL_STR);
    char* input=a->cell[0]->str;
    lval* Cabbagelang_load_string(lenv*e,const char*,const char*);
    lval* result=Cabbagelang_load_string(e,input,"<string>");
    lval_del(a);
    return result;
}

void lenv_add_builtins(lenv* e){

    lenv_add_builtin(e,"\\",builtin_lambda);
    lenv_add_builtin(e,"func",builtin_def);
    lenv_add_builtin(e,"=",builtin_put);
    /*list functions*/
    lenv_add_builtin(e, "list", builtin_list);
    lenv_add_builtin(e, "head", builtin_head);
    lenv_add_builtin(e, "tail", builtin_tail);
    lenv_add_builtin(e, "eval", builtin_eval);
    lenv_add_builtin(e, "join", builtin_join);
    lenv_add_builtin(e, "index", builtin_index);
    lenv_add_builtin(e, "len", builtin_len);
    lenv_add_builtin(e, "take", builtin_take);
    lenv_add_builtin(e, "drop", builtin_drop);

    /* Mathematical Functions */
    lenv_add_builtin(e, "+", builtin_add);
    lenv_add_builtin(e, "-", builtin_sub);
    lenv_add_builtin(e, "*", builtin_mul);
    lenv_add_builtin(e, "/", builtin_div);
    lenv_add_builtin(e, "**", builtin_pow);
    lenv_add_builtin(e, "%", builtin_mod);
    lenv_add_builtin(e, "//", builtin_imul);
    lenv_add_builtin(e, "min", builtin_min);
    lenv_add_builtin(e, "max", builtin_max);

    /* Logical Functions */
    lenv_add_builtin(e, "if", builtin_if);
    lenv_add_builtin(e, "while", builtin_while);
    lenv_add_builtin(e, "==", builtin_eq);
    lenv_add_builtin(e, "!=", builtin_ne);
    lenv_add_builtin(e, ">",  builtin_gt);
    lenv_add_builtin(e, "<",  builtin_lt);
    lenv_add_builtin(e, ">=", builtin_ge);
    lenv_add_builtin(e, "<=", builtin_le);

    /* String Functions */
    lenv_add_builtin(e, "import",  builtin_load);
    lenv_add_builtin(e, "throw", builtin_error);
    lenv_add_builtin(e, "output", builtin_print);
    lenv_add_builtin(e, ".", builtin_cs);
    lenv_add_builtin(e, "!", builtin_substr);
    lenv_add_builtin(e, "strlen", builtin_strlen);
    lenv_add_builtin(e, "nts", builtin_nts);
    lenv_add_builtin(e, "stn", builtin_stn);
    lenv_add_builtin(e, "ats", builtin_ats);
    lenv_add_builtin(e, "sta", builtin_sta);
    lenv_add_builtin(e, "b64encode", builitn_b64encode);
    lenv_add_builtin(e, "b64decode", builtin_b64decode);
    lenv_add_builtin(e, "getuntil", builtin_getuntil);
    lenv_add_builtin(e, "res_body", builtin_res_body);
    
    /* File Functions */
    lenv_add_builtin(e, "getall", builtin_getall);
    lenv_add_builtin(e, "sizeof", builtin_sizeof);
    lenv_add_builtin(e, "getbin", builtin_getbin);
    lenv_add_builtin(e, "putbin", builtin_putbin);
    lenv_add_builtin(e, "addbin", builtin_addbin);
	
	/* Environmental Functions */
	lenv_add_builtin(e, "getenv", builtin_getenv); 
	lenv_add_builtin(e, "putenv", builtin_putenv);
	lenv_add_builtin(e, "setenv", builtin_setenv);
	lenv_add_builtin(e, "unsetenv", builtin_unsetenv);
	
    /* System Functions */
    lenv_add_builtin(e, "system", builtin_system);
    lenv_add_builtin(e, "chdir", builtin_chdir);
    lenv_add_builtin(e, "argv", builtin_argv);
    lenv_add_builtin(e, "env", builtin_env);
    lenv_add_builtin(e, "kin", builtin_kin);
    lenv_add_builtin(e, "stdin", builtin_stdin);
    lenv_add_builtin(e, "stdout", builtin_stdout);
    lenv_add_builtin(e, "stderr", builtin_stderr);
    lenv_add_builtin(e, "exit", builtin_exit);
    lenv_add_builtin(e, "time", builtin_time);
    lenv_add_builtin(e, "srand", builtin_srand);
    lenv_add_builtin(e, "rand", builtin_rand);
    lenv_add_builtin(e, "delay", builtin_delay);
    lenv_add_builtin(e, "request", builtin_request);
    lenv_add_builtin(e, "download", builtin_download);
    lenv_add_builtin(e, "calldl", builtin_calldl);
    lenv_add_builtin(e, "extend", builtin_extend);
    lenv_add_builtin(e, "cthread", builtin_cthread);
    lenv_add_builtin(e, "jthread", builtin_jthread);
    lenv_add_builtin(e, "dthread", builtin_dthread);
    lenv_add_builtin(e, "mongoose", builtin_mongoose);
    lenv_add_builtin(e, "exec", builtin_exec);

    /* Information */
    lenv_add_builtin(e, "copyright", builtin_copyright);
    lenv_add_builtin(e, "license", builtin_license);
}

lenv* Cabbagelang_initialize(int argc,char* argv[],char* env[]){
	 /*
     * 
     * 
     * 
     * AST
     * 
     */
    Int       = mpc_new("int");
    Float     = mpc_new("float");
    Number    = mpc_new("number");
    Symbol    = mpc_new("symbol");
    Sexpr     = mpc_new("sexpr");//
    Qexpr     = mpc_new("qexpr");//Q{}
    Expr      = mpc_new("expr");
    String    = mpc_new("string");
    Comment   = mpc_new("comment");
    Lispy     = mpc_new("lispy");//

    //number int
    //intfloatfloatint
    //float

    /*
     * 
     * list:Q-
     * head:Q-Q-(car)
     * tail:Q-Q-(cdr)
     * join:Q-Q-
     * eval:Q-S-
     */
    mpca_lang(MPCA_LANG_DEFAULT,
              "                                                             \
                int         : /-?[0-9]+/;                                   \
                float       : /-?[0-9]+[.][0-9]+/;                          \
                string      : /\"(\\\\.|[^\"])*\"/ ;                        \
                comment     : /;[^\\r\\n]*/ ;                               \ 
                number      : <float> | <int>    ;                          \
                symbol      : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&%#.]+/;             \
                sexpr       : '(' <expr>* ')';                              \
                qexpr       : '{' <expr>* '}';                              \
                expr        : <number> |  <symbol> | <string> |             \
                              <comment> | <sexpr> | <qexpr>;                \
                lispy       : /^/ <expr>* /$/;                              \
              ",
              Number,Int,Float,Symbol,String,Comment,Sexpr,Qexpr,Expr,Lispy);
    /**********/
    
    argc_glob=argc;
    argv_list=argv;
    env_list=env;
    
    thread_list=malloc(1*sizeof(thread_ptr_t));
    
    for(;env_list[env_length];env_length++);

    lenv* e=lenv_new();
    CABBAGELANG_DEFAULT_ENVIRONMENT=e;
    lenv_add_builtins(e);
    
    mpc_result_t r;
	char* stdlib="(func {nil} {}) (func {true} 1) (func {false} 0) (func {fun} (\\ {f b} { func (head f) (\\ (tail f) b) })) (fun {unpack f l}{ eval (join (list f) l) }) (fun {pack f & xs} {f xs}) (func {curry} unpack) (func {uncurry} pack) (fun {do & l} { if (== l nil) {nil} {index l -1} }) (fun {let b} { ((\\ {_} b) ()) }) (fun {not x} {- 1 x}) (func {and} *) (func {or} +) (fun {split l n} {list (take l n) (drop l n)}) (fun {reverse l} {take l (- 0 (len l))}) (fun {filter f l} { do(= {result} nil) (= {i} 0) (while{< i (len l)}{ (if(unpack f (list(index l i))){ = {result} (join result (list(index l i))) }{}) (= {i} (+ i 1)) }) (let {result}) }) (fun {drop-while f l} { do(= {i} 0) (= {drop-while-flag} true) (while{and (< i (len l)) drop-while-flag} { (if(unpack f (list(index l i))){(= {i} (+ i 1))}{ = {drop-while-flag} false }) }) (drop l i) }) (fun {take-while f l} { do(= {i} 0) (= {take-while-flag} true) (while{and (< i (len l)) take-while-flag} { (if(unpack f (list(index l i))){(= {i} (+ i 1))}{ = {take-while-flag} false }) }) (take l i) }) (fun {map f l} { do(= {i} 0) (= {result} nil) (while {< i (len l)} { (= {result} (join result (list (f (index l i))))) (= {i} (+ i 1)) }) (let {result}) }) (fun {fold f l z} { do(= {i} 0) (while {< i (len l)} { (= {z} (f z (index l i))) (= {i} (+ i 1)) }) (let {z}) })";
	mpc_parse("<stdlib>",stdlib,Lispy,&r);
    lval*x =lval_eval(e,lval_read(r.output));
    lval_del(x);
    mpc_ast_delete(r.output);
    
    return e;
} 

void Cabbagelang_finalize(lenv* e){
	lenv_del(e);
    mpc_cleanup(10,Int,Float,Number,Symbol,String,
                Comment,Sexpr,Qexpr,Expr,Lispy);
    free(thread_list);
} 

lval* Cabbagelang_load_string(lenv* e, const char* input, const char* filename){
	mpc_result_t r;
	mpc_parse(filename, input, Lispy, &r);
	lval*x =lval_eval(e,lval_read(r.output));
	return x;
}

lval* Cabbagelang_load_file(lenv* e, const char* filename){
	lval* file=lval_add(lval_sexpr(),lval_str(filename));
    lval* x=builtin_load(e,file);
    return x;
}

#endif