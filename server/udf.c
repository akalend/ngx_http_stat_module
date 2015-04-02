#include <stdio.h>
#include "main.h"
#include "udf.h"



/**
*  this is user defined function, 
*  for realise user stats logic
*
*/
int user_function(void* in, void* out) {


	user_local_t* local_stats = (user_local_t*) in;
	user_info_t*  user_info = (user_info_t*) out;

	/* processing some user data */

	if (local_stats->user_id == 1)
		user_info->count_user_1 ++;
	else if (local_stats->user_id == 2)
		user_info->count_user_2 ++;
	else
		printf("unused user_id=%d\n", local_stats->user_id);
	printf("call %s\n", __FUNCTION__);

	uint8_t* ip = (uint8_t*) local_stats->remote_host;
	
	printf("remote IP(%u)\n", local_stats->remote_host); // : %d.%d.%d.%d,ip[0], ip[1], ip[2], ip[3]


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
