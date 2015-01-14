
#ifndef _MAIN_H
#define _MAIN_H
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>



#ifndef FALSE
#define FALSE   0
#endif

#ifndef TRUE
#define TRUE    (!FALSE)
#endif

enum {
	ERR = -1,
	OK = 0,
	END = 2
};


typedef struct {
    char*           key;
    
    union { char*           cvalue;
            int64_t         dvalue;
        } value;
} tuple_t;

typedef struct {
        uint32_t    timestamp;      // timestamp
        uint32_t    status;         // HTTP response status

        char*       host;           // host
        char*       servername;     // servername

        float       exe_time;       // execute scrit time
        uint32_t    tuple_count;    // count of tuple

        tuple_t*    tuples;         // array of tuples
} stats_t;

#define F_PARMS stats_t* st,  char* arg, int arg_len
typedef void (f_cb) (F_PARMS);

typedef int (f_udf) (void*, void*);



typedef struct {
    char            arg[16];
    f_cb*            cb;
} array_t;


typedef struct {
	int				number;
	char*			desc;	
	} format_t;


typedef struct {
	char* 			logfile;
	char* 			listen;
	char* 			pidfile;
	char * 			username;
	short 			is_demonize;	
	int 			count;
	format_t*		format;
	array_t**		cb;
} conf_t;


typedef struct {
	union {
		struct sockaddr		name;
		struct sockaddr_in	in_name;
		struct sockaddr_un	un_name;
	};
	int			pf_family;
	socklen_t	namelen;
	char		*a_addr;
} addr_t;


typedef struct {
        uint8_t     format_num;     // number of format
        uint8_t     el_count;       // count of elements
        uint16_t    lenght;         // lenght of body
        uint32_t    timestamp;      // timestamp
        u_char      data[];
} udp_stream_t;



typedef struct {
        uint8_t     format_num;     // number of format
        uint8_t     el_count;       // count of elementst
        uint16_t    lenght;         // lenght of body
        uint32_t    timestamp;      // timestamp
} udp_header_t;



typedef struct {
    uint8_t       pos;
    u_char        data[];
} stat_buf_t;


#define TRACE(fmt, ...)								\
	if (is_trace) {									\
		printf("%s[%d] ", __FUNCTION__, __LINE__);  \
		printf(fmt, ##__VA_ARGS__);					\
	};

#endif /* _MAIN_H */