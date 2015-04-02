#include <stdio.h>
#include "main.h"
#include "udf.h"


f_init_udf 	user_init_function;
f_init_udf 	user_destroy_function;

f_udf user_pre_cycle_function;
f_udf user_post_cycle_function;


static udf_module stat_module = {
	user_init_function,							/*  user init function 		*/
	user_destroy_function,						/*  user destroy function 	*/
	user_pre_cycle_function,					/*  user pre cycle function */
	user_post_cycle_function					/*  user post cycle function */
};


udf_module* udf_get_module() {
	return &stat_module;
}


/**
*  this is user defined function, 
*  init hook: for user stats logic
*
*/
int user_init_function(void* conf) {

	printf("****hook: %s\n", __FUNCTION__);
	return OK;
}


/**
*  this is user defined function, 
*  destroy hook: for user stats logic
*
*/
int user_destroy_function(void* conf) {

	printf("****hook: %s\n", __FUNCTION__);
	return OK;
}



/**
*  user defined function, 
*  init cycle before parse streaming
*
*/
int user_pre_cycle_function(void* in, void* out) {

	printf("****hook: %s\n", __FUNCTION__);
	return OK;
}



/**
*  this is user defined function, 
*  for realise user stats logic
*
*  after finish parse streaming cycle
*/
int user_post_cycle_function(void* in, void* out) {


	user_local_t* local_stats = (user_local_t*) in;
	user_info_t*  user_info = (user_info_t*) out;

	printf("****hook: %s\n", __FUNCTION__);

	/* processing some user data */

	if (local_stats->user_id == 1)
		user_info->count_user_1 ++;
	else if (local_stats->user_id == 2)
		user_info->count_user_2 ++;
	else
		printf("unused user_id=%d\n", local_stats->user_id);
	printf("call %s\n", __FUNCTION__);

	// uint8_t* ip = (uint8_t*) local_stats->remote_host;
	
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
