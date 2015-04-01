#include <string.h>
#include <stdlib.h>

#include "ini.h"
#include "main.h"
#include "parser.h"



extern void parse(const char* fname, conf_t *server_ctx);

static int count = 0;

int parser(void* pctx, const char* section, const char* name,
           const char* value) {
	conf_t* ctx = (conf_t*) pctx;


	int tmp_int = 0;
	int max_num=0;
		
    if (strcmp(section, "daemon") == 0) {
			
		if(strcmp("daemon", name)==0) {
			sscanf(value,"%d",&tmp_int);
			ctx->is_demonize = tmp_int;
		}
				
		if(strcmp("logfile", name)==0) {
			ctx->logfile = strdup(value);
		}
		
		if(strcmp("pidfile", name)==0) {
			ctx->pidfile = strdup(value);
		}
		 
		if(strcmp("listen", name)==0) {
				ctx->listen = strdup(value);
		}

		if(strcmp("username", name)==0) {
			if (strcmp("any", value))
				ctx->username = strdup(value);
		}

		if(strcmp("ip2city_file", name)==0) {
				ctx->ip2city_file = strdup(value);
		}


	} else if (strcmp(section, "format") == 0) {

		printf("[%s] %s=%s;\n", section, name, value);

		if (strcmp("number", name) == 0) {
			int number = atoi(value);

		// printf("[%s] %s=%d;\n", section, name, number);

			if (!number) {
				fprintf(stderr, "ERROR: [%s] %s=%s; number must be more 0\n", section, name, value);
				return ERR;
			}

			if (number > ctx->count) {
				fprintf(stderr, "ERROR: [%s] %s=%s; number must be less the count=%d\n", section, name, value, ctx->count);
				return ERR;
			}

			ctx->format[count].number = number;
		} else  if(strcmp("format", name)==0) {

			ctx->format[count].desc = strdup(value);
			count++;


		} else  if(strcmp("count", name)==0) {
			ctx->count = atoi(value);
			count = 0;
			if (!ctx->count) {
				fprintf(stderr, "ERROR: [%s] %s=%s; count must be more 0\n", section, name, value);
				return ERR;
			}

			ctx->format = (format_t*) calloc( (size_t)ctx->count , sizeof(format_t));

		} else {
			printf("[%s] UNKNOW: %s=%s;\n", section, name, value);
		}
	}	
	
	return OK;
}

int parse_config(const char* fname, conf_t *server_ctx) {
    int rc;
	// printf("call %s  filename=%s\n", __FUNCTION__, fname);
    count = 0;
    bzero(server_ctx, sizeof(conf_t));
    rc = ini_parse(fname, parser, (void*)server_ctx);

    if (rc == ERR) return rc;

    // postconfiguration
    int i;

    server_ctx->cb = (array_t**) calloc( server_ctx->count, sizeof(array_t*) );

    for (i = 0; i < server_ctx->count; i++) {

    	int num = server_ctx->format[i].number;
		printf("parse format num=%d '%s'\n", num, server_ctx->format[i].desc);
    
    	server_ctx->cb[num-1] = parse_format(server_ctx->format[i].desc);
		
        }

    return rc;
}


int free_config(conf_t *ctx) {

	if (ctx->username) { 
		free((void*)ctx->username);
		ctx->username = NULL;}

	if (ctx->logfile) {
		free(ctx->logfile);
		ctx->logfile = NULL;}

	if (ctx->pidfile)  {
		free(ctx->pidfile);
		ctx->pidfile = NULL;}

	if (ctx->listen)  {
		free(ctx->listen);
		ctx->listen = NULL;}

	if (ctx->ip2city_file)  {
		free(ctx->ip2city_file);
		ctx->ip2city_file = NULL;}

	if (ctx->ip2city)  {
		dict_free(ctx->ip2city);
		ctx->ip2city = NULL;}


	if (ctx->format) {
		int i=0;
		for (i=0; i < ctx->count; i++) {
			if(ctx->format[i].desc) {
				free(ctx->format[i].desc);
				ctx->format[i].desc = NULL;
			}
		}

		free(ctx->format);
		ctx->format = NULL;}


		if (ctx->cb) {
		int i=0;
		for (i=0; i < ctx->count; i++) {
			if(ctx->cb[i]) {
				free(ctx->cb[i]);
				ctx->cb[i] = NULL;
			}			
		}
		free(ctx->cb);
		ctx->cb = NULL;}


}


