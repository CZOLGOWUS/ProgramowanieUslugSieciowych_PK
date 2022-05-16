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

        /* Bufor na odebrany szyfrogram: */
    //unsigned char ciphertext[256+MD5_DIGEST_SIZE];
    unsigned char ciphertext[1024];
        /* Bufor na wiadomosc z MAC: */
    char buff[1024]; //256+MD5_DIGEST_SIZE
        /* Wiadomosc: */
    char message[256];
        /* Skrot wiadomosci: */
    char result[MD5_DIGEST_SIZE];

        /* Klucze: */
    char keyA[] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,
                           0x00,0x01,0x02,0x03,0x04,0x05
                          };

    char keyB[] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,
                           0x00,0x01,0x02,0x03,0x04,0x05
                          };


    /* Rozmiar tekstu i szyfrogramu: */
    unsigned int message_len, result_len, ciphertext_len;

    /* Kontekst: */
    HMAC_CTX *hmac_ctx;


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
                 ciphertext, sizeof(ciphertext),
                 0,
                 (struct sockaddr*)&client_addr, &client_addr_len
             );
    if (retval == -1) {
        perror("recvfrom()");
        exit(EXIT_FAILURE);
    }
    
    ciphertext_len = retval;
    printf("Ciphertext_len: %d\n",ciphertext_len);
    printf("Ciphertext:\n%s\n\n",ciphertext);

    fprintf(stdout, "UDP datagram received from %s:%d.\n",
        inet_ntop(AF_INET, &client_addr.sin_addr, addr_buff, sizeof(addr_buff)),
        ntohs(client_addr.sin_port)
        );



    ////////////////////////////////// cipher

        /* Kontekst: */
    EVP_CIPHER_CTX *cipher_ctx;

    const EVP_CIPHER* cipher;

    /* Zaladowanie tekstowych opisow bledow: */
    ERR_load_crypto_strings();

    /*
     * Utworzenie kontekstu pod deszyfrowanie
     */
    cipher_ctx = EVP_CIPHER_CTX_new();

    /* Konfiguracja kontekstu dla odszyfrowywania: */
    cipher = EVP_aes_128_ecb();
    fprintf(stdout, "Decrypting with ECB...\n\n");
    retval = EVP_DecryptInit_ex(cipher_ctx, cipher, NULL, keyB, NULL);
    if (!retval) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    /*
     * Domyslnie OpenSSL stosuje padding. 0 - padding nie bedzie stosowany.
     * Funkcja moze byc wywolana tylko po konfiguracji kontekstu
     * dla szyfrowania/deszyfrowania (odzielnie dla kazdej operacji).
     */
    EVP_CIPHER_CTX_set_padding(cipher_ctx, 1);

    int buff_len;
    /* Odszyfrowywanie: */
    retval = EVP_DecryptUpdate(cipher_ctx, (unsigned char*)buff, &buff_len,
                               ciphertext, ciphertext_len);

    if (!retval) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    /*
     * Prosze zwrocic uwage, ze rozmiar bufora 'plaintext' musi byc co najmniej o
     * rozmiar bloku wiekszy od dlugosci szyfrogramu (na padding):
     */
    int tmp;
    retval = EVP_DecryptFinal_ex(cipher_ctx, (unsigned char*)buff + buff_len, &tmp);
    if (!retval) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    buff_len += tmp;
    printf("updated buff_len: %d\n",buff_len);
    buff[buff_len] = '\0';
    fprintf(stdout, "Plaintext:\n%s\n", buff);

    EVP_CIPHER_CTX_free(cipher_ctx);
    /* Zwolnienie tekstowych opisow bledow: */
    ERR_free_strings();

    ////////////////////////////////// HMAC
    
    strcpy(message, buff+MD5_DIGEST_SIZE);
    message_len = strlen(message);

    /* Alokacja pamieci dla kontekstu: */
    hmac_ctx = HMAC_CTX_new();

    /* Inicjalizacja kontekstu: */
    //HMAC_CTX_init(hmac_ctx);
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

    /*Zapisanie kodu HMAC w buforze 'result': */
    retval = HMAC_Final(hmac_ctx, result, &result_len);
    if (!retval) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    /*
     * Usuwa wszystkie informacje z kontekstu i zwalnia pamiec zwiazana
     * z kontekstem:
     */
    //HMAC_CTX_cleanup(hmac_ctx);
    HMAC_CTX_free(hmac_ctx);  //in newer version

    // Weryfikacja HMAC:
    for(int i = 0; i<MD5_DIGEST_SIZE; i++)
    {
        if(buff[i] != result[i])
        {
            printf("Verification failed: HMAC digest do not match!\n");
            exit(EXIT_FAILURE);
        }
        else{
            printf(":)\n");
        }
    }
    printf("HMAC verification successful.\n");

    fprintf(stdout, "Message: ");
    fwrite(message, message_len, retval, stdout);
    fprintf(stdout, "\n");

    close(sockfd);
    exit(EXIT_SUCCESS);
}
