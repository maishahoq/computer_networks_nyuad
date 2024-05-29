#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 21
#define BUFFER_SIZE 1024

// function declarion
void handle_command(int socket, char *command);
void download_file(int socket, const char *filename);
void list_files(int socket, int local);
void change_directory(int socket, const char *foldername, int local);

int main()
{
    int client_socket = 0;
    struct sockaddr_in server_address;
    bzero(&server_address, sizeof(server_address));
    char command[BUFFER_SIZE];
    // char *message;

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

    // Connecting to the server
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("Connect failed");
        close(client_socket);
        exit(EXIT_FAILURE);
    }
    printf("New connection connected to server\n");

    printf("Type your command, the command list is: \n");

    printf("1. USER username \n 2. PASS password \n 3. STOR filename \n 4. RETR filename \n 5. LIST \n 6. !LIST \n");
    printf("7. CWD foldername \n 8. !CWD foldername \n 9.PWD \n 10.!PWD \n 11. QUIT  \n");

    // server message handling code starts//

    // Command prompt loop
    while (1)
    {
        printf("ftp> ");
        if (fgets(command, sizeof(command), stdin) == NULL)
        {
            perror("Error reading command");
            continue;
        }

        // Remove trailing newline character
        command[strcspn(command, "\r\n")] = 0;

        if (strcmp(command, "QUIT") == 0)
        {
            printf("Exiting FTP client.\n");
            break;
        }

        handle_command(client_socket, command);
    }
    // server message handling code ends//

    close(client_socket);
    return 0;
}

void handle_command(int socket, char *command)
{
    if (strncmp(command, "RETR ", 5) == 0)
    {
        download_file(socket, command + 5);
    }
    else if (strcmp(command, "LIST") == 0)
    {
        list_files(socket, 0);
    }
    else if (strcmp(command, "!LIST") == 0)
    {
        list_files(socket, 1);
    }
    else if (strncmp(command, "CWD ", 4) == 0)
    {
        change_directory(socket, command + 4, 0);
    }
    else if (strncmp(command, "!CWD ", 5) == 0)
    {
        change_directory(socket, command + 5, 1);
    }
    else
    {
        write(socket, command, strlen(command));
        write(socket, "\n", 1);

        char response[BUFFER_SIZE];
        int bytes_read = read(socket, response, sizeof(response) - 1);
        if (bytes_read > 0)
        {
            response[bytes_read] = '\0';
            printf("%s", response);
        }
    }
}

void download_file(int socket, const char *filename)
{
    char buffer[BUFFER_SIZE];
    int bytes_read;
    FILE *file;
    char filepath[BUFFER_SIZE];

    // Construct the path to the destination file
    snprintf(filepath, sizeof(filepath), "../client/%s", filename);

    write(socket, "RETR ", 5);
    write(socket, filename, strlen(filename));
    write(socket, "\n", 1);

    file = fopen(filepath, "wb");
    if (file == NULL)
    {
        perror("Could not create file");
        return;
    }

    while ((bytes_read = read(socket, buffer, BUFFER_SIZE)) > 0)
    {
        fwrite(buffer, 1, bytes_read, file);
    }
    fclose(file);
    printf("File downloaded successfully to %s\n", filepath);
}

void list_files(int socket, int local)
{
    char buffer[BUFFER_SIZE];
    FILE *fp;

    if (local)
    {
        fp = popen("ls -l ../client", "r");
    }
    else
    {
        write(socket, "LIST\n", 5);
        fp = fdopen(socket, "r");
    }

    if (fp == NULL)
    {
        perror("Failed to list directory");
        return;
    }

    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        printf("%s", buffer);
    }

    if (!local)
    {
        fclose(fp);
    }
    else
    {
        pclose(fp);
    }
}

void change_directory(int socket, const char *foldername, int local)
{
    char buffer[BUFFER_SIZE];

    if (local)
    {
        if (chdir(foldername) == 0)
        {
            printf("Local directory changed to %s\n", foldername);
        }
        else
        {
            perror("Failed to change local directory");
        }
    }
    else
    {
        snprintf(buffer, sizeof(buffer), "CWD %s\n", foldername);
        write(socket, buffer, strlen(buffer));

        int bytes_read = read(socket, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0)
        {
            buffer[bytes_read] = '\0';
            printf("%s", buffer);
        }
    }
}

// void download_file(int socket, const char *filename)
// {
//     char buffer[BUFFER_SIZE];
//     int bytes_read;
//     FILE *file;
//     char filepath[BUFFER_SIZE];

//     // Construct the path to the destination file
//     snprintf(filepath, sizeof(filepath), "../client/%s", filename);

//     // Sending the GET request to the server
//     write(socket, "GET ", 4);
//     write(socket, filename, strlen(filename));
//     write(socket, "\n", 1);

//     file = fopen(filepath, "wb");
//     if (file == NULL)
//     {
//         perror("Could not create file");
//         return;
//     }
//     printf("File opened successfully in %s\n", filepath);

//     // Reading the file data from the server
//     while ((bytes_read = read(socket, buffer, BUFFER_SIZE)) > 0)
//     {
//         fwrite(buffer, 1, bytes_read, file);
//         printf("Reading the file \n");
//     }
//     fclose(file);
//     printf("File downloaded successfully to %s\n", filepath);
// }