/*
 * Uruchamianie:        $ ./server1 <numer portu>
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> /* socket() */
#include <netinet/in.h> /* struct sockaddr_in */
#include <arpa/inet.h>  /* inet_ntop() */
#include <unistd.h>     /* close() */
#include <string.h>
#include <time.h>
#include <errno.h>

void printCurrentTime()
{
    time_t now;
    struct tm *tm;

    now = time(0);
    if ((tm = localtime(&now)) == NULL)
    {
        printf("Error extracting time stuff\n");
        return;
    }

    printf("[ %02d:%02d:%02d ] ",
           tm->tm_hour, tm->tm_min, tm->tm_sec);
}

int main(int argc, char **argv)
{

    /* Deskryptory dla gniazda nasluchujacego i polaczonego: */
    int server_sd, client_sd;

    int retval;

    /* Gniazdowe struktury adresowe (dla klienta i serwera): */
    struct sockaddr_in client_addr;
    struct sockaddr_in server_addr;

    /* Rozmiar struktur w bajtach: */
    socklen_t client_addr_len, server_addr_len;

    /* Bufor danych: */
    char buff[256];

    /* Bufor dla adresu IP klienta w postaci kropkowo-dziesietnej: */
    char addr_buff[256];

    time_t rawtime;
    struct tm *timeinfo;

    if (argc != 2)
    {
        fprintf(stderr, "Invocation: %s <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Utworzenie gniazda dla protokolu TCP:
    server_sd = socket(PF_INET, SOCK_STREAM, 0);
    if (server_sd == -1)
    {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    // Tworzenie struktury adresowej serwera:
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));
    server_addr_len = sizeof(struct sockaddr_in);

    // Przygotowanie gniazda servera
    if (bind(server_sd, (struct sockaddr *)&server_addr, server_addr_len) == -1)
    {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    if (listen(server_sd, 2) == -1)
    {
        perror("listen()");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        // oczekiwanie na polocznie przychodzace
        printCurrentTime();
        printf("Server is listening for incoming connection...\n");

        client_addr_len = sizeof(client_addr);
        client_sd = accept(server_sd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_sd == -1)
        {
            perror("accept()");
            exit(EXIT_FAILURE);
        }

        printCurrentTime();
        printf(
            "TCP connection accepted from %s port %d\n",
            inet_ntop(AF_INET, &client_addr.sin_addr, addr_buff, sizeof(addr_buff)),
            ntohs(server_addr.sin_port));

        // Wyslanie odpowiedzi:
        printCurrentTime();
        printf("Sending message...\n");

        retval = sendto(
            client_sd,
            "Laboratorium PUS",
            17,
            0,
            (struct sockaddr *)&client_addr,
            client_addr_len);

        printCurrentTime();
        printf("Closing connected socket (sending FIN to client)...\n");
        close(client_sd);
    }

    printCurrentTime();
    printf("Closing listening socket and terminating server...\n");
    close(server_sd);

    exit(EXIT_SUCCESS);
}
