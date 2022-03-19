/*
 * Uruchamianie:        $ ./client1 <adres IP> <numer portu>
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> /* socket() */
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h> /* struct sockaddr_in */
#include <arpa/inet.h>  /* inet_pton() */
#include <unistd.h>     /* close() */
#include <string.h>
#include <errno.h>

#define NI_MAXHOST      1025
#define NI_MAXSERV      32

int main(int argc, char** argv) {

    int sockfd;
    int retval;                 
    struct sockaddr_storage universal_server_addr_storage;
    struct addrinfo* universal_addr_info;
    socklen_t addr_len;
    socklen_t storage_size;
    char buff[256];
    char serv_buff[NI_MAXSERV];
    char host_buff[NI_MAXHOST];

    if (argc != 3) {
        fprintf(stderr, "Invocation: %s <IPv4 or IPv6 ADDRESS> <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Zidentyfikowanie rodzaju adresu podanego przez uzytkownika
    if(getaddrinfo(argv[1], argv[2], NULL, &universal_addr_info) != 0)
    {
        perror("getaddrinfo()");
        exit(EXIT_FAILURE);
    }

    if(universal_addr_info->ai_family == AF_INET)
    {
        printf("Given address is in IPv4 family\n");

        struct sockaddr_in ipv4_addr;
        memset(&ipv4_addr, 0, sizeof(ipv4_addr));
        ipv4_addr.sin_family = AF_INET;
        retval = inet_pton(AF_INET, argv[1], &ipv4_addr.sin_addr);
        if (retval == 0) {
            fprintf(stderr, "inet_pton(): invalid network address!\n");
            exit(EXIT_FAILURE);
        } else if (retval == -1) {
            perror("inet_pton()");
            exit(EXIT_FAILURE);
        }
        ipv4_addr.sin_port = htons(atoi(argv[2]));
        addr_len = sizeof(ipv4_addr);

        memcpy (&universal_server_addr_storage, &ipv4_addr, addr_len);
    }
    else if(universal_addr_info->ai_family == AF_INET6)
    {
        printf("Given address is in IPv6 family\n");

        struct sockaddr_in6 ipv6_addr;
        memset(&ipv6_addr, 0, sizeof(ipv6_addr));
        ipv6_addr.sin6_family = AF_INET6;
        retval = inet_pton(AF_INET6, argv[1], &ipv6_addr.sin6_addr);
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
        addr_len = sizeof(ipv6_addr);

        memcpy (&universal_server_addr_storage, &ipv6_addr, addr_len);
    }
    else
    {
        printf("Not an IP address!\n");
        exit(EXIT_FAILURE);
    }
    
    // Utworzenie gniazda odpowiedniego typu
    sockfd = socket(universal_addr_info->ai_family, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    // Rozpoczencie poloczenia: 
    if (connect(sockfd, (const struct sockaddr *)&universal_server_addr_storage, addr_len) == -1) {
        perror("connect()");
        exit(EXIT_FAILURE);
    }

    //Wypisanie danych
    memset(serv_buff, 0, NI_MAXSERV);
    memset(host_buff, 0, NI_MAXHOST);
    getsockname(sockfd, (struct sockaddr*)&universal_server_addr_storage, &storage_size);
    getnameinfo(
        (struct sockaddr*)&universal_server_addr_storage, storage_size,
        host_buff, NI_MAXHOST,
        serv_buff, NI_MAXSERV,
        NI_NUMERICHOST
    );



    printf(
        "Host IP: %s\nPort: %s\n",
        host_buff,
        serv_buff
    );


    // Odebranie danych:
    memset(buff, 0, 256);
    retval = read(sockfd, buff, sizeof(buff));
    printf("Received server response: %s\n", buff);

    printf("Terminating connecion and closing application.\n");
    // Zamkniecie clienta:
    close(sockfd);
    exit(EXIT_SUCCESS);
}
