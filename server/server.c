// gcc -g -O1 -o server   udp_server.c 
#include <stdio.h> //printf
#include <string.h> //memset
#include <stdlib.h>


#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <pwd.h>


#include <fcntl.h>

#include <arpa/inet.h>

#include "main.h" 
#include "ini.h"
#include "parser.h"
#include "udf.h"



#define BUFLEN 512  //Max length of buffer
#define PORT 5555   //The port on which to listen for incoming data

#define CONFIG_FILENAME "conf.ini"

 
const char* stat_format = "servername,host,status,time,arg_server_id,cook_TestRoot,arg_pos,arg_user_id,cook_TestXxx";

void die(char *s)
{
    perror(s);
    exit(1);
}

static void 
daemonize(int do_daemonize, const char *pidfile, const char *run_as)
{
    int fd;

    /* Daemonize part 1 */
    if (do_daemonize) {
        switch (fork()) {
            case ERR:
                perror("fork");
                exit(1);
            case OK:
                break;
            default:
                exit(EXIT_SUCCESS); 
        }

        if (setsid() == -1) {
            perror("setsid");
            exit(1);
        }
    }

    /* Daemonize part 2 */
    if (do_daemonize) {

        /* Change effective uid/gid if requested */
        if (run_as) {
            struct passwd *pw = getpwnam(run_as);
            if (!pw) {
                fprintf(stderr, "No such user: \"%s\"\n", run_as);
                exit(EXIT_FAILURE);
            }
            if (setgid(pw->pw_gid) < 0 || setuid(pw->pw_uid) < 0) {
                fprintf(stderr, "Can't switch to user \"%s\": %s\n", run_as, strerror(errno));
                exit(EXIT_FAILURE);
            }
        }

        /* Save PID file if requested */
        if (pidfile) {      
            FILE *fpid = fopen(pidfile, "w");
            if (!fpid) {
                perror("Can't create pid file");
                exit(1);
            }
            fprintf(fpid, "%ld", (long)getpid());
            fclose(fpid);
        }


        if(chdir("/") != 0) {
            perror("chdir");
            exit(1);
        }

        if ((fd = open("/dev/null", O_RDWR, 0)) != -1) {
            if(dup2(fd, STDIN_FILENO) < 0)  {perror("dup2 stdin"); exit(1);}
            if(dup2(fd, STDOUT_FILENO) < 0) {perror("dup2 stdout"); exit(1);}
            if(dup2(fd, STDERR_FILENO) < 0) {perror("dup2 stderr"); exit(1);}

            if (fd > STDERR_FILENO && close(fd) < 0) {perror("close"); exit(1);}
        } else {
            perror("open(\"/dev/null\")");
            exit(1);
        }
    }
}

int save_pid(const char *pidfile) {
    int fd = open(pidfile, O_WRONLY | O_CREAT );
    if (fd <1) {
        perror("can't open pid file" );
        return ERR;
    }

    fchmod(fd, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd <1) {
        perror("can't assign permission");
        return ERR;
    }

    char buf[16];
    int len = sprintf(buf, "%u\n", getpid());
    write(fd,buf,len);
    close(fd);

    return OK;
}


static pid_t
check_pid(const char *filename) {
    unsigned pid=0;
    FILE * fpid = fopen(filename, "r");
    if (!fpid) {
        fprintf(stderr, "open pid file: %s\n", strerror(errno));
        return OK;
    }

    fscanf(fpid, "%ul", &pid);
    if(ferror(fpid)) {
        fclose(fpid);
        perror("read pidfile");
        exit(1);

    }
    fclose(fpid);
    return (pid_t)pid;
}


static void
usage(const char *binary, int exitcode)
{
    const char *name = strrchr(binary, '/');

    name = name ? name + 1 : binary;
    printf("Usage: %s [path]\n"
            "Options are:\n"
            "  path is configuration file full path\n"
            "\n",
            name);
    
        exit(exitcode);
}



static int
init_addr( addr_t *addr, const char *astring) {

    addr->a_addr = strdup(astring);
    /* Make correct sockaddr */
    if (strncmp(astring,"file:",5) == 0) {
        /* local namespace */
        struct sockaddr_un *name = &addr->un_name;
        addr->pf_family = PF_LOCAL;
        name->sun_family = AF_LOCAL;
        strncpy (name->sun_path, astring+5, sizeof(name->sun_path));
        name->sun_path[sizeof(name->sun_path)-1] = '\0';
        addr->namelen = SUN_LEN(name);
    } else {
        /* inet namespace */
        struct sockaddr_in *name = &addr->in_name;
        char *colon = strchr(addr->a_addr, ':');
        addr->pf_family = PF_INET;
        if (colon || isdigit(*addr->a_addr)) {
            if (colon && colon != addr->a_addr) {
                struct hostent *hostinfo;
                *colon = 0;
                hostinfo = gethostbyname (addr->a_addr);
                if (hostinfo == NULL) {
                    free (addr->a_addr);
                    addr->a_addr=NULL;
                    return ERR;
                }
                *colon = ':';
                name->sin_addr = *(struct in_addr*)hostinfo->h_addr;
            } else {
                name->sin_addr.s_addr = htonl(INADDR_ANY);
            }
            name->sin_family = AF_INET;
            name->sin_port = htons(atoi(colon ? colon+1 : addr->a_addr));
            addr->namelen = sizeof(struct sockaddr_in);
        } else {
            free (addr->a_addr);
            addr->a_addr=NULL;
            return ERR;
        }
    }
    return OK;
}


static conf_t server_ctx;

int main(int argc, char** argv )
{
    struct sockaddr_in si_me, si_other;
    addr_t udp_addr;
    int i;
    int s, slen = sizeof(si_other) , recv_len;
    char buf[BUFLEN];
    char *filename = CONFIG_FILENAME;

    if (argc > 1) filename = argv[1];

    int rc = parse_config( filename,  &server_ctx);

    printf("parse_config=%d\n", rc);
    if (rc == ERR)
    {
        exit(1);
    }


    if (!server_ctx.listen) {
        free_config(&server_ctx);
        perror("undefined listen port");
        exit(1);
    }
    
    daemonize(server_ctx.is_demonize, server_ctx.pidfile, server_ctx.username);

    init_addr(&udp_addr, server_ctx.listen);

    int udp_sock = socket(udp_addr.pf_family, SOCK_DGRAM, IPPROTO_UDP);
    if (udp_sock < 0) {
        free_config(&server_ctx);
        perror("can't create socket");
        exit(1);
    }
 
    //bind server_ctxsocket to port
    if( bind(udp_sock , &udp_addr.name, udp_addr.namelen ) == -1)
    {
        free_config(&server_ctx);
        die("bind");
    }
    
    //keep listening for dat
    i = 0;
    while(1) {

        printf("Waiting for data...\n");

        fflush(stdout);
         
        bzero(buf,BUFLEN);

        //try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(udp_sock, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == ERR) {
            free_config(&server_ctx);
            // free(cb);
            die("recvfrom()");
        }
         
        int format_num = get_format_num(buf); 

        printf("get format_num = %d  save in %s len=%d\n",format_num, filename,recv_len);
    
        stream_parse(buf, server_ctx.cb[format_num-1], user_function );


    }
 
    if ( udp_addr.a_addr ) free(udp_addr.a_addr);
    close(udp_sock);

    free_config(&server_ctx);
    /// clean ctx_server

    return 0;
}
