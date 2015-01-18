# ngx_http_stat_module
The ngx_http_stat_module is addon for nginx HTTP server. The addon send some user data to stat daemon by UDP. The stat daemon collection, accumulation and processing statistics. This project using for adv partner-billing systems. 

# Introduction

The ad networks require accounting of data. The information from cookies and part url as GET parameter identify the users, clicks, partners and banners. This information send by binary UDP protocol to statistic daemon. The statistic daemon collection, accumulation, processing and agregation information save to database.


This project have files:

	drwxr-xr-x   conf			- the directory with example config files
	-rw-r--r--   config			- file for configuration of addon
	drwxr-xr-x   html			- the some test files
	drwxrwxrwx   server 			- the source of daemon framework is the server directory
	-rw-r--r--   LICENSE			- text of license
	-rw-r--r--   ngx_http_stat_module.c 	- source of module
	-rw-r--r--   README.md 			- this file


# Configuration

The context (http://nginx.org/en/docs/http/ngx_http_core_module.html#server) of ngx_http_stat_module module is server.


stats_server [host:port] ; 	address of daemon (only server context)

stats on|off ;		 	on/off statistic by any location (http://nginx.org/en/docs/http/ngx_http_core_module.html#location) 

stat_log_format [ number  "list of parms"];  number and structure data to be sent.


nginx.conf example: 


    server {
        listen       80;
        server_name  localhost;

        stat_server localhost:5555;

        location ~ /yyy {

            stat_log_format 1 "servername,host,status,time,arg_server_id,cook_TestRoot,arg_pos,arg_user_id";
            stats on;

            root   /home/akalend/projects/ngx_http_stat_module/html;
            index  index.htm;
        }
    }


The format structure is:

	arg_xxx 	- get value of argument xxx from uri: http://myhost.com?xxx=123 
	cook_abc 	- value of coockies abc
	servername 	- value of server variable SERVERNAME
	host,		- value of server variable   Host
	time,		- time  of use in msec
	referer,	- value of server HTTP Header : Referer
	user_agent	- value of server HTTP Header : UserAgent
	status		- code of status (200, 404, etc)




# Protocol

The atoms of representation in the protocol include:

	int8 - a single 8-bit byte (i.e. an octet)
	
	int32 - a 32-bit integer in little-endian format (Intel x86)
	
	int64 - a 64-bit integer in little-endian format (Intel x86)

	HTTP server (ngx_http_stat_module) sent stats packet to stats server. The response is absent.


	<packet> ::= <header><body>

	<header> ::= <format_num><el_count><body_lenght><timestamp>

	<format_num> ::= <int8>
	<format_num> represents a format number, first parameter from directive "stat_log_format" of nginx.conf

	<el_count>  ::= <int8>
	<el_count> represents a count of elements (tuples) from second parameter from directive "stat_log_format" of nginx.conf

	<el_count>  ::= <int16>
	<el_count> represents a lenght of body in bytes

	<timestamp>  ::= <int32>
	<timestamp> is a current timestamp in unix time

	<body> ::= <tuple>+
	A body that represents a set of tuples. Count of tuples represents in the field <el_count>. The order of tuples must be coordinate (cоответствовать) the order of field from the directive "stat_log_format" of nginx.conf. 

	<tuple> ::= <value_len><value>
	A tuple that represents a some data.

	<value_len> ::= <int8>
	<value_len> represents a lenght of tuple value

	<value> :: <int8>+
	represents a stream of byte of tuple value



# Server

Server is daemon, wich receive data from nginx. The server is framework. User must define UDF (user defined function) and some callbacks function,



# Server configuration file

The server configuration file have structure of ini-files: https://github.com/akalend/ngx_http_stat_module/blob/master/server/conf.ini. The two main section: "daemon" and "format". The "daemon" section describe daemon parameters as listening address, is demon enabled, username, pid filename and etc.


	; This is an INI file
	[daemon]  ; daemon section

	logfile = error.log								; to syslog

	listen = 127.0.0.1:5555 						; lisen ip:port format [host:port]
	daemon = 1
	username = nobody

	pidfile = /tmp/stat_server.pid


The "format" section describe formats of received messages. This section have parameter "count". It is count of formats. So any format have parameter "number" and "format":

	[format] 		; format section
	
	count = 2 		; count of formats

	number = 1
	format = servername,host,status,time,arg_server_id,cook_TestRoot,arg_pos,arg_user_id

	number = 2
	format = servername,host,status,cook_TestRoot,arg_pos

The "format" and "number" parameters must consist parameter "stat_log_format" from nginx.conf.

	stat_log_format 1 "servername,host,status,time,arg_server_id,cook_TestRoot,arg_pos,arg_user_id";


