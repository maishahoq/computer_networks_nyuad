#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/select.h>

#include <errno.h>

#define PORT 21 // Default FTP port acc to google
#define BUFFER_SIZE 1024

void handle_client(int client_socket);
void handle_command(int client_socket, char *buffer);
void handle_retr(int client_socket, char *filename);
void handle_list(int client_socket, int local);
void handle_cwd(int client_socket, char *foldername, int local);
void sigchld_handler(int signo);


int user_auth(int client_socket, char *command); //USER username PASS password 
void stor_commands(int client_socket, char *filename); // STOR filename
void pwd_commands(int client_socket); // CWD foldername !CWD foldername



int main()
{
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    bzero(&server_address, sizeof(server_address));
    socklen_t client_len = sizeof(client_address);

    fd_set master_set, read_fds; // To handle multiple connections
    int fd_max;                  // To track the maximum file descriptor

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

    // Setting up signal handler
    // Reaping dead child processes
    // struct sigaction sa;
    // sa.sa_handler = sigchld_handler;
    // sigemptyset(&sa.sa_mask);
    // sa.sa_flags = SA_RESTART; // Restart interrupted system calls
    // if (sigaction(SIGCHLD, &sa, NULL) == -1)
    // {
    //     perror("sigaction failed");
    //     exit(EXIT_FAILURE);
    // }

    printf("FTP server is listening on port %d ...\n", PORT);

    FD_ZERO(&master_set);               // Initializing the master set
    FD_SET(server_socket, &master_set); // Adding the server socket to the master set
    fd_max = server_socket;             // Setting the initial max file descriptor to the server socket

    // Accepting incoming connections
    // we used infinite while loop so that the connection stays until QUIT is pressed
    while (1)
    {

        // select() is used to monitor multiple file descriptors to see if any of them are ready for I/O operations
        // The master_set keeps track of all file descriptors, while read_fds is used by select() to check for ready descriptors
        // When a new connection is accepted, the client socket is added to the master_set
        // When a client disconnects, the client socket is removed from the master_set

        read_fds = master_set; // Copying the master set to the read set

        if (select(fd_max + 1, &read_fds, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) {
                continue; 
            }
            perror("select failed");
            exit(EXIT_FAILURE);
        }

        // Accept a new connection
        // Waits for an incoming connection and returns a new socket descriptor clinet_socket for communication with the client

        for (int i = 0; i <= fd_max; i++)
        {
            if (FD_ISSET(i, &read_fds))
            {
                if (i == server_socket)
                {
                    client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_len);
                    if (client_socket < 0)
                    {
                        perror("accept failed");
                        continue; // Continuing to accept new connections
                    }
                    printf("New connection accepted\n");
                    FD_SET(client_socket, &master_set); // Ading the new client socket to the master set

                    if (client_socket > fd_max)
                    {
                        fd_max = client_socket; // Updating the max file descriptor
                    }
                }
                else
                {
                    handle_client(i);       // Handling the client command
                    FD_CLR(i, &master_set); // Removing the client socket from the master set
                }
            }
        }
    }

    close(server_socket);
    return 0;
}

// void sigchld_handler(int signo)
// {
//     while (waitpid(-1, NULL, WNOHANG) > 0)
//         ;
// }

void handle_client(int client_socket)
{
    char buffer[BUFFER_SIZE] = {0};
    int bytes_read;

    while ((bytes_read = read(client_socket, buffer, BUFFER_SIZE - 1)) > 0) {
    buffer[bytes_read] = '\0';
    printf("Received: %s\n", buffer);
    handle_command(client_socket, buffer);
    if (strncmp(buffer, "QUIT", 4) == 0) {
        printf("Client requested to close the connection.\n");
        break;
    }

    if (bytes_read == 0)
    {
        printf("Client disconnected (in server)\n");
    }
    else if (bytes_read < 0)
    {
        perror("read failed (in server) from client");
    }

    close(client_socket); // IS THE ERROR BECAUSE OF THIS?
}

void handle_command(int client_socket, char *buffer)
{
    if (strncmp(buffer, "RETR ", 5) == 0)
    {
        char *filename = buffer + 5;
        filename[strcspn(filename, "\r\n")] = '\0'; // Removing the newline characters

        if (fork() == 0)
        {
            handle_retr(client_socket, filename);
            exit(0);
        }
    }
    else if (strcmp(buffer, "LIST\n") == 0 || strcmp(buffer, "LIST\r\n") == 0)
    {
        if (fork() == 0)
        {
            handle_list(client_socket, 0);
            exit(0);
        }
    }
    else if (strcmp(buffer, "!LIST\n") == 0 || strcmp(buffer, "!LIST\r\n") == 0)
    {
        if (fork() == 0)
        {
            handle_list(client_socket, 1);
            exit(0);
        }
    }
    else if (strncmp(buffer, "CWD ", 4) == 0)
    {
        char *foldername = buffer + 4;
        foldername[strcspn(foldername, "\r\n")] = '\0'; // Remove newline characters
        handle_cwd(client_socket, foldername, 0);
    }
    else if (strncmp(buffer, "!CWD ", 5) == 0)
    {
        char *foldername = buffer + 5;
        foldername[strcspn(foldername, "\r\n")] = '\0'; // Remove newline characters
        handle_cwd(client_socket, foldername, 1);

    } else if (strcmp(buffer, "PWD\n") == 0 || strcmp(buffer, "PWD\r\n") == 0) {
        pwd_commands(client_socket);
    } else if (strcmp(buffer, "!PWD\n") == 0 || strcmp(buffer, "!PWD\r\n") == 0) {
        system("pwd");
    }  else if (strncmp(buffer, "USER ", 5) == 0 || strncmp(buffer, "PASS ", 5) == 0) {
        user_auth(client_socket, buffer);

    } else if (strncmp(buffer, "STOR ", 5) == 0) {
        char *filename = buffer + 5;
        filename[strcspn(filename, "\r\n")] = '\0';
        stor_commands(client_socket, filename);
    } else {
        write(client_socket, "ERROR: Unknown command\n", 23);
    }
    // shutdown(client_socket, SHUT_WR); //should we add this here?
}


void handle_retr(int client_socket, char *filename)
{
    char filepath[BUFFER_SIZE];
    snprintf(filepath, sizeof(filepath), "../server/%s", filename); // Construct the path to the file

    FILE *file = fopen(filepath, "rb");

    if (file == NULL)
    {
        write(client_socket, "ERROR: File not found\n", 22);
        return;
    }

    char buffer[BUFFER_SIZE];
    int bytes_read;

    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0)
    {
        write(client_socket, buffer, bytes_read);
    }
    // shutdown(client_socket, SHUT_WR); // should we add this here?
    fclose(file);
}

void handle_list(int client_socket, int local)
{
    FILE *fp;
    char path[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];

    if (local)
    {
        printf("inside if statement BEFORE opening list in client");
        fp = popen("ls -l ../client", "r");
        printf("inside if statement AFTER opening list in client");
    }
    else
    {
        printf("inside if statement BEFORE opening list in server");
        fp = popen("ls -l ../server", "r");
        printf("inside if statement AFTER opening list in server");
    }

    if (fp == NULL)
    {
        printf("fp was null in list");
        write(client_socket, "ERROR: Failed to list directory\n", 32);
        return;
    }

    // takes the output generated by the ls -l command, reads it line by line, and sends each line to the client through the network socket
    while (fgets(path, sizeof(path), fp) != NULL)
    {
        printf("writing ls -l output into client");
        write(client_socket, path, strlen(path));
    }
    // shutdown(client_socket, SHUT_WR); // should we add this here?
    printf("completed writing ls -l output into client");
    pclose(fp);
}


void pwd_commands(int client_socket) {
    char cwd[BUFFER_SIZE];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        snprintf(cwd, sizeof(cwd), "257 \"%s\"\n", cwd);
        send(client_socket, cwd, strlen(cwd), 0);
    } else {
        send(client_socket, "550 Failed to get current directory.\n", 37, 0);
    }
}



void stor_commands(int client_socket, char *filename) {
    char buffer[BUFFER_SIZE];
    FILE *file = fopen(filename, "wb");

    if (file == NULL) {
        perror("fopen failed");
        send(client_socket, "550 Failed to open file.\n", 25, 0);
        return;
    }

    send(client_socket, "150 File status okay; about to open data connection.\n", 52, 0);

    while (1) {
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            break;
        }
        fwrite(buffer, 1, bytes_received, file);
    }

    fclose(file);
    send(client_socket, "226 Transfer completed.\n", 24, 0);
}

int user_auth(int client_socket, char *command) {
    static int state = 0; // 0: waiting for USER, 1: waiting for PASS
    static char stored_username[50];
    char buffer[BUFFER_SIZE];
    FILE *file = fopen("/Users/maimunaz/Downloads/github/computer_networks_nyuad/project1/users.csv", "r");

    if (file == NULL) {
        perror("fopen failed");
        send(client_socket, "530 Not logged in.\n", 19, 0);
        return 0;
    }

    if (state == 0 && strncmp(command, "USER ", 5) == 0) {
        char username[50];
        sscanf(command + 5, "%s", username);
        while (fscanf(file, "%49s %49s", stored_username, buffer) != EOF) {
            if (strcmp(username, stored_username) == 0) {
                send(client_socket, "331 Username OK, need password.\n", 32, 0);
                state = 1;
                fclose(file);
                return 1;
            }
        }
        send(client_socket, "530 Not logged in.\n", 19, 0);
    } else if (state == 1 && strncmp(command, "PASS ", 5) == 0) {
        char password[50];
        sscanf(command + 5, "%s", password);
        rewind(file); // Go back to the beginning of the file
        while (fscanf(file, "%49s %49s", stored_username, buffer) != EOF) {
            if (strcmp(stored_username, stored_username) == 0 && strcmp(password, buffer) == 0) {
                send(client_socket, "230 User logged in, proceed.\n", 29, 0);
                state = 0;
                fclose(file);
                return 2;
            }
        }
        send(client_socket, "530 Not logged in.\n", 19, 0);
        state = 0;
    } else {
        send(client_socket, "503 Bad sequence of commands.\n", 30, 0);
    }

    fclose(file);
    return 0;
}

void handle_cwd(int client_socket, char *foldername, int local)
{
    char path[BUFFER_SIZE];

    if (local)
    {
        if (chdir(foldername) == 0)
        {
            write(client_socket, "Local directory changed\n", 24);
        }
        else
        {
            write(client_socket, "ERROR: Failed to change local directory\n", 40);
        }
        // shutdown(client_socket, SHUT_WR); // should we add this here?
    }
    else
    {
        snprintf(path, sizeof(path), "../server/%s", foldername);
        if (chdir(path) == 0)
        {
            write(client_socket, "Remote directory changed\n", 25);
        }
        else
        {
            write(client_socket, "ERROR: Failed to change remote directory\n", 41);
        }
        // shutdown(client_socket, SHUT_WR); // should we add this here?
    }
}

// void handle_client(int client_socket)
// {
//     char buffer[BUFFER_SIZE] = {0};
//     int bytes_read;

//     while ((bytes_read = read(client_socket, buffer, BUFFER_SIZE - 1)) > 0)
//     {
//         buffer[bytes_read] = '\0';
//         printf("Received: %s", buffer);

//         if (strncmp(buffer, "GET ", 4) == 0)
//         {
//             char *filename = buffer + 4;
//             filename[strcspn(filename, "\r\n")] = '\0'; // Remove newline characters

//             printf("\n After removing newline characters from filename \n");

//             char filepath[BUFFER_SIZE];
//             snprintf(filepath, sizeof(filepath), "../server/%s", filename); // Construct the path to the file

//             printf("After constructing filepath \n");

//             FILE *file = fopen(filepath, "rb");
//             printf("After opening file \n");

//             if (file == NULL)
//             {
//                 write(client_socket, "ERROR: File not found\n", 22);
//                 continue;
//             }

//             // this line should read and write, read and write as long as there is something to read in file
//             while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0)
//             {
//                 printf("Before Writing the file in server");
//                 write(client_socket, buffer, bytes_read);
//                 printf("After Writing the file in server");
//             }
//             shutdown(client_socket, SHUT_WR);
//             fclose(file);
//         }
//         else
//         {
//             write(client_socket, "ERROR: Unknown command\n", 23);
//         }
//     }
//     close(client_socket);
// }
