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

#define BUFF_SIZE 256

int main(int argc, char** argv) {

    int                     sockfd;
    int                     retval;
    struct addrinfo hints, *result;
    char                    buff[BUFF_SIZE];

    struct sctp_initmsg sctp_options;
    struct sctp_status sctp_sts;
    socklen_t sctp_sts_len =  sizeof(struct sctp_status);

    if (argc != 3) {
        fprintf(stderr, "Invocation: %s <IP ADDRESS> <PORT NUMBER>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family         = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype       = SOCK_STREAM;
    hints.ai_flags          = 0;
    hints.ai_protocol       = 0;

    retval = getaddrinfo(argv[1], argv[2], &hints, &result);
    if (retval != 0) {
        fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(retval));
        exit(EXIT_FAILURE);
    }

    if (result == NULL) {
        fprintf(stderr, "Could not connect!\n");
        exit(EXIT_FAILURE);
    }

    sockfd = socket(result->ai_family, result->ai_socktype, IPPROTO_SCTP);
    if (sockfd == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    memset(&sctp_options, 0, sizeof(struct sctp_initmsg));
    sctp_options.sinit_num_ostreams = 3;
    sctp_options.sinit_max_instreams = 4;
    sctp_options.sinit_max_attempts = 5;
    //sctp_options.sinit_max_init_timeo = ?;
    retval = setsockopt(sockfd, IPPROTO_SCTP, SCTP_INITMSG, (void*)&sctp_options, sizeof(struct sctp_initmsg));
    if (retval != 0) {
        fprintf(stderr, "setsockopt()\n");
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd, result->ai_addr, result->ai_addrlen) == -1) {
        perror("connect()");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);

    retval = getsockopt(sockfd, IPPROTO_SCTP, SCTP_STATUS, &sctp_sts, &sctp_sts_len);
    if (retval == -1) {
        fprintf(stderr, "getsockopt()\n");
        exit(EXIT_FAILURE);
    }
    printf("ID: %d\n",sctp_sts.sstat_assoc_id);
    printf("Association state: %d\n",sctp_sts.sstat_state);
    printf("Output sterams: %d\n",sctp_sts.sstat_outstrms);
    printf("Input sterams: %d\n",sctp_sts.sstat_instrms);

    retval = sctp_recvmsg(sockfd,buff, BUFF_SIZE,NULL, NULL, NULL, NULL);
    if (retval == -1) {
        perror("sctp_recvmsg()");
        exit(EXIT_FAILURE);
    }

    printf("Received 1: %s\n",buff);

    retval = sctp_recvmsg(sockfd,buff, BUFF_SIZE,NULL, NULL, NULL, NULL);
    if (retval == -1) {
        perror("sctp_recvmsg()");
        exit(EXIT_FAILURE);
    }

    printf("Received 2: %s\n",buff);
    
    close(sockfd);
    exit(EXIT_SUCCESS);
}
