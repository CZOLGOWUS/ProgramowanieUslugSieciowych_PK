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
    int tmp;

    /* Gniazdowe struktury adresowe (dla klienta i serwera): */
    struct          sockaddr_in client_addr, server_addr;

    /* Rozmiar struktur w bajtach: */
    socklen_t       client_addr_len, server_addr_len;

    /* Bufor dla adresu IP klienta w postaci kropkowo-dziesietnej: */
    char            addr_buff[256];

        /* Bufor na odebrany szyfrogram: */
    unsigned char ciphertext[1024];
        /* Bufor na wiadomosc z MAC: */
    char buff[1024];
        /* Wiadomosc: */
    char message[256];
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
    unsigned int message_len, result_len, ciphertext_len, buff_len;

    // Kontekst dla HMAC:
    HMAC_CTX *hmac_ctx;

    // Kontekst dla deszyfrowania:
    EVP_CIPHER_CTX *cipher_ctx;

    // Zastosowana metoda szyfrowania
    const EVP_CIPHER* cipher = EVP_aes_128_ecb();
    

    if (argc != 2) {
        fprintf(stderr, "Invocation: %s <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

#pragma region udp

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
                 ciphertext, sizeof(ciphertext),
                 0,
                 (struct sockaddr*)&client_addr, &client_addr_len
             );
    if (retval == -1) {
        perror("recvfrom()");
        exit(EXIT_FAILURE);
    }
    
    ciphertext_len = retval;

    fprintf(stdout, "UDP datagram received from %s:%d.\n",
        inet_ntop(AF_INET, &client_addr.sin_addr, addr_buff, sizeof(addr_buff)),
        ntohs(client_addr.sin_port)
        );

    #pragma endregion udp
    
#pragma region cipher
    fprintf(stdout, "Decrypting with ECB...\n");

    // Zaladowanie tekstowych opisow bledow:
    ERR_load_crypto_strings();

    // Utworzenie kontekstu pod deszyfrowanie:
    cipher_ctx = EVP_CIPHER_CTX_new();

    // Konfiguracja kontekstu dla odszyfrowywania:
    retval = EVP_DecryptInit_ex(cipher_ctx, cipher, NULL, keyB, NULL);
    if (!retval) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    EVP_CIPHER_CTX_set_padding(cipher_ctx, 1);

    // Odszyfrowywanie:
    retval = EVP_DecryptUpdate(cipher_ctx, (unsigned char*)buff, &buff_len,
                               ciphertext, ciphertext_len);

    if (!retval) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    retval = EVP_DecryptFinal_ex(cipher_ctx, (unsigned char*)buff + buff_len, &tmp);
    if (!retval) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    buff_len += tmp;
    buff[buff_len] = '\0';

    EVP_CIPHER_CTX_free(cipher_ctx);

    // Zwolnienie tekstowych opisow bledow:
    ERR_free_strings();

#pragma endregion cipher
    
#pragma region hmac

    fprintf(stdout, "Verifying HMAC...\n");

    strcpy(message, buff+MD5_DIGEST_SIZE);
    message_len = strlen(message);

    /* Alokacja pamieci dla kontekstu: */
    hmac_ctx = HMAC_CTX_new();

    /* Inicjalizacja kontekstu: */
    HMAC_CTX_reset(hmac_ctx);

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

    /*Zapisanie kodu HMAC w buforze 'hmac_buff': */
    retval = HMAC_Final(hmac_ctx, hmac_buff, &result_len);
    if (!retval) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }


    /* Usuwa wszystkie informacje z kontekstu i zwalnia pamiec zwiazana z kontekstem: */
    HMAC_CTX_free(hmac_ctx);

    /* Weryfikacja HMAC: */
    for(int i = 0; i<MD5_DIGEST_SIZE; i++)
    {
        if(buff[i] != hmac_buff[i])
        {
            printf("Verification failed: HMAC digest do not match!\n");
            exit(EXIT_FAILURE);
        }
    }
    printf("HMAC verification successful.\n");

#pragma endregion hmac

    fprintf(stdout, "Message: ");
    fwrite(message, message_len, retval, stdout);
    fprintf(stdout, "\n");


    close(sockfd);
    exit(EXIT_SUCCESS);
}
