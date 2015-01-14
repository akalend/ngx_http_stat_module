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
