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

        /* Bufor na szyfrogram: */
    unsigned char ciphertext[1024];
        /* Bufor na wiadomosc z MAC: */
    char buff[1024];
        /* Wiadomosc: */
    char message[256] = "Laboratorium PUS.";
        /* HMAC wiadomosci: */
    char hmac_buff[MD5_DIGEST_SIZE];

        /* Klucze: */
    char keyA[] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,
                           0x00,0x01,0x02,0x03,0x04,0x05
                          };

    char keyB[] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,
                           0x00,0x01,0x02,0x03,0x04,0x05
                          };

    // rozmiary poszceglnych bufforow 
    unsigned int message_len, result_len, ciphertext_len;

    // Kontekst dla HMAC:
    HMAC_CTX *hmac_ctx;

    // Kontekst dla szyfrowania:
    EVP_CIPHER_CTX *cipher_ctx;

    // Zastosowana metoda szyfrowania
    const EVP_CIPHER* cipher = EVP_aes_128_ecb();;


    if (argc != 3) {
        fprintf(stderr, "Invocation: %s <IPv4 ADDRESS> <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

#pragma region hmac
    fprintf(stdout, "Generating HMAC...\n");

    message_len = strlen(message);

    /* Alokacja pamieci dla kontekstu: */
    hmac_ctx = HMAC_CTX_new();

    /* Inicjalizacja kontekstu: */
    HMAC_CTX_reset(hmac_ctx); // in newer version

    /* Konfiguracja kontekstu: */
    retval = HMAC_Init_ex(hmac_ctx, keyA, sizeof(keyA), EVP_md5(), NULL);
    if (!retval) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    /* Obliczenie kodu HMAC: */
    retval = HMAC_Update(hmac_ctx, message, message_len);
    if (!retval) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    /* Zapisanie kodu HMAC w buforze 'hmac_buff': */
    retval = HMAC_Final(hmac_ctx, hmac_buff, &result_len);
    if (!retval) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    /* Usuwa wszystkie informacje z kontekstu i zwalnia pamiec zwiazana z kontekstem: */
    HMAC_CTX_free(hmac_ctx);

#pragma endregion hmac

#pragma region cipher
    fprintf(stdout, "Encrypting with ECB...\n");

    strcpy(buff, hmac_buff);
    strcpy(buff+MD5_DIGEST_SIZE, message);

    /* Zaladowanie tekstowych opisow bledow: */
    ERR_load_crypto_strings();

    /* Alokacja pamieci dla kontekstu: */
    cipher_ctx = EVP_CIPHER_CTX_new();

    /* Inicjalizacja kontekstu: */
    EVP_CIPHER_CTX_init(cipher_ctx);

    /* Konfiguracja kontekstu dla szyfrowania: */
    retval = EVP_EncryptInit_ex(cipher_ctx, cipher, NULL, keyB, NULL);
    if (!retval) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    EVP_CIPHER_CTX_set_padding(cipher_ctx, 1);

    int buff_len = strlen(buff);

    /* Szyfrowanie: */
    retval = EVP_EncryptUpdate(cipher_ctx, ciphertext, &ciphertext_len,
                               (unsigned char*)buff, buff_len);

    if (!retval) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    int tmp;
    retval = EVP_EncryptFinal_ex(cipher_ctx, ciphertext + ciphertext_len, &tmp);
    if (!retval) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    /* Usuwa wszystkie informacje z kontekstu i zwalnia pamiec zwiazana z kontekstem: */

    EVP_CIPHER_CTX_free(cipher_ctx);

    ciphertext_len += tmp;

#pragma endregion cipher

#pragma region udp

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
                 ciphertext, ciphertext_len,
                 0,
                 (struct sockaddr*)&remote_addr, addr_len
             );

    if (retval == -1) {
        perror("sendto()");
        exit(EXIT_FAILURE);
    }

    close(sockfd);
#pragma endregion udp

    exit(EXIT_SUCCESS);
}
