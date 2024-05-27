#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT 21 // Default FTP port acc to google

int main()
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    char *response = "WalaikumSaalam from server";

    // the following creating socket, binding and listening is inspired from lab code//

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // // Setting socket options to allow immediate reuse of the address and port
    // int opt = 1;
    // if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    // {
    //     perror("setsockopt");
    //     exit(EXIT_FAILURE);
    // }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Binding the socket to the specified address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) // error handling, if value less than 0, means binding failed
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listening for incoming connections
    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("FTP server is listening on port %d ...\n", PORT);

    // Accepting incoming connections
    while (1) // we used infinite while loop so that the connection stays until QUIT is pressed
    {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        printf("New connection accepted\n");

        // client message handling starts

        //  Reading the data sent by the client
        int valread = read(new_socket, buffer, 1024);
        printf("%s\n", buffer);

        // Sending a response back to the client
        send(new_socket, response, strlen(response), 0);
        printf("WalaikumSalam was sent to client\n");

        // client message handling ends

        close(new_socket); // Closing the connection with the client
    }

    return 0;
}
