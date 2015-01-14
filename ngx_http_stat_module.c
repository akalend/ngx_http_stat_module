
/*
* nginx-stat module
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
* POSSIBILITY OF SUCH DAMAGE.
*
* part this code was derived from nginx-statd module by Valery Kholodkov
*
* Copyright (C) 2014 Alexandre Kalendarev
* akalend@mail.ru
*
*/
#include <ctype.h>
#include <ngx_core.h>
#include <ngx_config.h>
#include <ngx_http.h>
#include <nginx.h>


#define BUFF_SIZE 1024
#define SMALL_BUFF_SIZE 8
#define STAT_UDP_PORT	7777

#define dead     0xDEAD

typedef struct {
    ngx_addr_t				peer_addr;
    ngx_udp_connection_t	*udp_connection;
    ngx_log_t 				*log;
} ngx_udp_endpoint_t;

typedef struct {
    uint8_t          len;
    u_char           data[];
} stat_buf_t;

typedef struct {
        uint8_t     format_num;     // number of format
        uint8_t     el_count;       // count of elements
        uint16_t    lenght;         // lenght of body
        uint32_t    timestamp;      // timestamp
        u_char      data[];
} udp_stream_t;


// bufer_out, request, arg
typedef void (*cb_f) (udp_stream_t*, ngx_http_request_t*, ngx_str_t*);


typedef struct {
	cb_f					cb;
	ngx_str_t 				arg;
} cb_el_t;

typedef struct {
    ngx_flag_t				enable;
    ngx_int_t 				off;
    ngx_udp_endpoint_t      *endpoint;
    ngx_array_t             *cb;
    ngx_uint_t              format_num;
} ngx_http_stat_loc_conf_t;

typedef struct {
	ngx_array_t 			*endpoints;
    ngx_array_t             *cb;
    ngx_uint_t              format_num;
} ngx_http_stat_main_conf_t;


static void ngx_http_stat_cb_host(udp_stream_t*, ngx_http_request_t*, ngx_str_t*);
static void ngx_http_stat_cb_servername(udp_stream_t*, ngx_http_request_t*, ngx_str_t*);
static void ngx_http_stat_cb_time(udp_stream_t*, ngx_http_request_t*, ngx_str_t*);
static void ngx_http_stat_cb_arg(udp_stream_t*, ngx_http_request_t*, ngx_str_t*);
static void ngx_http_stat_cb_referer(udp_stream_t*, ngx_http_request_t*, ngx_str_t*);
static void ngx_http_stat_cb_cookie(udp_stream_t*, ngx_http_request_t*, ngx_str_t*);
static void ngx_http_stat_cb_status(udp_stream_t*, ngx_http_request_t*, ngx_str_t*);


ngx_int_t ngx_udp_connect(ngx_udp_connection_t *uc);

static ngx_int_t ngx_http_stat_init(ngx_conf_t *cf);
static char* ngx_conf_set_stat_enable (ngx_conf_t* cf, ngx_command_t* cmd, void* conf) ;
static char* ngx_http_stat_set_string_format (ngx_conf_t* cf, ngx_command_t* cmd, void* conf) ;

static ngx_udp_endpoint_t *ngx_http_stat_add_endpoint(ngx_conf_t *cf, ngx_addr_t *peer_addr);
static char* ngx_http_stat_set_udp_server(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_stat_udp_send(ngx_udp_endpoint_t *l, u_char *buf, size_t len);

static void * ngx_http_stat_create_main_conf(ngx_conf_t *cf);
static char *ngx_http_stat_megre_loc_conf(ngx_conf_t *cf, void *parent, void *child);
static void *ngx_http_stat_create_loc_conf(ngx_conf_t *cf);

static ngx_command_t  ngx_http_stat_commands[] = {

	{ ngx_string("stats"), //NGX_HTTP_SIF_CONF|NGX_HTTP_LIF_CONF|
      NGX_HTTP_LOC_CONF|NGX_CONF_FLAG|NGX_CONF_TAKE1,
      ngx_conf_set_stat_enable,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("stat_log_format"),
	  NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE2,
	  ngx_http_stat_set_string_format,
	  NGX_HTTP_LOC_CONF_OFFSET,
	  0,
	  NULL },

	{ ngx_string("stat_server"),
	  NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
	  ngx_http_stat_set_udp_server,
	  NGX_HTTP_LOC_CONF_OFFSET,
	  0,
	  NULL },

      ngx_null_command
};

static ngx_http_module_t  ngx_http_stat_module_ctx = {
	NULL,									/* preconfiguration */
	ngx_http_stat_init,						/* postconfiguration */

	ngx_http_stat_create_main_conf,			/* create main configuration */
	NULL,									/* init main configuration */

	NULL,									/* create server configuration */
	NULL,									/* merge server configuration */

	ngx_http_stat_create_loc_conf,	      	/* create location configuration */
	ngx_http_stat_megre_loc_conf			/* merge location configuration */
};



ngx_module_t ngx_http_stat_module = {
    NGX_MODULE_V1,
    &ngx_http_stat_module_ctx,           /* module context */
    ngx_http_stat_commands,              /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
	NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


ngx_int_t ngx_http_stat_handler(ngx_http_request_t *r) 
{
	ngx_http_stat_loc_conf_t  *lcf;	
    udp_stream_t* udp_stream;

	ngx_log_error(NGX_LOG_ERR,  r->connection->log, 0, "**************  http stats handler");

	lcf = ngx_http_get_module_loc_conf(r, ngx_http_stat_module);
    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "enable=%d", lcf->enable);

    if(!lcf->enable) return NGX_OK;


    ngx_int_t res;
    
	cb_el_t* el = lcf->cb->elts;
	ngx_uint_t i=0;

    u_char* pbuf =  ngx_pcalloc(r->pool, 1024);
    ngx_memzero(pbuf, 1024);

    udp_stream = (udp_stream_t*)pbuf; 
    udp_stream->format_num = lcf->format_num;
    udp_stream->el_count = (uint8_t)lcf->cb->nelts;
    udp_stream->timestamp = time(NULL);
    udp_stream->lenght = 0;
 
//    res = ngx_http_stat_udp_send(lcf->endpoint,(u_char*) pbuf, sizeof(udp_stream_t));
	for(i = 0; i < lcf->cb->nelts; i++) {
        el[i].cb(udp_stream, r, &el[i].arg);

       (void) ngx_write_console(ngx_stderr, "===\n", 4); 
	}

    // udp_stream->lenght = (uint16_t)buf->pos;


	res = ngx_http_stat_udp_send(lcf->endpoint,(u_char*) pbuf, 
        udp_stream->lenght + 8);

  ngx_log_error(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "udp_stream sizeof=%d format_num=%d elts=%d time=%d", 
        sizeof(udp_stream_t), lcf->format_num, udp_stream->el_count, udp_stream->timestamp);



	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "send res:%s",  res == NGX_OK ? "OK" : "ERR" );

	return NGX_OK;
}

static void ngx_http_stat_udp_dummy_handler(ngx_event_t *ev) {}


static ngx_int_t
ngx_http_stat_udp_send(ngx_udp_endpoint_t *l, u_char *buf, size_t len)
{
    ssize_t                n;
    ngx_udp_connection_t  *uc;

    uc = l->udp_connection;
    if (uc->connection == NULL) {

        uc->log = *l->log;
        uc->log.handler = NULL;
        uc->log.data = NULL;
        uc->log.action = "logging";

        if(ngx_udp_connect(uc) != NGX_OK) {
            if(uc->connection != NULL) {
                ngx_free_connection(uc->connection);
                uc->connection = NULL;
            }

            return NGX_ERROR;
        }

        uc->connection->data = l;
        uc->connection->read->handler = ngx_http_stat_udp_dummy_handler;
        uc->connection->read->resolver = 0;
    }

    n = ngx_send(uc->connection, buf, len);

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, uc->connection->log, 0, "send %d bytes from", n, len);


    if (n == -1) {
        return NGX_ERROR;
    }

    if ((size_t) n != (size_t) len) {
#if defined nginx_version && nginx_version >= 8032
        ngx_log_error(NGX_LOG_CRIT, &uc->log, 0, "send() incomplete");
#else
        ngx_log_error(NGX_LOG_CRIT, uc->log, 0, "send() incomplete");
#endif
        return NGX_ERROR;
    }

    return NGX_OK;
}


static ngx_udp_endpoint_t *
ngx_http_stat_add_endpoint(ngx_conf_t *cf, ngx_addr_t *peer_addr)
{
    ngx_http_stat_main_conf_t	   *umcf;
    ngx_udp_endpoint_t             *endpoint;

    umcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_stat_module);

    if(umcf->endpoints == NULL) {
        umcf->endpoints = ngx_array_create(cf->pool, 2, sizeof(ngx_udp_endpoint_t));
        if (umcf->endpoints == NULL) {
            return NULL;
        }
    }

    endpoint = ngx_array_push(umcf->endpoints);
    if (endpoint == NULL) {
        return NULL;
    }

    endpoint->peer_addr = *peer_addr;

    return endpoint;
}

static void
ngx_stat_updater_cleanup(void *data)
{
    ngx_udp_endpoint_t  *e = data;

    ngx_log_debug0(NGX_LOG_DEBUG_CORE, ngx_cycle->log, 0,
                   "cleanup stats_updater");

    if(e->udp_connection) {
        if(e->udp_connection->connection) {
            ngx_close_connection(e->udp_connection->connection);
        }

        ngx_free(e->udp_connection);
    }
}


static ngx_int_t ngx_stat_init_endpoint(ngx_conf_t *cf, ngx_udp_endpoint_t *endpoint) {
    ngx_pool_cleanup_t    *cln;
    ngx_udp_connection_t  *uc;

	// ngx_log_debug0(NGX_LOG_DEBUG_HTTP, cf->log, 0,
	// 		   "stat: initting endpoint");

	ngx_log_error(NGX_LOG_ERR, cf->log, 0, "===  initting endpoint");

    cln = ngx_pool_cleanup_add(cf->pool, 0);
    if(cln == NULL) {
        return NGX_ERROR;
    }

    cln->handler = ngx_stat_updater_cleanup;
    cln->data = endpoint;

    uc = ngx_calloc(sizeof(ngx_udp_connection_t), cf->log);
    if (uc == NULL) {
        return NGX_ERROR;
    }

    endpoint->udp_connection = uc;

    uc->sockaddr = endpoint->peer_addr.sockaddr;
    uc->socklen = endpoint->peer_addr.socklen;
    uc->server = endpoint->peer_addr.name;

    endpoint->log = &cf->cycle->new_log;

    return NGX_OK;
}


static void 
ngx_http_stat_cb_host(udp_stream_t* buf, ngx_http_request_t *r, ngx_str_t *arg) {

	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "call %s", __FUNCTION__ );

    ngx_table_elt_t *el = r->headers_in.host;

    stat_buf_t* p = (stat_buf_t*)(buf->data + buf->lenght);  
    buf->lenght =  buf->lenght + (uint16_t)el->value.len + 1; 

    p->len = (uint8_t)el->value.len;
    ngx_memcpy(p->data, el->value.data, el->value.len);

    (void) ngx_write_console(ngx_stderr, "--------------        host  -----------------\n", 42);
    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "p=%s len=%d", p->data, p->len );
}

static void 
ngx_http_stat_cb_referer(udp_stream_t* buf, ngx_http_request_t *r, ngx_str_t *arg) {

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "call %s", __FUNCTION__ );

    ngx_table_elt_t *el = r->headers_in.referer;

    stat_buf_t* p = (stat_buf_t*)(buf->data + buf->lenght);  

    buf->lenght += (uint16_t)el->value.len + 1; 

    p->len = (uint8_t)el->value.len;
    ngx_memcpy(p->data, el->value.data, el->value.len);

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "p=%s len=%d", p->data, p->len );

}

static void 
ngx_http_stat_cb_servername(udp_stream_t* buf, ngx_http_request_t *r, ngx_str_t *arg) {
    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "call %s", __FUNCTION__ );

    ngx_str_t server = r->headers_in.server;
    stat_buf_t* p = (stat_buf_t*)(buf->data + buf->lenght);

    buf->lenght =  buf->lenght + (uint16_t)server.len + 1;

    p->len = (uint8_t)server.len;
    ngx_memcpy(p->data, server.data, server.len);

    (void) ngx_write_console(ngx_stderr, "--------------  servername  -----------------\n", 42);
    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "p=%s len=%d", p->data, p->len );
}


static void 
ngx_http_stat_cb_status(udp_stream_t* buf, ngx_http_request_t *r, ngx_str_t *arg) {
    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "call %s", __FUNCTION__ );

    stat_buf_t* p = (stat_buf_t*)(buf->data + buf->lenght);
    buf->lenght += 3;

    ngx_memcpy(p->data, &r->headers_out.status, 2);
    p->len = 2;

    (void) ngx_write_console(ngx_stderr, "--------------  status  -----------------\n", 42);
    ngx_log_debug3(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "status=%d len=%d status=%d", 
        r->headers_out.status, p->len, r->err_status );


}

static void 
ngx_http_stat_cb_time(udp_stream_t* buf, ngx_http_request_t *r, ngx_str_t *arg) {
	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "call %s", __FUNCTION__ );
   
    ngx_time_t *tp = ngx_timeofday();
    ngx_msec_int_t ms = (ngx_msec_int_t) ((tp->sec - r->start_sec) * 1000 + (tp->msec - r->start_msec));
    ms = (ms >= 0) ? ms : 0;
    
    float request_time = (float)ms/1000;
    uint8_t sz = (uint8_t)sizeof(float);

    stat_buf_t* p = (stat_buf_t*)(buf->data + buf->lenght);

    buf->lenght += sz + 1;
 
    // time_t                            start_sec;
    // ngx_msec_t                        start_msec;

    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "time=%4.4f size=%d status=%d", request_time, sz, r->headers_out.status);

    p->len = (uint8_t)sz;
    ngx_memcpy(p->data, &request_time, sz);
}

static void 
ngx_http_stat_cb_arg(udp_stream_t* buf, ngx_http_request_t *r, ngx_str_t *arg) {
	ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "call %s", __FUNCTION__ );

    u_char   *value, *value_end, *start, *last, *end;
   
    (void) ngx_write_console(ngx_stderr, "--------------  arg  -----------------\n", 40);

    stat_buf_t* p = (stat_buf_t*)(buf->data + buf->lenght);
    
    start = r->args.data;
    if (!start) {
        // тут надо записать 0 в байт
        buf->lenght ++;
        p->len = 0;
        return;
    }
    end = r->args.data + r->args.len;


    ngx_log_debug4(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "uri.data: \"%s\" len=%d \narg.data: \"%s\" len=%d\n", 
                   start, r->args.len, arg->data, arg->len);


    while (start < end){
    
        last = start;
        while (start < end && *start != '=') {start++;}

        value = start+1;

        ngx_log_debug4(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "args len=%d last='%s' len=%d value='%s'", 
            start - last, last, arg->len, value);

        value_end = start;
        while ( value_end < end && *value_end != '&') {value_end++;} 

        if (arg->len != (ngx_uint_t)(start - last)) {
            start = value_end + 1;
            ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "arg->len != len next: '%s'", 
            start);
            if (start >= end ) break;
            continue;
        }
        
        ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "cmp key: \"%s\" arg=\"%V\" ", last, arg);

        if (ngx_strncasecmp(last, arg->data, arg->len) != 0) {

            ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "strcmp_case != 0");
        
            start = value_end + 1;
            ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "next: '%s'", 
            start);
            if (start >= end ) break;
            continue;

        }
        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "strcmp_case Ok '%s'" , last);

        last = start;

        start++;
        ngx_log_debug3(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "**** value: \"%s\" len=%d arg=\"%V\" ", start, value_end - value, arg);
        
        buf->lenght += (value_end - value) + 1;

        p->len = (uint8_t)(value_end - value);
        ngx_memcpy(p->data, value, p->len);

            break;
        }
}

static void 
ngx_http_stat_cb_cookie(udp_stream_t* buf, ngx_http_request_t *r, ngx_str_t *arg) {
    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "call %s", __FUNCTION__ );

    ngx_uint_t i;
    u_char   *value, *value_end, *start, *last, *end;
    ngx_table_elt_t  **h;
    stat_buf_t* p = (stat_buf_t*)(buf->data + buf->lenght);

    (void) ngx_write_console(ngx_stderr, "--------------  cook  -----------------\n", 41);

    h = r->headers_in.cookies.elts;

    for (i = 0; i < r->headers_in.cookies.nelts; i++) {
        
        ngx_log_debug3(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
         "parse Cookies: key=\"%V\" value=\"%V\" arg=\"%V\" ",
          &h[i]->key, &h[i]->value, arg );

        start = h[i]->value.data;
        end = h[i]->value.data + h[i]->value.len;
        // len = 0;

        ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "Header.data: \"%s\" len=%d", start, h[i]->value.len);

        while (start < end){
        
            last = start;
            while (start < end && *start != '=') {start++;}

            value = start + 1;

            ngx_log_debug4(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "cookies len=%d '%s' len=%d value='%s'", 
                start - last, last, arg->len, value);

            value_end = start;
            while ( value_end < end && *value_end != ';') {value_end++;} 


            if (arg->len != (ngx_uint_t)(start - last)) {
                start = value_end + 2;
                ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "arg->len != len next: '%s'", 
                start);
                if (start >= end ) break;
                continue;
            }

            
            ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                           "cmp key: \"%s\" arg=\"%V\" ", last, arg);

            if (ngx_strncasecmp(last, arg->data, arg->len) != 0) {

                ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "strcmp_case != 0");
            
                start = value_end + 2;
                ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "next: '%s'", 
                start);
                if (start >= end ) break;
                continue;

            }
          ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "strcmp_case Ok '%s'" , last);
    
            last = start;
    
            start++;
            ngx_log_debug3(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                           "**** value: \"%s\" len=%d arg=\"%V\" ", start, value_end - value, arg);
            
            buf->lenght += (value_end - value) + 1;

            p->len = (uint8_t)(value_end - value);
            ngx_memcpy(p->data, value, p->len);

            break;
        }
    }
}

static char*
ngx_http_stat_set_string_format (ngx_conf_t* cf, ngx_command_t* cmd, void* conf) {

	ngx_log_error(NGX_LOG_ERR, cf->log, 0, "*******  parse format");
    ngx_str_t 					*value;
	ngx_http_stat_main_conf_t 	*umcf;
    ngx_http_stat_loc_conf_t    *lcf;
	ngx_int_t 					len;

    value = cf->args->elts;

    ngx_log_error(NGX_LOG_ERR, cf->log, 0, "parse len=%d '%s'", value[2].len, value[2].data);

    if (value[2].len == 0) return NULL;

    u_char* p = value[2].data;
    ngx_uint_t count=0;

	while( p < (u_char*)(value[2].data + value[2].len) ){
	    if (*p == ',') count++;
	    p++;
	}

	ngx_log_error(NGX_LOG_INFO, cf->log, 0, "count=%d", count);

    umcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_stat_module);
    if(umcf == NULL) {
        ngx_log_error(NGX_LOG_ERR, cf->log, 0, "main conf == NULL");
        return NULL;
    }

    lcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_stat_module);
    if(lcf == NULL) {
        ngx_log_error(NGX_LOG_ERR, cf->log, 0, "loc conf == NULL");
        return NULL;
    }

    lcf->format_num = ngx_atoi(value[1].data, value[1].len);
    if (!lcf->format_num) {
        ngx_log_error(NGX_LOG_ERR, cf->log, 0, "stats log_fomat number equal 0");
        return NULL;
    }

    ngx_log_error(NGX_LOG_ERR, cf->log, 0, "format_num == %d", lcf->format_num );

    if(lcf->cb == NGX_CONF_UNSET_PTR ) {
       lcf->cb = ngx_array_create(cf->pool, count+1, sizeof(cb_el_t));
        if (lcf->cb == NULL) {
            ngx_log_error(NGX_LOG_ERR, cf->log, 0, "can not  ngx_array_create");
            return NULL;
        }
    }

    p = value[2].data;
	while( p < (u_char*)(value[2].data + value[2].len) ){

		cb_el_t* el = ngx_array_push(lcf->cb);

        // ngx_log_error(NGX_LOG_ERR, cf->log, 0, "parse tok '%s'", p);

		if (ngx_strncmp(p, "host", 4) == 0) { // $host
			ngx_log_error(NGX_LOG_ERR, cf->log, 0, "set '%s'", p);
			p += sizeof("host");
			el->cb = ngx_http_stat_cb_host;

		} else if (ngx_strncmp(p, "servername", 10) == 0) { // $host
		    ngx_log_error(NGX_LOG_ERR, cf->log, 0, "set '%s'", p);
			p += sizeof("servername");
			el->cb = ngx_http_stat_cb_servername;

		} else if (ngx_strncmp(p, "time", 4) == 0) { 		// $time
		    ngx_log_error(NGX_LOG_ERR, cf->log, 0, "set '%s'", p);
			p += sizeof("time");
			el->cb = ngx_http_stat_cb_time;

        } else if (ngx_strncmp(p, "referer", 6) == 0) {        // $referer
            ngx_log_error(NGX_LOG_ERR, cf->log, 0, "set '%s'", p);
            p += sizeof("referer");
            el->cb = ngx_http_stat_cb_referer;

        } else if (ngx_strncmp(p, "status", 6) == 0) {        // $status
            ngx_log_error(NGX_LOG_ERR, cf->log, 0, "set '%s'", p);
            p += sizeof("status");
            el->cb = ngx_http_stat_cb_status;

		} else if (ngx_strncmp(p, "arg", 3) == 0) { 		// $arg_
			p += sizeof("arg");
		    ngx_log_error(NGX_LOG_ERR, cf->log, 0, "set arg='%s'", p);

		    el->arg.data = p;		    
		    len=0;
			while( p < (u_char*)(value[2].data + value[2].len) ){
				if (*p++ == ',') break;
				len++;
			}
			el->arg.len = len;
			el->cb = ngx_http_stat_cb_arg;

        } else if (ngx_strncmp(p, "cook", 4) == 0) {         // $cook_
            p += sizeof("cook");
            ngx_log_error(NGX_LOG_ERR, cf->log, 0, "set arg='%s'", p);

            el->arg.data = p;           
            len=0;
            while( p < (u_char*)(value[2].data + value[2].len) ){
                if (*p++ == ',') break;
                len++;
            }
            el->arg.len = len;
            el->cb = ngx_http_stat_cb_cookie;

		} else {
			ngx_log_error(NGX_LOG_ERR, cf->log, 0, "no parse '%s'", p);
			while( p < (u_char*)(value[2].data + value[2].len) ){
				if (*p++ == ',') break;
			}
		}
	}

    if(umcf->cb == NGX_CONF_UNSET_PTR ) {
        umcf->cb = lcf->cb;
        umcf->format_num = lcf->format_num;
    }

	return NGX_CONF_OK;
}


static char* ngx_http_stat_set_udp_server(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    // ngx_http_stat_udp_conf_t	*ulcf = conf;
    ngx_http_stat_loc_conf_t 	*ulcf = conf;
    ngx_str_t 					*value;
    ngx_url_t					u;

	// ngx_log_debug0(NGX_LOG_DEBUG_HTTP, cf->log, 0,
	// 		   "stat: initting endpoint");

	ngx_log_error(NGX_LOG_ERR, cf->log, 0, "*******  init endpoint");

    value = cf->args->elts;

    if (ngx_strcmp(value[1].data, "off") == 0) {
        ulcf->off = 1;
        return NGX_CONF_OK;
    }
    ulcf->off = 0;

    ngx_memzero(&u, sizeof(ngx_url_t));

    u.url = value[1];
    u.default_port = STAT_UDP_PORT;
    u.no_resolve = 0;

    if(ngx_parse_url(cf->pool, &u) != NGX_OK) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "%V: %s", &u.host, u.err);
        return NGX_CONF_ERROR;
    }

	ulcf->endpoint = ngx_http_stat_add_endpoint(cf, &u.addrs[0]);
    if(ulcf->endpoint == NULL) {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}

static ngx_int_t ngx_http_stat_init(ngx_conf_t *cf) 
{
	ngx_http_handler_pt 			*h;
	ngx_http_stat_main_conf_t  		*umcf;
	ngx_http_core_main_conf_t  		*cmcf;
    ngx_udp_endpoint_t           	*e;
    ngx_uint_t                    	i;
    ngx_uint_t                    	rc;

    // ngx_http_stat_loc_conf_t    *lcf;
    // lcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_stat_module);


	umcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_stat_module);
	// srvcf = ngx_http_conf_get_module_srv_conf(cf, ngx_http_core_module);

    if(umcf->endpoints != NULL) {
        e = umcf->endpoints->elts;
        for(i = 0; i < umcf->endpoints->nelts; i++) {
            rc = ngx_stat_init_endpoint(cf, e + i);

            if(rc != NGX_OK) {
                return NGX_ERROR;
            }
        }
	}

    // lcf->off = 1;

	ngx_log_error(NGX_LOG_ERR, cf->log, 0,
    	"*********** postconfiguration:  stat init cfg" );

	cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

	h = ngx_array_push(&cmcf->phases[NGX_HTTP_LOG_PHASE].handlers);
	if (h == NULL) {
		return NGX_ERROR;
	}

	*h = ngx_http_stat_handler;

	return NGX_OK;
}


static char* ngx_conf_set_stat_enable (ngx_conf_t* cf, ngx_command_t* cmd, void* conf) {

	ngx_str_t *value;

    // ngx_http_core_loc_conf_t  *clcf;

	ngx_http_stat_loc_conf_t *lcf = conf;

    // clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);

	value = cf->args->elts;

	if (cf->args->nelts == 2 && ngx_strncmp(value[1].data, "off",3) == 0) {
		lcf->enable = 0;
	} else if (cf->args->nelts == 2 && ngx_strncmp(value[1].data, "on",2) == 0) {
		lcf->enable = 1;
	} else {
		ngx_log_error(NGX_LOG_ERR, cf->log, 0,
                  "conf parameter error 'stats %s'",
                   value[1].data);
	
	    return NGX_CONF_ERROR;

	}


	ngx_log_error(NGX_LOG_ERR, cf->log, 0,
                  "stage %s [%s] enable=%d value='%s' args=%d",__FUNCTION__, value[0].data,lcf->enable, value[1].data, cf->args->nelts );
    
    return NGX_CONF_OK;
}


static void * ngx_http_stat_create_main_conf(ngx_conf_t *cf)
{
    ngx_http_stat_main_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_stat_main_conf_t));
    if (conf == NULL) {
        return NGX_CONF_ERROR;
    }
	ngx_log_error(NGX_LOG_EMERG, cf->log, 0,
        "*** stage %s createmain conf", __FUNCTION__);

    conf->cb         = NGX_CONF_UNSET_PTR;
    conf->format_num = NGX_CONF_UNSET_UINT;

    return conf;
}

static void *ngx_http_stat_create_loc_conf(ngx_conf_t *cf) 
{
	ngx_http_stat_loc_conf_t  *conf;

	conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_stat_loc_conf_t));
	if (conf == NULL) {		
		return NULL;
	}

	conf->enable 	 = NGX_CONF_UNSET_UINT;
	conf->endpoint 	 = NGX_CONF_UNSET_PTR;
	conf->off 		 = NGX_CONF_UNSET_UINT;
    conf->cb         = NGX_CONF_UNSET_PTR;
    conf->format_num = NGX_CONF_UNSET_UINT;

	ngx_log_error(NGX_LOG_EMERG, cf->log, 0,
                      "*** stage %s conf->enable=%d ", __FUNCTION__, conf->enable);

	return conf;
}

static char *
ngx_http_stat_megre_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
	ngx_http_stat_loc_conf_t *prev = parent;
	ngx_http_stat_loc_conf_t *conf = child;

    ngx_log_error(NGX_LOG_EMERG, cf->log, 0,"ngx_http_stat_megre_loc_conf()");


	ngx_conf_merge_value(conf->enable, prev->enable, 0);
	ngx_conf_merge_ptr_value(conf->endpoint, prev->endpoint, NULL);
	ngx_conf_merge_off_value(conf->off, prev->off, 0);
    ngx_conf_merge_ptr_value(conf->cb, prev->cb, NULL);
    ngx_conf_merge_uint_value(conf->format_num, prev->format_num, 0);


	ngx_log_error(NGX_LOG_EMERG, cf->log, 0,
                      "[args=%d] stage %s conf->enable=%d",
                      cf->args->nelts , __FUNCTION__, conf->enable);

	return NGX_CONF_OK;
}
