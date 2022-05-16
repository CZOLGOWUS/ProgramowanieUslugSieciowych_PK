#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> /* socket() */
#include <netinet/in.h> /* struct sockaddr_in */
#include <arpa/inet.h>  /* inet_pton() */
#include <unistd.h>     /* close() */
#include <string.h>
#include <errno.h>

#include <openssl/hmac.h>
#include <openssl/err.h>
#include <openssl/evp.h>

#define MD5_DIGEST_SIZE 16

int main(int argc, char** argv) {

    int             sockfd;                 /* Desktryptor gniazda. */
    int             retval;                 /* Wartosc zwracana przez funkcje. */
    struct          sockaddr_in remote_addr;/* Gniazdowa struktura adresowa. */
    socklen_t       addr_len;               /* Rozmiar struktury w bajtach. */

        /* Bufor do wyslania przez UDP: */
    char buff[256+MD5_DIGEST_SIZE];
        /* Wiadomosc: */
    char message[256] = "Laboratorium PUS.";
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


    if (argc != 3) {
        fprintf(stderr, "Invocation: %s <IPv4 ADDRESS> <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    ////////////////////////////////// HMAC
    message_len = strlen(message);

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


    ////////////////////////////////// UDP

    strcpy(buff, result);
    strcpy(buff+MD5_DIGEST_SIZE, message);

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

    fprintf(stdout, "Sending message to %s.\n", argv[1]);

    /* sendto() wysyla dane na adres okreslony przez strukture 'remote_addr': */
    retval = sendto(
                 sockfd,
                 buff, strlen(buff),
                 0,
                 (struct sockaddr*)&remote_addr, addr_len
             );

    if (retval == -1) {
        perror("sendto()");
        exit(EXIT_FAILURE);
    }

    close(sockfd);
    exit(EXIT_SUCCESS);
}
