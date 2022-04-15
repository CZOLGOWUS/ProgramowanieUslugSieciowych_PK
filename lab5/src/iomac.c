#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <net/if.h> /* struct ifconf, struct ifreq */
#include <unistd.h>
#include <net/if_arp.h>
#include <sys/ioctl.h>



void print_MAC_MTU(int socket_fd, struct ifreq* if_info)
{
    int i, retval;
    u_int8_t macAddress[6];

    //get current mac and MTU info
    if ((retval = ioctl(socket_fd, SIOCGIFMTU, if_info)) == -1) 
    {
        perror("ioctl() getting MTU");
        exit(EXIT_FAILURE);
    }

    printf("MTU: %d \n", if_info->ifr_mtu);
    printf("MAC: ");

    //get current mac and MTU info
    if ((retval = ioctl(socket_fd, SIOCGIFHWADDR, if_info)) == -1) 
    {
        perror("ioctl() getting MAC");
        exit(EXIT_FAILURE);
    }

    memcpy(macAddress, if_info->ifr_hwaddr.sa_data, sizeof(macAddress));

    for (i = 0; i < 6; i++)
    {
        printf("%02X", macAddress[i]);

        if (i != 5) 
            printf(":");
    }

}



int main(int argc, char** argv) {

    int                     sockfd, retval;
    struct ifreq if_info;


    if (argc != 4) 
    {
        fprintf(stderr, "Invocation: %s <IF NAME> <NEW MAC ADDRESS> <NEW MTU>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if ((sockfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) 
    {
        perror("socket()");
        exit(EXIT_FAILURE);
    }

    strcpy(if_info.ifr_name, argv[1]);

    printf("before change\n");
    print_MAC_MTU(sockfd, &if_info);
    printf("\n\n");

    /* Adres MAC: */
    if_info.ifr_hwaddr.sa_family = ARPHRD_ETHER;

    retval = sscanf(
                 argv[2], "%2x:%2x:%2x:%2x:%2x:%2x",
                 (unsigned int*)&if_info.ifr_hwaddr.sa_data[0],
                 (unsigned int*)&if_info.ifr_hwaddr.sa_data[1],
                 (unsigned int*)&if_info.ifr_hwaddr.sa_data[2],
                 (unsigned int*)&if_info.ifr_hwaddr.sa_data[3],
                 (unsigned int*)&if_info.ifr_hwaddr.sa_data[4],
                 (unsigned int*)&if_info.ifr_hwaddr.sa_data[5]
             );

    if (retval != 6) 
    {
        fprintf(stderr, "Invalid address format!\n");
        exit(EXIT_FAILURE);
    }


    if((retval = ioctl(sockfd, SIOCSIFHWADDR, &if_info)) == -1)
    {
        perror("ioctl() overwriting MAC");
        exit(EXIT_FAILURE);
    }

    if_info.ifr_mtu = atoi(argv[3]);

    if((retval = ioctl(sockfd, SIOCSIFMTU, &if_info)) == -1)
    {
        perror("ioctl() over writing MTU");
        exit(EXIT_FAILURE);
    }

    
    printf("after change\n");
    print_MAC_MTU(sockfd, &if_info);
    printf("\n\n");

    close(sockfd);
    exit(EXIT_SUCCESS);
}
