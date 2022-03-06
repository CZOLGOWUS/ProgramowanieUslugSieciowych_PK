/*
 * Data:                2009-02-10
 * Autor:               Jakub Gasior <quebes@mars.iti.pk.edu.pl>
 * Kompilacja:          $ gcc server2.c -o server2
 * Uruchamianie:        $ ./server2 <numer portu>
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> /* socket() */
#include <netinet/in.h> /* struct sockaddr_in */
#include <arpa/inet.h>  /* inet_ntop() */
#include <unistd.h>     /* close() */
#include <string.h>
#include <errno.h>
#include <time.h>

#include "libpalindrome.h"


void printCurrentTime()
{
    time_t now;
    struct tm *tm;

    now = time(0);
    if ((tm = localtime (&now)) == NULL) {
        printf ("Error extracting time stuff\n");
        return;
    }

    printf ("[ %02d:%02d:%02d ] ", tm->tm_hour, tm->tm_min, tm->tm_sec );
}


int main(int argc, char** argv) {

    int             sockfd; /* Deskryptor gniazda. */
    int             retval; /* Wartosc zwracana przez funkcje. */

    /* Gniazdowe struktury adresowe (dla klienta i serwera): */
    struct          sockaddr_in client_addr, server_addr;

    /* Rozmiar struktur w bajtach: */
    socklen_t       client_addr_len, server_addr_len;

    /* Bufor wykorzystywany przez recvfrom() i sendto(): */
    //char            buff[256];

    /* Bufor dla adresu IP klienta w postaci kropkowo-dziesietnej: */
    char            addr_buff[256];


    if (argc != 2) {
        fprintf(stderr, "Invocation: %s <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }


    /* Bufor wykorzystywany przez recvfrom() i sendto(): */
    char            buff[256];

    /* Utworzenie gniazda dla protokolu UDP: */
    sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }



    /* Wyzerowanie struktury adresowej serwera: */
    memset(&server_addr, 0, sizeof(server_addr));
    
    /* Domena komunikacyjna (rodzina protokolow): */
    server_addr.sin_family          =       AF_INET;
    /* Adres nieokreslony (ang. wildcard address): */
    server_addr.sin_addr.s_addr     =       htonl(INADDR_ANY);
    /* Numer portu: */
    server_addr.sin_port            =       htons(atoi(argv[1]));
    /* Rozmiar struktury adresowej serwera w bajtach: */
    server_addr_len                 =       sizeof(server_addr);

    /* Powiazanie "nazwy" (adresu IP i numeru portu) z gniazdem: */
    if (bind(sockfd, (struct sockaddr*) &server_addr, server_addr_len) == -1) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

while(1)
{
    printCurrentTime();
    fprintf(stdout, "Server is listening for incoming connection...\n");

    client_addr_len = sizeof(client_addr);

    /* Oczekiwanie na dane od klienta: */
    retval = recvfrom(
                 sockfd,
                 buff, 
                 sizeof(buff),
                 0,
                 (struct sockaddr*)&client_addr, &client_addr_len
             );

    if(retval == 0)
    {
        close(sockfd);
        printf("received empty msg, shuting down server\n");
        exit(EXIT_SUCCESS);
    }

    buff[retval] = '\0';

    if (retval == -1) {
        perror("recvfrom()");
        exit(EXIT_FAILURE);

    }

    printCurrentTime();
    fprintf(stdout, "UDP datagram received from %s:%d.\n",
            inet_ntop(AF_INET, &client_addr.sin_addr, addr_buff, sizeof(addr_buff)),
            ntohs(client_addr.sin_port)
           );



    int palindromResult = is_palindrome(buff,retval);
    char respondBuff[3];

    sprintf(respondBuff,"%d",palindromResult);
    printf("result : %s\n",respondBuff);
    
    /* Wyslanie odpowiedzi (echo): */
    retval = sendto(
                 sockfd,
                 respondBuff, 
                 sizeof(respondBuff),
                 0,
                 (struct sockaddr*)&client_addr, 
                 client_addr_len
             );
             
    if (retval == -1) {
        perror("sendto()");
        exit(EXIT_FAILURE);
    }

}
    

    close(sockfd);


    exit(EXIT_SUCCESS);
}
