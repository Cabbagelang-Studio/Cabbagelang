#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include"./lib/mpc.h"

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
#include<readline/readline.h>
#include<readline/history.h>
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
    escaped=(char*)mpcf_escape(escaped);
    escaped=lval_string_replace(escaped,"\\n","\n");
    escaped=lval_string_replace(escaped,"\\0","\0");
    escaped=lval_string_replace(escaped,"\\r","\r");
    escaped=lval_string_replace(escaped,"\\t","\t");
    escaped=lval_string_replace(escaped,"\\v","\v");
    escaped=lval_string_replace(escaped,"\\a","\a");
    escaped=lval_string_replace(escaped,"\\b","\b");
    escaped=lval_string_replace(escaped,"\\f","\f");
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

lval* http_request(const char* hostname,int port,const char* request){
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        return lval_num(1);
    }
    SOCKET Socket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    struct hostent *host;
    host = gethostbyname(hostname);
    SOCKADDR_IN SockAddr;
    SockAddr.sin_port=htons(port);
    SockAddr.sin_family=AF_INET;
    SockAddr.sin_addr.s_addr = *((unsigned long*)host->h_addr);
    if(connect(Socket,(SOCKADDR*)(&SockAddr),sizeof(SockAddr)) != 0){
        return lval_num(1);
    }
    send(Socket,request, strlen(request),0);
    char* x=malloc(1*sizeof(char));
    x="\0";
    char buffer[1024];
    int nDataLength;
    while ((nDataLength = recv(Socket,buffer,1024,0)) > 0){        
        int i = 0;
        while (buffer[i] >= 32 || buffer[i] == '\n' || buffer[i] == '\r') {
            char* tmp=x;
	        x=malloc((strlen(x)+2)*sizeof(char)+3);
	        strcpy(x,tmp);
	        char character[2]={buffer[i],'\0'};
	        strcat(x,character);
            i += 1;
        }
    }
    closesocket(Socket);
        WSACleanup();
    return lval_str(x);
}
#else

#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> 

lval* http_request(const char* hostname, int port, const char* request){
	int sockfd;
	int len;
	struct sockaddr_in address;
	int result;
	char ch;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd<0){
		perror("Failed to connect");
	}
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = gethostbyname(hostname) ;
	address.sin_port = htons(port);
	len = sizeof(address);
	result = connect(sockfd,  (struct sockaddr *)&address, len);
	if(result == -1){
	    perror("Failed to connect");
	    return 1;
	}
	
	write(sockfd,request,strlen(request));
	
	char* x=malloc(1*sizeof(char));
    x="\0";
	
	while(read(sockfd,&ch,1)){
	    char* tmp=x;
            x=malloc((strlen(x)+2)*sizeof(char)+3);
            strcpy(x,tmp);
            char character[2]={ch,'\0'};
            strcat(x,character);
	}
	close(sockfd);
	
	return 0;
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

lval* builtin_load(lenv* e,lval* a){
    LASSERT_NUM("import", a, 1);
    LASSERT_TYPE("import", a, 0, LVAL_STR);

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
    char* x=malloc(1*sizeof(char));
    x="\0";
    for(int i=0;i<a->count;i++){
    	LASSERT_TYPE("#",a,i,LVAL_STR);
        char* tmp=x;
        x=malloc((strlen(x)+strlen(a->cell[i]))*sizeof(char)+2);
        strcpy(x,tmp);
        strcat(x,a->cell[i]->str);
    }
    strcat(x,"\0");
    lval_del(a);
    lval* result=lval_str(x);
    return result;
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
    return x;
}
lval* builtin_strlen(lenv* e, lval* a){
	LASSERT_TYPE("strlen",a,0,LVAL_STR);
	LASSERT_NUM("strlen",a,1)
    return lval_num(strlen(a->cell[0]->str));
}
lval* builtin_nts(lenv* e, lval* a){
	LASSERT_TYPE("nts",a,0,LVAL_NUM);
	LASSERT_NUM("nts",a,1);
    int length=0;
    double d=a->cell[0]->num,d2=d;
    while(d!=0){
        length++;
        d=d/10-(int)d%10;
    }d2=d-(int)d2;
    length++;
    while(d2!=0){
        length++;
        d2=d2*10;
        d2=d2-(int)d2;
    }length++;
    char* x=malloc(length);
    sprintf(x,"%lf",a->cell[0]->num);
    return lval_str(x);
}
lval* builtin_stn(lenv* e, lval* a){
	LASSERT_TYPE("stn",a,0,LVAL_STR);
	LASSERT_NUM("stn",a,1);
    return lval_num(atof(a->cell[0]->str));
}

lval* builtin_system(lenv* e,lval* a){
	LASSERT_TYPE("system",a,0,LVAL_STR);
	LASSERT_NUM("system",a,1);
    system(a->cell[0]->str);
    lval_del(a);
    return lval_sexpr();
}

lval* builtin_kin(lenv* e,lval* a){
	LASSERT_TYPE("kin",a,0,LVAL_STR);
	LASSERT_NUM("kin",a,1);
    char* input=readline(a->cell[0]->str);
    lval_del(a);
    return lval_str(input);
}


char *readfile(char *path, int *length)
{
	FILE *pfile;
	char *data;
 
	pfile = fopen(path, "rb");
	if (pfile == NULL)
	{
		return NULL;
	}
	fseek(pfile, 0, SEEK_END);
	*length = ftell(pfile);
	data = (char *)malloc((*length + 1) * sizeof(char));
	rewind(pfile);
	*length = fread(data, 1, *length, pfile);
	data[*length] = '\0';
	fclose(pfile);
	return data;
}

lval* builtin_getall(lenv* e,lval* a){
	LASSERT_TYPE("getall",a,0,LVAL_STR);
	LASSERT_NUM("getall",a,1);
	int buffer=0;
	char* input=readfile(a->cell[0]->str,&buffer);
	lval_del(a);
	return lval_str(input);
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
	LASSERT_TYPE("sizeof",a,0,LVAL_STR);
	LASSERT_NUM("sizeof",a,1);
	FILE *pFile;
	pFile = fopen(a->cell[0]->str, "rb");
	return lval_num(get_file_size(pFile));
}

lval* builtin_stdin(lenv* e,lval* a){
	LASSERT_TYPE("stdin",a,0,LVAL_STR);
	LASSERT_NUM("stdin",a,1);
    freopen(a->cell[0]->str,a->cell[1]->str,stdin);
    lval_del(a);
    return lval_sexpr();
}
lval* builtin_stdout(lenv* e,lval* a){
	LASSERT_TYPE("stdout",a,0,LVAL_STR);
	LASSERT_NUM("stdout",a,1);
    freopen(a->cell[0]->str,a->cell[1]->str,stdout);
    lval_del(a);
    return lval_sexpr();
}
lval* builtin_stderr(lenv* e,lval* a){
	LASSERT_TYPE("stdout",a,0,LVAL_STR);
	LASSERT_NUM("stdout",a,1);
	freopen(a->cell[0]->str,a->cell[1]->str,stderr);
    lval_del(a);
    return lval_sexpr();
}
lval* builtin_exit(lenv* e,lval* a){
	LASSERT_TYPE("exit",a,0,LVAL_NUM);
	LASSERT_NUM("exit",a,1);
    exit(a->cell[0]->num);
    return lval_sexpr();
}

lval* builtin_time(lenv* e,lval* a){
	LASSERT_TYPE("time",a,0,LVAL_NUM);
	LASSERT_NUM("time",a,1);
    return lval_num(time((int)a->cell[0]->num));
}
lval* builtin_srand(lenv* e,lval* a){
	LASSERT_TYPE("srand",a,0,LVAL_NUM);
	LASSERT_NUM("srand",a,1);
    srand(a->cell[0]->num);
    return lval_sexpr();
}
lval* builtin_rand(lenv* e,lval* a){
	LASSERT_TYPE("rand",a,0,LVAL_SEXPR);
	LASSERT_NUM("rand",a,1);
	return lval_num(rand());
}
lval* builtin_delay(lenv* e,lval* a){
	LASSERT_TYPE("delay",a,0,LVAL_NUM);
	LASSERT_NUM("delay",a,1);
	sleep((int)a->cell[0]->num);
	return lval_sexpr();
}
lval* builtin_request(lenv* e,lval* a){
	LASSERT_NUM("request",a,3);
	LASSERT_TYPE("request",a,0,LVAL_STR);
	LASSERT_TYPE("request",a,1,LVAL_NUM);
	LASSERT_TYPE("request",a,2,LVAL_STR);
	return http_request(a->cell[0]->str,a->cell[1]->num,a->cell[2]->str);
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

    /* Mathematical Functions */
    lenv_add_builtin(e, "+", builtin_add);
    lenv_add_builtin(e, "-", builtin_sub);
    lenv_add_builtin(e, "*", builtin_mul);
    lenv_add_builtin(e, "/", builtin_div);
    lenv_add_builtin(e, "**", builtin_pow);
    lenv_add_builtin(e, "%", builtin_mod);
    lenv_add_builtin(e, "//", builtin_imul);

    /* Logical Functions */
    lenv_add_builtin(e, "if", builtin_if);
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
    lenv_add_builtin(e, "#", builtin_cs);
    lenv_add_builtin(e, "!", builtin_substr);
    lenv_add_builtin(e, "strlen", builtin_strlen);
    lenv_add_builtin(e, "nts", builtin_nts);
    lenv_add_builtin(e, "stn", builtin_stn);
    
    /* File Functions  */
    lenv_add_builtin(e, "getall", builtin_getall);
    lenv_add_builtin(e, "sizeof", builtin_sizeof);

    /* System Functions */
    lenv_add_builtin(e, "system", builtin_system);
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
}
int main(int argc,char* argv[]){
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
                symbol      : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&%#]+/;             \
                sexpr       : '(' <expr>* ')';                              \
                qexpr       : '{' <expr>* '}';                              \
                expr        : <number> |  <symbol> | <string> |             \
                              <comment> | <sexpr> | <qexpr>;                \
                lispy       : /^/ <expr>* /$/;                              \
              ",
              Number,Int,Float,Symbol,String,Comment,Sexpr,Qexpr,Expr,Lispy);
    /**********/

    lenv* e=lenv_new();
    lenv_add_builtins(e);
    
    mpc_result_t r;
    char* stdlib="(func{nil}{})(func{true}1)(func {false} 0)(func {fun} (\\{f b} {    func (head f)        (\\ (tail f) b)}))(fun {unpack f l}{    eval (join (list f) l)})(fun {pack f & xs} {f xs})(func {curry} unpack)(func {uncurry} pack)(fun {do & l} {    if (== l nil)        {nil}        {last l}})(fun {let b} {    ((\\{_} b) ())})(fun {not x} {- 1 x})(fun {or x y} {+ x y})(fun {and x y} {* x y})(fun {min & xs} {    if (== (tail xs) nil) {fst xs}        {do            (= {rest} (unpack min(tail xs)))            (= {item} (fst xs))            (if (< item rest) {item} {rest})        }})(fun {max & xs} {    if(== (tail xs) nil) {fst xs}        {do            (= {rest} (unpack min (tail xs)))            (= {item} (fst xs))            (if (< item rest) {item} {rest})        }})(fun {flip f a b} {f b a})(fun {ghost & xs} {eval xs})(fun {comp f g x} {f (g x)})(fun {fst l} { eval (head l) })(fun {snd l} { eval (head (tail l)) })(fun {trd l} { eval (head (tail (tail l))) })(fun {len l} {   if (== l nil)    {0}    {+ 1 (len (tail l))}})(fun {nth n l} {   if (== n 0)    {fst l}    {nth (- n 1) (tail l)}})(fun {last l} {nth (- (len l) 1) l})(fun {take n l} {   if (== n 0)    {nil}    {join (head l) (take (- n 1) (tail l))}})(fun {drop n l} {   if (== n 0)    {l}    {drop (- n 1) (tail l)}})(fun {split n l} {list (take n l) (drop n l)})(fun {elem x l} {   if (== l nil)    {false}    {if (== x (fst l))        {true}        {elem x (tail l)}    }})(fun {take-while f l} {   if (not (unpack f (head l)))    {nil}    {join (head l) (take-while f (tail l))}})(fun {drop-while f l} {   if (not (unpack f (head l)))    {l}    {drop-while f (tail l)}})(fun {map f l} {   if (== l nil)    {nil}    {join (list (f (fst l))) (map f (tail l))}})(\\{x}{>x2}){5 2 11 -7 8 1}(fun {filter f l} {    if (== l nil)     {nil}     {join (if (f (fst l))                {head l}                {nil})    (filter f (tail l))}})(fun {foldl f zl}{    if (== l nil)        {z}        {fold l (f z (fst l)) (tail l)}})(fun {sum l} {foldl + 0 l})(fun {product l} {foldl * 1 l})(fun {select &cs} {    if (== cs nil)      {error \"No Selection Found\"}      {if (fst (fst cs))        {snd (fst cs)}        {unpack select (tail cs)}      }})(func {otherwise} true)(fun {month-day-suffix i}{    select        {(== i 0) \"st\"}        {(== i 1) \"nd\"}        {(== i 3) \"rd\"}        {otherwise \"th\"}})(fun {case x & cs} {    if (== cs nil)        {error \"No Case Found\"}        {if (== x (fst (fst cs)))            {snd (fst cs)}            {unpack case (join (list x) (tail cs))}        }})(fun {day-name x} {   case x        {0 \"Monday\"}        {1 \"Tuesday\"}        {2 \"Wednesday\"}        {3 \"Thursday\"}        {4 \"Friday\"}        {5 \"Saturday\"}        {6 \"Sunday\"}})(fun {fib n} {    select        { (== n 0) 0}        { (== n 1) 1}        { otherwise (+ (fib (- n 1)) (fib (- n2)))}})(fun {lookup x l} {   if (== l nil)       {error \"No Element Found\"}        {do            (= {key} (fst (fst l)))            (= {val} (snd (fst l)))            (if (== key x) {val} {lookup x (tail l)})        }})(fun {zip x y} {   if (or (== x nil) (== y nil))    {nil}    {join (list (join (head x) (head y))) (zip (tail x) (tail y))}})(fun {unzip l} {   if (== l nil)    {{nil nil}}    {do        (= {x} (fst l))        (= {xs} (unzip (tail l)))        (list (join (head x) (fst xs)) (join (tail x) (snd xs)))    }})  (fun {while rule expression} {(if (eval rule) {(eval expression) (while rule expression)} {})})";
    mpc_parse("<stdlib>",stdlib,Lispy,&r);
    lval*x =lval_eval(e,lval_read(r.output));
    lval_del(x);
    mpc_ast_delete(r.output);
    
    if(argc==1){

        puts("Cabbagelang Version 3.0.1");
        puts("press Ctrl+C to Exit\n");

        while(1){
            char* input=readline("Cabbagelang>>> ");
            add_history(input);
            //
            mpc_result_t r;
            if(mpc_parse("<stdin>",input,Lispy,&r)){

                lval*x =lval_eval(e,lval_read(r.output));
                lval_println(x);
                //mpc_ast_print(r.output);
                lval_del(x);

                mpc_ast_delete(r.output);
            }else{
                mpc_err_print(r.error);
                mpc_err_delete(r.error);
            }

            free(input);
        }
    }

    if(argc>=2){
        for(int i=1;i<argc;i++){
            lval* args=lval_add(lval_sexpr(),lval_str(argv[i]));
            lval* x=builtin_load(e,args);
            if(x->type==LVAL_ERR){
                lval_println(x);
            }
            //lval_print(x);
            lval_del(x);
        }
    }

    lenv_del(e);
    mpc_cleanup(10,Int,Float,Number,Symbol,String,
                Comment,Sexpr,Qexpr,Expr,Lispy);
    return 0;
}
