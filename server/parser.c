#include <stdio.h> //printf
#include <string.h> //memset
#include <stdlib.h>

#include "main.h"
#include "parser.h"
#include "udf.h"

static f_cb cb_host;
static f_cb cb_servername;
static f_cb cb_time;
static f_cb cb_status;

static user_info_t user_info;

int get_format_num(char* buf) {
	udp_header_t* pheader = (udp_header_t*)buf;
	return pheader->format_num;
}

int stream_parse(char* buf, array_t* cb, f_udf udf ) {

    user_local_t stats;
    bzero(&stats, sizeof(stats));

        udp_header_t* pheader = (udp_header_t*)buf;

        printf("header=%d el_count=%d len=%d time=%u\n",
            pheader->format_num, pheader->el_count, pheader->lenght, pheader->timestamp);

        // stat_buf_t* sb = (stat_buf_t*)(buf + sizeof(udp_header_t));

        udp_stream_t* usb = (udp_stream_t*) buf;

        stat_buf_t* sb = (stat_buf_t*)usb->data;
        u_char* p = (u_char*)sb; 

        int i;
        for(i=0; i < pheader->el_count; i++) {
            printf("sb [%d] pos=%d \n", i, sb->pos);
            u_char* p = (u_char*)sb; 
            
            cb[i].cb( (stats_t*) &stats, sb->data, sb->pos );

            p += sb->pos+1;
            sb = (stat_buf_t*)p;
        }

    udf( &stats, &user_info);
}

user_info_t* get_info() {
    return &user_info;
}


array_t* parse_format(const char* format) {
    
    char* str = strdup(format);
    int count = 0, i = 0;
    char* p = str;

    while(*p != '\0') {
        if (*p == ',') count++;
        p++;
    }

    array_t* arr = calloc(count+1, sizeof(array_t)) ;

    printf("tokens count=%d arr=%lx\n", count,(long) arr);
	 
    char* tok = strtok (str,",");

  	i=0;
  	while (tok != NULL) {
 
	    if (strcmp(tok ,"servername") == 0){
	        printf("[%d] servername is Ok\n", i);
	        
	        //strncpy(arr[i].name, tok, strlen(tok) > 8 ? 8 : strlen(tok));
	        arr[i].cb = cb_servername;
	    } else if (strcmp(tok ,"host") == 0){
	        printf("[%d] host is Ok\n", i);

	        //strncpy(arr[i].name, tok, strlen(tok) > 8 ? 8 : strlen(tok));
	        arr[i].cb = cb_host;

	    } else if (strcmp(tok ,"time") == 0){
	        printf("[%d] time is Ok\n", i);

	        //strncpy(arr[i].name, tok, strlen(tok) > 8 ? 8 : strlen(tok));
	        arr[i].cb = cb_time;

	    } else if (strcmp(tok ,"status") == 0){
	        printf("[%d] status is Ok\n", i);

	        //strncpy(arr[i].name, tok, strlen(tok) > 8 ? 8 : strlen(tok));
	        arr[i].cb = cb_status;

	    } else if (strncmp(tok ,"arg_", 4) == 0){

	        char* p = tok+4;

	        printf("[%d] args: %s\n", i, p);

	        strcpy(arr[i].arg, p);
	        arr[i].cb = cb_args;

	    } else if (strncmp(tok ,"cook_", 5) == 0){

	        char* p = tok+5;

	        printf("[%d] cook: %s\n", i, p);

	        strcpy(arr[i].arg, p);
	        arr[i].cb = cb_args;

	    } else {    

	        printf("unparsed token '%s'\n", tok);
	    }

	    i++;
	    tok = strtok (NULL, ",");
	}

    free(str);

    return arr;
}

//define F_PARMS stats_t* st,  char* arg, int arg_len
static void cb_host (F_PARMS){
    st->host = strndup(arg, arg_len);
}


static void cb_servername (F_PARMS){
    // printf("---------- %s  ---------------\n", __FUNCTION__);
   st->servername = strndup(arg, arg_len);
    // printf("%s\n",  st->servername );
}


static void cb_status (F_PARMS){
    int16_t *status = (int16_t *) arg; 
}


static void cb_time (F_PARMS){
    float *exe_time = (float*) arg; 
}

