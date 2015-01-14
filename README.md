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
