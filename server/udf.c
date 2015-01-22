#include <stdio.h>
#include "main.h"
#include "udf.h"


/**
*  this is user defined function, 
*  for realise user stats logic
*
*/
int user_function(void* in, void* out) {


	printf("call %s\n", __FUNCTION__);
	return OK;
}

void cb_args (F_PARMS){

	user_stat_t*  stats = (user_stat_t*) st;

    printf("---------- %s  ---------------\n", __FUNCTION__);
    printf("len=%d arg=%s\n", arg_len,arg);

    if ( strncmp(arg,"pos", arg_len) == OK ) {
    	stats->pos = atoi(arg);
    }

}

void cb_cook (F_PARMS){
    printf("---------- %s  ---------------\n", __FUNCTION__);

    printf("len=%d arg=%s\n", arg_len,arg);
}
