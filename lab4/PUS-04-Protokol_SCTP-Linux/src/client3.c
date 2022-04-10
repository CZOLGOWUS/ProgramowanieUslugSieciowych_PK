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

#define IN_STREAMS 3
#define OUT_STREAMS 3
#define ATTEMPTS 4

typedef struct sctp_sndrcvinfo SndInfo;
void print_client_info( SndInfo* info)
{
	printf("Stream = %d\n",info->sinfo_stream);
	printf("Id = %d\n",info->sinfo_assoc_id);
	printf("SSN = %d\n",info->sinfo_ssn);
	printf("TSN = %d\n",info->sinfo_tsn);
}


int main(int argc, char** argv) 
{

    int                     sockfd;
    int                     retval, bytes;
    char                    *retptr;


    struct addrinfo hints, *result;

	struct sctp_initmsg init_msg;
	struct sctp_status status;
	struct sctp_sndrcvinfo snd_info;
	struct sockaddr from_addr;


    socklen_t from_len = sizeof(from_addr);
    socklen_t opt_len = sizeof(struct sctp_status);


	char buff_to_send[BUFF_SIZE] = "CLIENT data";
	char buff_to_rcv[BUFF_SIZE] = "";
	unsigned short stream_num = 0;
	


    if (argc != 3)
	{
        fprintf(stderr, "Invocation: %s <IP ADDRESS> <PORT NUMBER>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family         = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype       = SOCK_STREAM;
    hints.ai_flags          = 0;
    hints.ai_protocol       = 0;

    retval = getaddrinfo(argv[1], argv[2], &hints, &result);
    if (retval != 0) 
	{
        fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(retval));
        exit(EXIT_FAILURE);
    }

    if (result == NULL) 
	{
        fprintf(stderr, "Could not connect!\n");
        exit(EXIT_FAILURE);
    }

    sockfd = socket(result->ai_family, SOCK_STREAM, IPPROTO_SCTP);
    if (sockfd == -1) 
	{
        perror("socket()");
        exit(EXIT_FAILURE);
    }



	memset(&init_msg, 0, sizeof(struct sctp_initmsg));

    init_msg.sinit_max_instreams = IN_STREAMS;
    init_msg.sinit_num_ostreams = OUT_STREAMS;
    init_msg.sinit_max_attempts = ATTEMPTS;

	if ( (retval = setsockopt(sockfd, IPPROTO_SCTP, SCTP_INITMSG, &init_msg, sizeof(struct sctp_initmsg))) == -1) 
	{
		perror("setsockopt()");
		exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);


    for (;;) 
	{
		retval = sctp_sendmsg(sockfd,(void*)buff_to_send, BUFF_SIZE,result->ai_addr, result->ai_addrlen,0,0,stream_num,10,0 );
		if(retval == -1)
		{
			perror("sctp_sendmsg()");
            exit(EXIT_FAILURE);
		}

		bytes = sctp_recvmsg(sockfd, buff_to_rcv, BUFF_SIZE, result->ai_addr, &(result->ai_addrlen) , &snd_info, 0);
		if(bytes == -1)
		{
			perror("sctp_recvmsg()");
            exit(EXIT_FAILURE);
		}

		print_client_info(&snd_info);
		printf("received msg = \'%s\'\n",buff_to_rcv);

		memset(buff_to_rcv,0,BUFF_SIZE);

    }

    close(sockfd);
    return 0;
}
