/*
 * Uruchamianie:        $ ./client3 <adres IP> <numer portu>
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> /* socket() */
#include <netinet/in.h> /* struct sockaddr_in */
#include <arpa/inet.h>  /* inet_pton() */
#include <unistd.h>     /* close() */
#include <string.h>
#include <errno.h>
#include <time.h>

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

    int             sockfd;                 /* Desktryptor gniazda. */
    int             retval;                 /* Wartosc zwracana przez funkcje. */
    struct          sockaddr_in remote_addr;/* Gniazdowa struktura adresowa. */
    socklen_t       addr_len;               /* Rozmiar struktury w bajtach. */
    char            msg[256];               // wiadomosc do wyslania
    char            buff[256];              /* Bufor dla funkcji recvfrom(). */


    if (argc != 3) {
        fprintf(
            stderr,
            "Invocation: %s <IPv4 ADDRESS> <PORT>\n", argv[0]
        );
        exit(EXIT_FAILURE);
    }

    /* Utworzenie gniazda dla protokolu UDP: */
    sockfd = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    /* Wyzerowanie struktury adresowej dla adresu zdalnego (serwera): */
    memset(&remote_addr, 0, sizeof(remote_addr));
    /* Domena komunikacyjna (rodzina protokolow): */
    remote_addr.sin_family = AF_INET;

    /* Konwersja adresu IP z postaci kropkowo-dziesietnej: */
    retval = inet_pton(AF_INET, argv[1], &remote_addr.sin_addr);
    if (retval == 0) {
        fprintf(stderr, "inet_pton(): invalid network address!\n");
        exit(EXIT_FAILURE);
    } else if (retval == -1) {
        perror("inet_pton()");
        exit(EXIT_FAILURE);
    }

    remote_addr.sin_port = htons(atoi(argv[2])); /* Numer portu. */
    addr_len = sizeof(remote_addr); /* Rozmiar struktury adresowej w bajtach. */

    // skojarzenie adresu z gniazdem
    retval = connect(sockfd,(const struct sockaddr*) &remote_addr, addr_len);
    if(retval==-1)
    {
        perror("connect()");
        exit(EXIT_FAILURE);
    }

    while(1)
    {
        // Wprowadzenie wiadomosci do wyslania:
        printCurrentTime();
        fprintf(
            stdout,
            "Enter message:\n"
        );

        fgets(msg, 256, stdin);

        if(msg[0]=='\n')
        {
            msg[0]='\0';
        }

        retval = send(sockfd,msg,strlen(msg),0);
        if (retval == -1) {
            perror("sendto()");
            exit(EXIT_FAILURE);
        }

        if(msg[0]=='\0')
        {
            break;
        }

        printCurrentTime();
        fprintf(
            stdout,
            "Waiting for server...\n"
        );

        // Oczekiwanie na odpowiedz.
        retval = recv(sockfd, buff, sizeof(buff), 0);
        if (retval == -1) {
            perror("recv()");
            exit(EXIT_FAILURE);
        }
        buff[retval] = '\0';
        printCurrentTime();
        fprintf(stdout, "Server response: '%s'\n", buff);

    }
    //while(msg[0]!='\0');
    printCurrentTime();
    fprintf(
        stdout,
        "Closing client app...\n"
    );

    close(sockfd);

    exit(EXIT_SUCCESS);
}