#include <stdio.h>
#include "main.h"
#include "udf.h"



/**
*  this is user defined function, 
*  for realise user stats logic
*
*/
int user_function(void* in, void* out) {


	user_local_t*  stats = (user_local_t*) in;

	user_info_t*  user_info = (user_info_t*) out;


	printf("call %s\n", __FUNCTION__);
	return OK;
}


/**
*  this is user defined callback function, 
*  for realise user logic processing url args 
*
*/
void cb_args (F_PARMS){

	user_local_t*  stats = (user_local_t*) st;

    printf("---------- %s  ---------------\n", __FUNCTION__);
    printf("len=%d arg=%s\n", arg_len,arg);

    if ( strncmp(arg,"pos", arg_len) == OK ) {
    	stats->pos = atoi(arg);
    } else if ( strncmp(arg,"user_id", arg_len) == OK ) {
		stats->user_id = atoi(arg);
    } else {
    	printf("unused args=%s\n", arg);
    }

}

/**
*  this is user defined callback function, 
*  for realise user logic processing cookies 
*
*/
void cb_cook (F_PARMS){
    printf("---------- %s  ---------------\n", __FUNCTION__);

    printf("len=%d arg=%s\n", arg_len,arg);
}
