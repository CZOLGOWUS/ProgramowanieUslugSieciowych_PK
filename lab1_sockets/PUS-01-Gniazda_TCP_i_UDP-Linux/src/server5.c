/*
 * Uruchamianie:        $ ./server4 <numer portu>
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> /* socket(), listen() */
#include <sys/select.h> /* select() fd_set */
#include <netinet/in.h> /* struct sockaddr_in */
#include <arpa/inet.h>  /* inet_ntop() */
#include <unistd.h>     /* close() */
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include <sys/types.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/time.h>

#include <regex.h>

#define MAX_CLIENTS 20 // maskymalna obslugiwana liczba klientow
#define BUFF_SIZE 256  // maskymalna dlugosc buffora danych

void *handleConnection(void *arg);
void setHttpHeader(char httpHeader[]);
void readFile(char *path, char result[]);
void writeToFile(char path[],char text[]);

struct request_package
{
    int socket_desc;
    char request[256];
};

int main(int argc, char **argv)
{

    int main_sd; // Deskryptor gniazda glownego.

    /* Gniazdowe struktury adresowe (dla klienta i serwera): */
    struct sockaddr_in client_addr, server_addr;

    /* Rozmiar struktur w bajtach: */
    socklen_t client_addr_len, server_addr_len;

    /* Bufor wykorzystywany przez recvfrom() i sendto(): */
    char buff[BUFF_SIZE];

    if (argc != 2)
    {
        fprintf(stderr, "Invocation: %s <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Utworzenie gniazda dla protokolu TCP: */
    main_sd = socket(PF_INET, SOCK_STREAM, 0);
    if (main_sd == -1)
    {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    /* Wyzerowanie struktury adresowej serwera: */
    memset(&server_addr, 0, sizeof(server_addr));

    /* Domena komunikacyjna (rodzina protokolow): */
    server_addr.sin_family = AF_INET;
    /* Adres nieokreslony (ang. wildcard address): */
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    /* Numer portu: */
    server_addr.sin_port = htons(atoi(argv[1]));
    /* Rozmiar struktury adresowej serwera w bajtach: */
    server_addr_len = sizeof(server_addr);

    // Powiazanie "nazwy" (adresu IP i numeru portu) z gniazdem
    if (bind(main_sd, (struct sockaddr *)&server_addr, server_addr_len) == -1)
    {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

    // Rozpocznij nasluch na glownym gniezdzie (deskryptorze)
    if (listen(main_sd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for connections ...");

    int thread_counter = 0;
    int new_sd;
    while (1)
    {
        // zaakceptuj je
        if ((new_sd = accept(main_sd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len)) < 0)
        {
            perror("accept() error");
            exit(EXIT_FAILURE);
        }

        printf("Nowe poloczenie %d\n  ip: %s\n  port : %d\n\n", new_sd, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        recv(new_sd, buff, sizeof(buff), 0);
        printf("%s", buff);

        // stwórz nowy wątek i podaj deskryptor gniazda
        pthread_t th;
        if (pthread_create(&th, NULL, &handleConnection, (void *)&new_sd) != 0)
        {
            perror("pthread_create()\n");
            exit(EXIT_FAILURE);
        }

        thread_counter++;
        pthread_detach(th);
    }

    close(main_sd);
    exit(EXIT_SUCCESS);
}

void setHttpHeader(char httpHeader[])
{
    // File object to return
    FILE *htmlData = fopen("index.html", "r");

    char line[20];
    char responseData[200];
    while (fgets(line, 20, htmlData) != 0)
    {
        strcat(responseData, line);
    }
    // char httpHeader[8000] = "HTTP/1.1 200 OK\r\n\n";
    strcat(httpHeader, responseData);
}

void readFile(char *path, char result[])
{
    FILE *fp;
    char con[20];

    fp = fopen(path, "r");
    if (!fp)
    {
        perror("file error : ");
        exit(EXIT_FAILURE);
    }

    while (fgets(con, 20, fp) != NULL)
    {
        strcat(result, con);
    }

    fclose(fp);
}

void *handleConnection(void *arg)
{
    int socketDescritpor = *((int *)arg);

    char buff[BUFF_SIZE];

    printf("connectting... new socket : %d \n", socketDescritpor);

    if (recv(socketDescritpor, buff, sizeof(buff), 0) == -1)
    {
        close(socketDescritpor);
        exit(EXIT_FAILURE);
    }

    char* methodFind = strstr(buff,"GET");
    printf("\n--------METHOD :%s",buff);

    if (methodFind != NULL)
    {
        char httpResponse[8192] = "";

        char httpResponse_temp[8192] = "";
        char httpHeader_template[] =
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: %d\r\n"
            "Content-Type: text/html; charset=iso-8859-1\r\n"
            "\r\n";
        char httpBody_temp[8192 - 512] = "";

        readFile("index.html", httpBody_temp);

        strcat(httpResponse_temp, httpHeader_template);
        strcat(httpResponse_temp, httpBody_temp);
        sprintf(httpResponse, httpResponse_temp, (int)strlen(httpBody_temp));


        //debug
        /*
        writeToFile("text.html",httpResponse);
        */      

        send(socketDescritpor, httpResponse, strlen(httpResponse), 0);
    }

    return 0;
}

void writeToFile(char path[],char text[])
{
    FILE *fileAddress;
    fileAddress = fopen(path, "w");

    if (fileAddress != NULL)
    {
        // Let us use our fputs
        fputs(text, fileAddress);
        fclose(fileAddress);
    }
    else
    {
        printf("\n Unable to Create or Open the Sample.txt File");
    }
}