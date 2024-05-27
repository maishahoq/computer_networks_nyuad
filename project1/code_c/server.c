#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT 21 // Default FTP port acc to google
#define BUFFER_SIZE 1024

void handle_client(int client_socket);
void sigchld_handler(int signo);

int main()
{
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_len = sizeof(client_address);

    // the following creating socket, binding and listening is inspired from lab code//

    // Creating socket file descriptor
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Binding the socket to the specified address and port
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY; // Allows the socket to be bound to all available interfaces
    server_address.sin_port = htons(PORT);       // Converts the port number to network byte order

    // If bind fails, close the socket and exit
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("bind failed");
        close(server_socket); // necessary to release the resources associated with the socket if an error occurs, otherwise there might be resouce leakage,
        exit(EXIT_FAILURE);
    }

    // Listening for incoming connections
    // If listen fails, close the socket and exit
    if (listen(server_socket, 3) < 0) // upto 3 pending connections can be queued
    {
        perror("listen failed");
        close(server_socket); // Release resources and make port available
        exit(EXIT_FAILURE);
    }

    signal(SIGCHLD, sigchld_handler); // Reaping dead child processes

    printf("FTP server is listening on port %d ...\n", PORT);

    // Accepting incoming connections
    // we used infinite while loop so that the connection stays until QUIT is pressed
    while (1)
    {
        // Accept a new connection
        // Waits for an incoming connection and returns a new socket descriptor clinet_socket for communication with the client
        client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_len);
        if (client_socket < 0)
        {
            perror("accept failed");
            continue; // Continue to accept new connections
        }
        printf("New connection accepted\n");

        if (fork() == 0)
        {
            // Child process
            close(server_socket); // Child doesn't need the listener
            // client message handling starts
            handle_client(client_socket); // Handle client communication
            // client message handling ends
            exit(0); // Exit child process after handling client
        }
        else
        {
            // Parent process
            close(client_socket); // Parent doesn't need this socket
        }
    }

    return 0;
}

void sigchld_handler(int signo)
{
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}

void handle_client(int client_socket)
{
    char buffer[BUFFER_SIZE] = {0};
    int bytes_read;

    while ((bytes_read = read(client_socket, buffer, BUFFER_SIZE - 1)) > 0)
    {
        buffer[bytes_read] = '\0';
        printf("Received: %s", buffer);

        if (strncmp(buffer, "GET ", 4) == 0)
        {
            char *filename = buffer + 4;
            filename[strcspn(filename, "\r\n")] = '\0'; // Remove newline characters

            printf("\n After removing newline characters from filename \n");

            char filepath[BUFFER_SIZE];
            snprintf(filepath, sizeof(filepath), "../server/%s", filename); // Construct the path to the file

            printf("After constructing filepath \n");

            FILE *file = fopen(filepath, "rb");
            printf("After opening file \n");

            if (file == NULL)
            {
                write(client_socket, "ERROR: File not found\n", 22);
                continue;
            }

            // this line should read and write, read and write as long as there is something to read in file
            while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0)
            {
                printf("Before Writing the file in server");
                write(client_socket, buffer, bytes_read);
                printf("After Writing the file in server");
            }
            fclose(file);
        }
        else
        {
            write(client_socket, "ERROR: Unknown command\n", 23);
        }
    }
    close(client_socket);
}
