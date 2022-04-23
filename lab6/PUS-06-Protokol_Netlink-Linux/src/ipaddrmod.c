#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <asm/types.h>
#include <arpa/inet.h>
#include <net/if.h>
#include "libnetlink.h"
/*
 * Program wymaga uprawnien roota.
 */

#define BUFF_SIZE 16384

int main(int argc, char** argv) {

    int                     sockfd; /* Deskryptor gniazda. */
    int                     retval; /* Wartosc zwracana przez funkcje. */
    uint32_t                addr;   /* Do przechowywania adresu IPv4. */
    unsigned char           action; /* Typ operacji: dodanie/usuniecie adresu IP. */
    struct sockaddr_nl      sa;     /* Struktura adresowa */
    struct nlmsghdr         *nh;    /* Wskaznik na naglowek Netlink. */
    struct ifaddrmsg        *ia;    /* Wskaznik na naglowek rodziny. */

    /* Bufor dla wysylanego komunikatu: */
    void                    *request;
    int                     request_size;

    /* Bufor dla odbieranego komunikatu: */
    void                    *response;
    int                     response_size;

    if (argc != 5) {
        fprintf(
            stderr, "Invocation: %s <INTERFACE NAME> <ACTION>"
            " <IPv4 ADDRESS> <PREFIX-LENGTH>\n",
            argv[0]
        );

        exit(EXIT_FAILURE);
    }

    /* Okreslenie operacji - dodanie lub usuniecie adresu IPv4: */
    if (strcmp(argv[2], "add") == 0) {
        action = RTM_NEWADDR;
    } else if (strcmp(argv[2], "del") == 0) {
        action = RTM_DELADDR;
    } else {

        fprintf(
            stderr, "Invalid action! Only \"add\" and \"del\" are allowed.\n"
        );

        exit(EXIT_FAILURE);
    }

    /* Utworzenie gniazda Netlink: */
    sockfd = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (sockfd == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    /*
     * Alokacja pamieci dla wysylanego komunikatu.
     *
     * NLMSG_SPACE(sizeof(struct ifaddrmsg)) = rozmiar naglowka 'nlmsghdr'
     * + opcjonalny padding + rozmiar naglowka 'ifaddrmsg' + opcjonalny padding;
     *
     * RTA_SAPCE(sizeof(uint32_t)) = rozmiar naglowka 'rtattr' + opcjonalny padding
     * + sizeof(uint32_t) + opcjonalny padding;
     *
     * Podobnie RTA_SAPCE(strlen(argv[1])).
     */
    request_size = NLMSG_SPACE(sizeof(struct ifaddrmsg))
                   + 3*RTA_SPACE(sizeof(uint32_t))
                   + RTA_SPACE(strlen(argv[1]));

    request = malloc(request_size);
    if (request == NULL) {
        fprintf(stderr, "malloc() failed!\n");
        exit(EXIT_FAILURE);
    }

    /* Wyzerowanie struktury adresowej.
     * Jadro jest odpowiedzialne za ustalenie identyfikatora gniazda.
     * Poniewaz jest to pierwsze (i jedyne) gniazdo procesu,
     * identyfikatorem gniazda bedzie PID procesu. */
    memset(&sa, 0, sizeof(struct sockaddr_nl));
    sa.nl_family            =       AF_NETLINK;

    if (bind(sockfd, (struct sockaddr*) &sa, sizeof(struct sockaddr_nl)) == -1) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    nh                      = (struct nlmsghdr*)request;
    /*
     * Rozmiar naglowka 'nlmsghdr' + opcjonalny padding
     * + rozmiar naglowka 'ifaddrmsg':
     */
    nh->nlmsg_len           =       NLMSG_LENGTH(sizeof(struct ifaddrmsg));
    /*
     * Pole nh->nlmsg_len bedzie aktualizowane
     * przez wywolania funkcji 'addattr_l'.
     */

    nh->nlmsg_type          =       action; /* RTM_NEWADDR lub RTM_DELADDR */
    nh->nlmsg_flags         =       NLM_F_REQUEST | NLM_F_ACK; // oczekujemy odpowiedzi ACK
    nh->nlmsg_seq           =       1;
    nh->nlmsg_pid           =       getpid();

    ia                      = (struct ifaddrmsg*)NLMSG_DATA(nh);
    ia->ifa_family          =       AF_INET;
    ia->ifa_prefixlen       =       atoi(argv[4]);
    ia->ifa_flags           =       IFA_F_PERMANENT;
    ia->ifa_scope           =       0;
    ia->ifa_index           =       if_nametoindex(argv[1]);

    /* Zero nie jest prawidlowym indeksem interfejsu: */
    if (ia->ifa_index == 0) {
        fprintf(stderr, "Cannot find device \"%s\"!\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    retval = inet_pton(AF_INET, argv[3], &addr);
    if (retval == 0) {
        fprintf(stderr, "inet_pton(): invalid IPv4 address!\n");
        exit(EXIT_FAILURE);
    } else if (retval == -1) {
        perror("inet_pton()");
        exit(EXIT_FAILURE);
    }

    addattr_l(nh, request_size, IFA_ADDRESS, (void *)&addr, 4);
    addattr_l(nh, request_size, IFA_LOCAL, (void *)&addr, 4);

    addr = addr & htonl(0xffffffff<<(32 - ia->ifa_prefixlen));
    addattr_l(nh, request_size, IFA_BROADCAST, (void *) &addr, 4);

    /* IFA_LABEL nie moze byc obecne przy usuwaniu.
     * Przy usuwaniu mozna podac eth0 zamiast
     * aliasu eth0:1 (ten sam index interfejsu). */
    if (action == RTM_NEWADDR) {
        addattr_l(
            nh, request_size, IFA_LABEL,
            (void *) argv[1], strlen(argv[1])
        );
    }

    /* Struktura adresowa jest wyzerowana.
     * Wiadomosc bedzie przeznaczona do jadra (sa.nl_pid rowne 0). */
    memset(&sa, 0, sizeof(struct sockaddr_nl));
    sa.nl_family            =       AF_NETLINK;

    fprintf(stdout, "Sending message to kernel...\n");

    retval = sendto(
                 sockfd, request, nh->nlmsg_len, 0,
                 (struct sockaddr*)&sa , sizeof(struct sockaddr_nl)
             );

    if (retval == -1) {
        perror("sendto()");
        exit(EXIT_FAILURE);
    }

    free(request);

    // response_size = NLMSG_SPACE(sizeof(struct ifaddrmsg))
    //                + 3*RTA_SPACE(sizeof(uint32_t))
    //                + RTA_SPACE(strlen(argv[1]));
    response_size = BUFF_SIZE;

    response = malloc(response_size);
    if (response == NULL) {
        fprintf(stderr, "malloc() failed!\n");
        exit(EXIT_FAILURE);
    }

    // oczekiwanie na odpowiedz:

    retval = recvfrom(
                 sockfd, response, response_size, 0,
                 NULL , 0
             );

    if (retval == -1) {
        perror("recvfrom()");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "Received message:\n");

    struct nlmsghdr *nh_response; // Wskaznik na naglowek Netlink odpowiedzi.
    nh_response = (struct nlmsghdr*)response;
    printf("  type: %x",nh_response->nlmsg_type);
    if(nh_response->nlmsg_type & NLMSG_ERROR)
    {
        struct nlmsgerr *err_info = 
            (struct nlmsgerr*) (response + NLMSG_HDRLEN);
        
        printf(" --> NLMSG_ERROR --> err number: %d --> %s ",err_info->error,err_info->error ? "ERROR" : "ACKNOWLEDGEMENT");
        switch (err_info->error)
        {
        case -1:
            printf(" Not a superuser.");
            break;
        case -17:
            printf(" Identical record already added.");
            break;
        case -99:
            printf(" Cannot remove nonexistent record.");
            break;
        default:
            break;
        }
        printf("\n");
    }
    printf("  flags: 0x%x",nh_response->nlmsg_flags);
    if(nh_response->nlmsg_flags & NLM_F_REPLACE)
    {
        printf(" --> Operation of replacement successful!\n");
    }
    else if(nh_response->nlmsg_flags == 0)
    {
        printf(" --> No operation.\n");
    }
    else
    {
        printf(" --> Other operation.\n");
    }
    
    free(response);
    close(sockfd);
    exit(EXIT_SUCCESS);
}
