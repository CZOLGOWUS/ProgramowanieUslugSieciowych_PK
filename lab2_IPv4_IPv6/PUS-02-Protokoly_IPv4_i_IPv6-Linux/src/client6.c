/*
 * Uruchamianie:        $ ./client1 <adres IP> <numer portu>
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> /* socket() */
#include <netinet/in.h> /* struct sockaddr_in */
#include <arpa/inet.h>  /* inet_pton() */
#include <unistd.h>     /* close() */
#include <string.h>
#include <netdb.h>
#include <net/if.h>
#include <errno.h>

int main(int argc, char **argv)
{

    int sockfd;
    int retval;
    struct sockaddr_in6 remote_addr;
    socklen_t addr_len;
    char buff[256];

    struct hostent *host;

    if (argc != 4)
    {
        fprintf(stderr, "Invocation: %s <IPv6 ADDRESS> <PORT> <INTERFACE_NAME>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Utworzenie gniazda dla protokolu TCP:
    sockfd = socket(PF_INET6, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    // Budowa struktury adresowej dla adresu serwera
    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin6_family = AF_INET6;
    retval = inet_pton(AF_INET6, argv[1], &remote_addr.sin6_addr);
    if (retval == 0)
    {
        fprintf(stderr, "inet_pton(): invalid network address!\n");
        exit(EXIT_FAILURE);
    }
    else if (retval == -1)
    {
        perror("inet_pton()");
        exit(EXIT_FAILURE);
    }

    remote_addr.sin6_port = htons(atoi(argv[2]));
    // dodanie interfejsu do struktury
    remote_addr.sin6_scope_id = htons(if_nametoindex(argv[3]));
    addr_len = sizeof(remote_addr);

    // Rozpoczencie poloczenia:
    if (connect(sockfd, (const struct sockaddr *)&remote_addr, addr_len) == -1)
    {
        perror("connect()");
        exit(EXIT_FAILURE);
    }

    // Odebranie danych:
    memset(buff, 0, 256);
    retval = read(sockfd, buff, sizeof(buff));
    printf("Received server response: %s\n", buff);

    // Zamkniecie clienta:
    printf("Closing socket (sending FIN to server)...\n");
    close(sockfd);
    printf("Terminating application. After receiving FIN from server, "
           "TCP connection will go into TIME_WAIT state.\n");
    exit(EXIT_SUCCESS);
}
