#define _CABBAGELANG_BUILTIN_LEAVES 1
#include"./lib/Cabbagelang.h"

int main(int argc,char* argv[],char* env[]){
	lenv* e=Cabbagelang_initialize(argc,argv,env);
    if(argc==1){

        printf("Cabbagelang Version %s\n",CABBAGELANG_VERSION);
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
    	lval* file=lval_add(lval_sexpr(),lval_str(argv[1]));
        lval* x=builtin_load(e,file);
        if(x->type==LVAL_ERR){
            lval_println(x);
        }
        //lval_print(x);
        lval_del(x);
    }

    Cabbagelang_finalize(e);
    return 0;
}
