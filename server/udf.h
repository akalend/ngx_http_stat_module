#ifndef _UDF_H
#define _UDF_H

#include "main.h"

f_cb cb_args;
f_cb cb_cook;


typedef struct {
	ST_SERVER
	int pos;		// some user data field
	int user_id;	// some user data field
} user_local_t;

typedef struct {
	int count_user_1;		// some user data field
	int count_user_2;		// some user data field
} user_info_t;


typedef struct  {
	f_init_udf* udf_init;
	f_init_udf* udf_destroy;
	f_udf* udf_pre_cycle;
	f_udf* udf_post_cycle;
} udf_module;

udf_module* udf_get_module();


#endif /*  _UDF_H */