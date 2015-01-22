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





int user_function(void* in, void* out);

#endif /*  _UDF_H */