### Introduction

Модуль предназначен для сбора статистики и передачу её по UDP на демон обработчик. Исходники демона обработчика находядся в директории server.

Какую информацию собираем: 
- заданные переменные GET
- заданные куки
- время выполнения скрипта
- серверные переменные: servername, host 
- HTTP заголовки: Referer, User-agent


### Configuration


stats_server [host:port] ; 		адрес сервера сборщика информации.

stats on|off ;		 включение/исключение статистики по данному локейшену

stat_log_format  "servername,host,time,arg_xxx,cook_abc";  формат данных, поступаемых на сервер-сборщик.

Переменные конфигурации формата:

	arg_xxx - значение аргумента xxx из строки запроса: http://myhost.com?xxx=123 
	cook_abc - значение куки abc
	servername, - значение y переменной
	host,		- Значение в запросе переменной  Host
	time,		- время выполнения скрипта в милисек
	referer,	- значение заголовка HTTP : Referer
	user_agent	- значение заголовка HTTP : UserAgent


stats_ua_botlist_file ua_botlist.txt; имя файла конфигурации - списка ua ботнетов





Протокол общения с UDP сервером (stats_collector):

[Header, Body]

Header = [timestamp int_32, num_format int_8, elm_count int_8, body_lenght int_16  ]
Body = [element, element, element, ...]
element = [len int_8, data ]		- элемент данных, из строки stat_log_format 
data = [byte, byte, byte, byte, ...] 
byte uchar;

timestamp - время выполнения запроса
