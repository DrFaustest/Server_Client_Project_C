/* Auth: Scott Faust
 * Date: 10-14-2024  (Due: 10-14-2024)
 * Course: CSCI-3550 (Sec: 850)
 * Desc:  PROJECT, Server side for a tcp connection exercise.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>

/* Structure to manage resources */
typedef struct {
    int server_socket;  /* Socket descriptor */
    FILE *open_file;    /* Pointer to the currently open file */
} Resources;

/* Global resources (static for file-local visibility) */
static Resources resources = { -1, NULL };

/* Global variables */
static int file_counter = 1;

/* Function prototypes */
static void handle_sigint(int sig);
static void error(const char *msg);
static void receive_file(int client_socket);
static void cleanup(void);

/* Main function */
int main(int argc, char *argv[]) {
    int listening_port;
    char *endptr;
    long port;
    struct sockaddr_in server_address;

    /* Check for correct number of arguments */
    if (argc != 2) {
        fprintf(stderr, "server: USAGE: %s <listening_port>\n", argv[0]);
        exit(1);
    }

    /* Parse command-line argument for listening port */
    port = strtol(argv[1], &endptr, 10);
    if (*endptr != '\0' || port < 1024 || port > 65535) {
        fprintf(stderr, "server: ERROR: Port number is privileged.\n");
        exit(1);
    }
    listening_port = (int)port;

    /* Register signal handler for SIGINT */
    signal(SIGINT, handle_sigint);

    /* Create a socket for listening */
    resources.server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (resources.server_socket < 0) {
        error("server: ERROR: Failed to create socket.");
    }

    /* Configure the server address structure */
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(listening_port);

    /* Bind the server socket to the specified port */
    if (bind(resources.server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        error("server: ERROR: Failed to bind socket.");
    }

    /* Listen for incoming connections */
    if (listen(resources.server_socket, 5) < 0) {
        error("server: ERROR: Failed to listen on socket.");
    }
    printf("server: Awaiting TCP connections over port %d...\n", listening_port);

    /* Main server loop */
    while (1) {
        struct sockaddr_in client_address;
        socklen_t client_address_len = sizeof(client_address);
        int client_socket;

        /* Accept a new client connection */
        client_socket = accept(resources.server_socket, (struct sockaddr *)&client_address, &client_address_len);
        if (client_socket < 0) {
            perror("server: ERROR: Failed to accept connection.");
            continue;
        }

        printf("server: Connection accepted!\n");

        /* Receive the file from the client */
        receive_file(client_socket);

        /* Close the client connection */
        close(client_socket);
        printf("server: Connection closed.\n");
    }

    /* Close the server socket */
    cleanup();
    return 0;
}

/* Signal handler for SIGINT */
static void handle_sigint(int sig) {
    printf("\nserver: Server interrupted. Shutting down.\n");
    cleanup();
    exit(0);
}

/* Cleanup function to release resources */
static void cleanup(void) {
    if (resources.server_socket >= 0) {
        close(resources.server_socket);
        resources.server_socket = -1;
    }
    if (resources.open_file != NULL) {
        fclose(resources.open_file);
        resources.open_file = NULL;
    }
}

/* Print error message and exit program */
static void error(const char *msg) {
    perror(msg);
    cleanup();
    exit(1);
}

/* Receive file data from the client and save it to disk */
static void receive_file(int client_socket) {
    char buffer[1024];
    ssize_t bytes_received;
    char filename[64];

    /* Create a unique filename */
    sprintf(filename, "file-%02d.dat", file_counter++);

    /* Open a new file for writing */
    resources.open_file = fopen(filename, "wb");
    if (resources.open_file == NULL) {
        fprintf(stderr, "server: ERROR: Unable to create: \"%s\".\n", filename);
        return;
    }

    printf("server: Receiving file...\n");

    /* Receive file data from the client */
    while ((bytes_received = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, bytes_received, resources.open_file);
    }

    if (bytes_received < 0) {
        perror("server: ERROR: File transfer failed.");
        fclose(resources.open_file);
        resources.open_file = NULL;
        remove(filename); /* Delete incomplete file */
    } else {
        printf("server: Saving file: \"%s\".\n", filename);
    }

    /* Close the file */
    fclose(resources.open_file);
    resources.open_file = NULL;
    printf("server: Done.\n");
}

