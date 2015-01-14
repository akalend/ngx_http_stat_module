#ifndef _PARSER_H
#define _PARSER_H

#include "main.h"

int stream_parse(char* buf, array_t* cb, f_udf udf);
array_t* parse_format(const char* format);
int get_format_num(char* buf);

#endif /*  _PARSER_H */