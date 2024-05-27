#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 21

int main()
{
    int sock = 0;
    struct sockaddr_in serv_addr;
    char *salam = "Assalamualaikum from client";
    char buffer[1024] = {0};

    // Creating socket file descriptor
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Converting IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // Connecting to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }

    printf("New connection connected\n");

    // server message handling code starts//

    while (1)
    {
        // Sending a message to the server
        send(sock, salam, strlen(salam), 0);
        printf("Assalamualaikum was sent\n");

        // Reading the server's response
        int valread = read(sock, buffer, 1024);
        printf("%s\n", buffer);
    }

    // server message handling code ends//

    close(sock);
    return 0;
}