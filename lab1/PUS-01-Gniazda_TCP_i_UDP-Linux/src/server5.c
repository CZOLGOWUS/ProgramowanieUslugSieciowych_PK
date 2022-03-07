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

int main(int argc, char **argv)
{

    int main_sd;                // Deskryptor gniazda glownego.
    int client_sd[MAX_CLIENTS]; // Deksryptory klientow.
    int retval;
    int max_sd; // Maks. deskryptor (potrzebne funkcji select)

    /* Gniazdowe struktury adresowe (dla klienta i serwera): */
    struct sockaddr_in client_addr, server_addr;

    /* Rozmiar struktur w bajtach: */
    socklen_t client_addr_len, server_addr_len;

    /* Bufor wykorzystywany przez recvfrom() i sendto(): */
    char buff[BUFF_SIZE];

    /* Bufor dla adresu IP klienta w postaci kropkowo-dziesietnej: */
    char addr_buff[256];

    // zbior deskryptorow uzywany przez select
    fd_set descriptors_set;

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
    // inicjalizacja listy klientow (0 = brak clienta)
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        client_sd[i] = 0;
    }

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


        recv(new_sd,buff,sizeof(buff),0);
        printf("%s",buff);

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

    char line[100];
    char responseData[8000];
    while (fgets(line, 100, htmlData) != 0) {
        strcat(responseData, line);
    }
    // char httpHeader[8000] = "HTTP/1.1 200 OK\r\n\n";
    strcat(httpHeader, responseData);
}

void *handleConnection(void *arg)
{
    int socketDescritpor = *((int *)arg);

    char buff[BUFF_SIZE];
    int retval;

    struct sockaddr_in client_addr;
    socklen_t client_addr_len;

    char *expectedRequest;

    printf("connection... new socket : %d \n", socketDescritpor);

    if (recv(socketDescritpor, buff, sizeof(buff), 0) == -1)
    {
        close(socketDescritpor);
        exit(EXIT_FAILURE);
    }


    char* methodFind = strstr(buff,"GET");
    

    if (1)
    {
        FILE *htmlData = fopen("index.html", "r");

        printf("    ----------         HERE     --------------    \n");
        char line[100];
        char responseData[8000];
        while (fgets(line, 100, htmlData) != 0) 
        {
            strcat(responseData, line);
        }   

        char pszRespond[1000]= {0};
        char conntect_length[5];

        sprintf(conntect_length, "%d", strlen(responseData));

        char pszHostAddress[]="127.0.0.1";
        sprintf(pszRespond, "HTTP/1.1 200 OK\n\rServer: 127.0.0.1\n\rContent-Type: text/html\n\r\n\r%s", responseData);

        printf("respond header:\n%s",pszRespond);

        char* dummyResponse=
        "HTTP/1.1 200 OK\n"
        "Server: 127.0.0.1\n"
        "Content-Type: text/html\n"
        "\n"
        "<html><body>\n"
        "<img src=\"../bin/img/mars-top.gif\"></img><br />\n"
        "<img src=\"../bin/img/mars2.jpg\"></img><br />\n"
        "<img src=\"../bin/img/pklogo.gif\"></img><br />\n"
        "</body></html> \n";

        printf("\n\nrespond header2:\n%s",dummyResponse);

        send(socketDescritpor, dummyResponse, strlen(dummyResponse), 0);
    }

}

