#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 21
#define BUFFER_SIZE 1024

// function declarion
void download_file(int socket, const char *filename);

int main()
{
    int client_socket = 0;
    struct sockaddr_in server_address;
    char *message;

    // Creating socket file descriptor, creates a TCP socket for the client
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1)
    {
        perror("\n Socket creation error \n");
        exit(EXIT_FAILURE);
    }

    //    Sets up the server address and port
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1"); // inet_addr("127.0.0.1") : Specifies the loopback address(localhost)
    server_address.sin_port = htons(PORT);

    // // Converting IPv4 and IPv6 addresses from text to binary form
    // if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    // {
    //     printf("\nInvalid address/ Address not supported \n");
    //     return -1;
    // }

    // Connecting to the server
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("Connect failed");
        close(client_socket);
        exit(EXIT_FAILURE);
    }
    printf("New connection connected to server\n");

    // server message handling code starts//

    // Example command to get a file
    message = "example.txt";
    download_file(client_socket, message);

    // server message handling code ends//

    close(client_socket);
    return 0;
}

void download_file(int socket, const char *filename)
{
    char buffer[BUFFER_SIZE];
    int bytes_read;
    FILE *file;
    char filepath[BUFFER_SIZE];

    // Construct the path to the destination file
    snprintf(filepath, sizeof(filepath), "../client/%s", filename);

    // Sending the GET request to the server
    write(socket, "GET ", 4);
    write(socket, filename, strlen(filename));
    write(socket, "\n", 1);

    file = fopen(filepath, "wb");
    if (file == NULL)
    {
        perror("Could not create file");
        return;
    }
    printf("File opened successfully in %s\n", filepath);

    // Reading the file data from the server
    while ((bytes_read = read(socket, buffer, BUFFER_SIZE)) > 0)
    {
        fwrite(buffer, 1, bytes_read, file);
        printf("Reading the file \n");
    }
    fclose(file);
    printf("File downloaded successfully to %s\n", filepath);
}