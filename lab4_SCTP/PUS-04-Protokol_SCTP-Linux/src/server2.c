#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/sctp.h>
#include <time.h>

#define BUFF_SIZE 256

int main(int argc, char** argv) {

    int                     listenfd, acceptedfd;
    int                     retval;
    struct sockaddr_in      servaddr;
    char                    buffer[BUFF_SIZE];

    struct sctp_initmsg sctp_options;
    struct sctp_status sctp_sts;

    if (argc != 2) {
        fprintf(stderr, "Invocation: %s <PORT NUMBER>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    listenfd = socket(PF_INET, SOCK_STREAM, IPPROTO_SCTP);
    if (listenfd == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    memset(&sctp_options, 0, sizeof(struct sctp_initmsg));
    sctp_options.sinit_num_ostreams = 2;
    sctp_options.sinit_max_instreams = 2;
    sctp_options.sinit_max_attempts = 5;
    //sctp_options.sinit_max_init_timeo = ?;
    retval = setsockopt(listenfd, IPPROTO_SCTP, SCTP_INITMSG, (void*)&sctp_options, sizeof(struct sctp_initmsg));
    if (retval != 0) {
        fprintf(stderr, "setsockopt() [regular options]\n");
        exit(EXIT_FAILURE);
    }


    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family             =       AF_INET;
    servaddr.sin_port               =       htons(atoi(argv[1]));
    servaddr.sin_addr.s_addr        =       htonl(INADDR_ANY);

    if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    if (listen(listenfd, 5) == -1) {
        perror("listen()");
        exit(EXIT_FAILURE);
    }

    while (1) {

        acceptedfd = accept(listenfd, NULL, 0);
        if (acceptedfd == -1) {
            close(acceptedfd);
            perror("accept()");
            exit(EXIT_FAILURE);
        }

        time_t current_time = time(NULL);
        struct tm* curr_tm = localtime(&current_time);
        strftime(buffer, BUFF_SIZE, "%x", curr_tm);

        retval = sctp_sendmsg(acceptedfd,(void*)buffer,BUFF_SIZE,NULL,0,0,0,0,1000,0);
        if(retval==-1)
        {
            close(listenfd);
            close(acceptedfd);
            perror("sctp_sendmsg()");
            exit(EXIT_FAILURE);
        }
        printf("DATE SENT: %s\n",buffer);

        strftime(buffer, BUFF_SIZE, "%X", curr_tm);

        retval = sctp_sendmsg(acceptedfd,(void*)buffer,BUFF_SIZE,NULL,0,0,0,1,1000,0);
        if(retval==-1)
        {
            close(listenfd);
            close(acceptedfd);
            perror("sctp_sendmsg()");
            exit(EXIT_FAILURE);
        }
        printf("TIME SENT: %s\n",buffer);
    }

}
