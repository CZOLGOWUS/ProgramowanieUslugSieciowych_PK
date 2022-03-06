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

#include <sys/types.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/time.h>

#define MAX_CLIENTS 20      //maskymalna obslugiwana liczba klientow
#define BUFF_SIZE 256       //maskymalna dlugosc buffora danych


int main(int argc, char** argv) {

    int main_sd;                // Deskryptor gniazda glownego.
    int client_sd[MAX_CLIENTS]; // Deksryptory klientow.
    int retval;
    int max_sd;                 // Maks. deskryptor (potrzebne funkcji select)

    /* Gniazdowe struktury adresowe (dla klienta i serwera): */
    struct          sockaddr_in client_addr, server_addr;

    /* Rozmiar struktur w bajtach: */
    socklen_t       client_addr_len, server_addr_len;

    /* Bufor wykorzystywany przez recvfrom() i sendto(): */
    char            buff[BUFF_SIZE];

    /* Bufor dla adresu IP klienta w postaci kropkowo-dziesietnej: */
    char            addr_buff[256];

    // zbior deskryptorow uzywany przez select
    fd_set          descriptors_set;
    
    
    if (argc != 2) {
        fprintf(stderr, "Invocation: %s <PORT>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Utworzenie gniazda dla protokolu TCP: */
    main_sd = socket(PF_INET, SOCK_STREAM, 0);
    if (main_sd == -1) {
        perror("socket()");
        exit(EXIT_FAILURE);
    }


    /* Wyzerowanie struktury adresowej serwera: */
    memset(&server_addr, 0, sizeof(server_addr));
    //initialise all client_socket[] to 0 so not checked 
    for (int i = 0; i < MAX_CLIENTS; i++)  
    {  
        client_sd[i] = 0;
    }
    
    /* Domena komunikacyjna (rodzina protokolow): */
    server_addr.sin_family          =       AF_INET;
    /* Adres nieokreslony (ang. wildcard address): */
    server_addr.sin_addr.s_addr     =       htonl(INADDR_ANY);
    /* Numer portu: */
    server_addr.sin_port            =       htons(atoi(argv[1]));
    /* Rozmiar struktury adresowej serwera w bajtach: */
    server_addr_len                 =       sizeof(server_addr);

    /* Powiazanie "nazwy" (adresu IP i numeru portu) z gniazdem: */
    if (bind(main_sd, (struct sockaddr*) &server_addr, server_addr_len) == -1) {
        perror("bind()");
        exit(EXIT_FAILURE);
    }

     //try to specify maximum of 3 pending connections for the master socket 
    if (listen(main_sd, 3) < 0)  
    {  
        perror("listen");  
        exit(EXIT_FAILURE);  
    }  
         
    //accept the incoming connection 
    puts("Waiting for connections ...");  

    

    int current_sd; //temp
    int activity;
    int new_sd;
    while(1)
    {
        //clear the socket set 
        FD_ZERO(&descriptors_set);  
        
        //add master socket to set 
        FD_SET(main_sd, &descriptors_set);  
        max_sd = main_sd;  
                
        //add child sockets to set 
        for (int i = 0 ; i < MAX_CLIENTS ; i++)  
        {  
            //socket descriptor 
            current_sd = client_sd[i];  

            //if valid socket descriptor then add to read list 
            if(current_sd > 0)  
                FD_SET( current_sd , &descriptors_set);
                    
            //highest file descriptor number, need it for the select function 
            if(current_sd > max_sd)  
                max_sd = current_sd;  
        }  
        

        //wait for an activity on one of the sockets
        activity = select( max_sd + 1 , &descriptors_set , NULL , NULL , NULL);  
        
        if ((activity < 0) && (errno!=EINTR))  
        {  
            printf("select() error");  
        }  
                
        
        if (FD_ISSET(main_sd, &descriptors_set))  
        {  
            //Nowe poloczenie
            if ((new_sd = accept(main_sd, 
                    (struct sockaddr *)&client_addr, (socklen_t*)&client_addr_len))<0)  
            {  
                perror("accept() error");  
                exit(EXIT_FAILURE);  
            }  
                
            //inform user of socket number - used in send and receive commands 
            printf("Nowe poloczenie %d\n  ip: %s\n  port : %d\n\n" , new_sd , inet_ntoa(client_addr.sin_addr) , ntohs(client_addr.sin_port)); 
                    
            //dodaj nowego klienta
            for (int i = 0; i < MAX_CLIENTS; i++)  
            {  
                if( client_sd[i] == 0 )  
                {  
                    client_sd[i] = new_sd;  
                    break;  
                }  
            }  
        }  
                
        //else its some IO operation on some other socket
        for (int i = 0; i < MAX_CLIENTS; i++)  
        {  
            current_sd = client_sd[i];  
            if (FD_ISSET(current_sd, &descriptors_set))  
            {  
                //Check if it was for closing , and also read the incoming message 
                if ((retval = read(current_sd, buff, BUFF_SIZE)) == 0)  
                {  
                    //Somebody disconnected , get his details and print 
                    getpeername(current_sd , (struct sockaddr*)&client_addr , \
                        (socklen_t*)&client_addr_len);  
                    printf("Klient zamyka polonczenie\n  ip: %s\n  port: %d\n\n" , 
                            inet_ntoa(client_addr.sin_addr) , ntohs(client_addr.sin_port));  
                            
                    close(current_sd);  
                    client_sd[i] = 0;  
                }  
                else 
                {  
                    buff[retval] = '\0';  
                    printf("sd: %d\n",current_sd);
                    for (int j = 0; j < MAX_CLIENTS; j++)  
                    {  
                        if( client_sd[j] == 0 )  
                        {  
                            break;  
                        }
                        else if(client_sd[j]!=current_sd)
                        {
                            //write(j,buff,strlen(buff));
                            printf("sd: %d\n",j);
                            send(client_sd[j], buff, strlen(buff), 0);  
                        }  
                    }  
                    //broadcast_msg(current_sd,buff,0);
                }
            }  
        }  
    }
    close(main_sd);
    exit(EXIT_SUCCESS);
}
