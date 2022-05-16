#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> /* socket() */
#include <netinet/in.h> /* struct sockaddr_in */
#include <arpa/inet.h>  /* inet_ntop() */
#include <unistd.h>     /* close() */
#include <string.h>
#include <errno.h>

#include <openssl/hmac.h>
#include <openssl/err.h>
#include <openssl/evp.h>

#define MD5_DIGEST_SIZE 16

int main(int argc, char** argv) {

    int             sockfd; /* Deskryptor gniazda. */
    int             retval; /* Wartosc zwracana przez funkcje. */

    /* Gniazdowe struktury adresowe (dla klienta i serwera): */
    struct          sockaddr_in client_addr, server_addr;

    /* Rozmiar struktur w bajtach: */
    socklen_t       client_addr_len, server_addr_len;

    /* Bufor dla adresu IP klienta w postaci kropkowo-dziesietnej: */
    char            addr_buff[256];

            /* Bufor odebrania przez UDP: */
    char buff[256+MD5_DIGEST_SIZE];
        /* Wiadomosc: */
    char message[256];
        /* Skrot wiadomosci: */
    char result[MD5_DIGEST_SIZE];

        /* Klucz: */
    char key[] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,
                           0x00,0x01,0x02,0x03,0x04,0x05
                          };


    /* Rozmiar tekstu i szyfrogramu: */
    unsigned int message_len, result_len;

    /* Kontekst: */
    HMAC_CTX *ctx;


    if (argc != 2) {
        fprintf(stderr, "Invocation: %s <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

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

    fprintf(stdout, "Server is waiting for UDP datagram...\n");

    client_addr_len = sizeof(client_addr);

    /* Oczekiwanie na dane od klienta: */
    retval = recvfrom(
                 sockfd,
                 buff, sizeof(buff),
                 0,
                 (struct sockaddr*)&client_addr, &client_addr_len
             );
    if (retval == -1) {
        perror("recvfrom()");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "UDP datagram received from %s:%d.\n",
        inet_ntop(AF_INET, &client_addr.sin_addr, addr_buff, sizeof(addr_buff)),
        ntohs(client_addr.sin_port)
        );

    strcpy(message, buff+MD5_DIGEST_SIZE);
    message_len = strlen(message);

    ////////////////////////////////// HMAC
    

    /* Alokacja pamieci dla kontekstu: */
    ctx = HMAC_CTX_new();

    /* Inicjalizacja kontekstu: */
    //HMAC_CTX_init(ctx);
    HMAC_CTX_reset(ctx); // in newer version

    /* Konfiguracja kontekstu: */
    retval = HMAC_Init_ex(ctx, key, sizeof(key), EVP_md5(), NULL);
    if (!retval) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    /* Obliczenie kodu HMAC: */
    retval = HMAC_Update(ctx, message, message_len);
    if (!retval) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    /*Zapisanie kodu HMAC w buforze 'result': */
    retval = HMAC_Final(ctx, result, &result_len);
    if (!retval) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    /*
     * Usuwa wszystkie informacje z kontekstu i zwalnia pamiec zwiazana
     * z kontekstem:
     */
    //HMAC_CTX_cleanup(ctx);
    HMAC_CTX_free(ctx);  //in newer version

    // Weryfikacja HMAC:
    for(int i = 0; i<MD5_DIGEST_SIZE; i++)
    {
        if(buff[i] != result[i])
        {
            printf("Verification failed: HMAC digest do not match!\n");
            exit(EXIT_FAILURE);
        }
    }
    printf("HMAC verification successful.\n");

    fprintf(stdout, "Message: ");
    fwrite(message, message_len, retval, stdout);
    fprintf(stdout, "\n");

    close(sockfd);
    exit(EXIT_SUCCESS);
}
