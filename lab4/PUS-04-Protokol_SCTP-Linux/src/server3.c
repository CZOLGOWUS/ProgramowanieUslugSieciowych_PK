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
void print_server_info_recv(int stream_id, SndInfo* info)
{
	printf("Stream = %d\n",stream_id);
	printf("Id = %d\n",info->sinfo_assoc_id);
	printf("SSN = %d\n",info->sinfo_ssn);
	printf("TSN = %d\n",info->sinfo_tsn);
}

int main(int argc, char** argv)
{
	int option_selected = -1;
	int current_stream_id = 0;
	int to_send_stream_id = 0;

	int                     listenfd;
    int                     retval, bytes;
    struct sockaddr_in      server_addr;
    struct sockaddr dest_addr;
	socklen_t dest_addr_len = sizeof(dest_addr);
	
	struct sctp_initmsg init_msg;
	struct sctp_sndrcvinfo send_receive_info;

    struct sctp_event_subscribe events_subscr;

	char buff_to_send[BUFF_SIZE] = "Sample Data from server";
	char buff_to_rcv[BUFF_SIZE] = "";
	
    if (argc != 3) 
	{
        fprintf(stderr, "Invocation: %s <PORT NUMBER> <OPTION>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

	option_selected = atoi(argv[2]);
	if(option_selected != 0 && option_selected != 1)
	{
		printf("option can be \'0\' or \'1\'");
		exit(EXIT_FAILURE);
	}

    listenfd = socket(PF_INET, SOCK_SEQPACKET, IPPROTO_SCTP);
    if (listenfd == -1) 
	{
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    memset(&events_subscr, 0, sizeof(events_subscr));
    events_subscr.sctp_data_io_event = 1;

	memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family             =       AF_INET;
    server_addr.sin_port               =       htons(atoi(argv[1]));
    server_addr.sin_addr.s_addr        =       htonl(INADDR_ANY);

    memset(&init_msg, 0, sizeof(struct sctp_initmsg));
    init_msg.sinit_max_instreams = IN_STREAMS;
    init_msg.sinit_num_ostreams = OUT_STREAMS;
    init_msg.sinit_max_attempts = ATTEMPTS;

    
    if ( setsockopt(listenfd, IPPROTO_SCTP, SCTP_INITMSG, &init_msg, sizeof(struct sctp_initmsg)) ||
         setsockopt(listenfd, IPPROTO_SCTP, SCTP_EVENTS, &events_subscr, sizeof(struct sctp_event_subscribe))
    )
    {
        perror("setsockopt()");
        exit(EXIT_FAILURE);
    }

    if (bind(listenfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) 
	{
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    if (listen(listenfd, 10) == -1) 
	{
        perror("listen()");
        exit(EXIT_FAILURE);
    }


	for(;;)
    {
        if (option_selected) 
		{
            current_stream_id = to_send_stream_id;

            if( (to_send_stream_id + 1) == OUT_STREAMS)
                to_send_stream_id = -1;

            ++to_send_stream_id;
        }

		bytes = sctp_recvmsg(listenfd, buff_to_rcv, BUFF_SIZE, &dest_addr, &dest_addr_len, &send_receive_info, NULL);
        if (bytes == -1) 
		{
            perror("recv()");
            exit(EXIT_FAILURE);
        }

        print_server_info_recv(current_stream_id,&send_receive_info);
		printf("received msg = \'%s\'\n",buff_to_rcv);

		retval = sctp_sendmsg(listenfd, (void*)buff_to_send, BUFF_SIZE, &dest_addr, dest_addr_len,0,0,current_stream_id,10,0);
		if(retval == -1)
		{
			perror("sctp_sendmsg()");
			exit(EXIT_FAILURE);
		}

		memset(buff_to_rcv,0,BUFF_SIZE);

	}

	close(listenfd);

}
