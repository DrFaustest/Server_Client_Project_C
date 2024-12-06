/* Auth: Scott Faust
 * Date: 10-14-2024  (Due: 10-14-2024)
 * Course: CSCI-3550 (Sec: 850)
 * Desc:  PROJECT, client side for a tcp connection exercise.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>

/* Structure to track resources */
typedef struct {
    int sockfd;          /* Socket descriptor */
    char *file_contents; /* Pointer to allocated file memory */
} Resources;

/* Global resources structure (static for local linkage) */
static Resources resources = { -1, NULL };

/* Function prototypes (static for local linkage) */
static void SIGINT_handler(int sig);
static void send_file(const char *server_ip, int server_port, const char *filename);
static void cleanup(void);

/* Main function */
int main(int argc, char *argv[]) {
    char *server_ip;
    int server_port;
    int i;

    /* Check for correct number of arguments */
    if (argc < 4) {
        fprintf(stderr, "client: USAGE: %s <server_IP> <server_Port> file1 file2 ...\n", argv[0]);
        exit(1);
    }

    /* Register signal handler for SIGINT (CTRL+C) */
    signal(SIGINT, SIGINT_handler);

    /* Parse command-line arguments */
    server_ip = argv[1];
    server_port = atoi(argv[2]);

    /* Check if the port is valid and unprivileged */
    if (server_port < 1024 || server_port > 65535) {
        fprintf(stderr, "client: ERROR: Port number is privileged.\n");
        exit(1);
    }

    /* Verify each file exists before processing */
    for (i = 3; i < argc; i++) {
        if (access(argv[i], F_OK) != 0) { /* File does not exist */
            fprintf(stderr, "client: ERROR: File \"%s\" does not exist. Skipping.\n", argv[i]);
            continue;
        }

        /* Process and send file */
        send_file(server_ip, server_port, argv[i]);
    }

    /* Exit program */
    printf("client: File transfer(s) complete.\n");
    printf("client: Goodbye!\n");
    cleanup();
    return 0;
}

/* Signal handler for SIGINT (CTRL+C) */
static void SIGINT_handler(int sig) {
    fprintf(stderr, "client: Client interrupted. Shutting down.\n");
    cleanup();
    exit(EXIT_FAILURE);
}

/* Cleanup function to release resources */
static void cleanup(void) {
    if (resources.sockfd >= 0) {
        close(resources.sockfd);
        resources.sockfd = -1; /* Reset socket descriptor */
    }
    if (resources.file_contents != NULL) {
        free(resources.file_contents);
        resources.file_contents = NULL; /* Reset file contents pointer */
    }
}

/* Send the contents of a file to the server */
static void send_file(const char *server_ip, int server_port, const char *filename) {
    FILE *file;
    long file_size;
    size_t bytes_read;
    struct sockaddr_in server_addr;

    /* Open the file for reading */
    file = fopen(filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "client: ERROR: Unable to open file \"%s\"\n", filename);
        return;
    }

    /* Get the file size */
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    fseek(file, 0, SEEK_SET); /* Reset file pointer to beginning */

    /* Ensure file size does not exceed 10MB */
    if (file_size > 10 * 1024 * 1024) {
        fclose(file);
        fprintf(stderr, "client: ERROR: File \"%s\" exceeds 10MB limit. Skipping.\n", filename);
        return;
    }

    /* Allocate memory for the file contents */
    resources.file_contents = (char *)malloc(file_size);
    if (resources.file_contents == NULL) {
        fclose(file);
        fprintf(stderr, "client: ERROR: Memory allocation failed.\n");
        return;
    }

    /* Read the file contents into memory */
    bytes_read = fread(resources.file_contents, 1, file_size, file);
    if (bytes_read != file_size) {
        fclose(file);
        fprintf(stderr, "client: ERROR: File read error.\n");
        cleanup();
        return;
    }

    /* Close the file */
    fclose(file);

    /* Create a socket */
    resources.sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (resources.sockfd < 0) {
        fprintf(stderr, "client: ERROR: Failed to create socket.\n");
        cleanup();
        return;
    }

    /* Initialize server address structure */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    /* Check if server IP is valid */
    if (server_addr.sin_addr.s_addr == INADDR_NONE) {
        fprintf(stderr, "client: ERROR: Invalid server IP address.\n");
        cleanup();
        return;
    }

    /* Connect to the server */
    printf("client: Connecting to %s:%d...\n", server_ip, server_port);
    if (connect(resources.sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        fprintf(stderr, "client: ERROR: Connection failed.\n");
        cleanup();
        return;
    }
    printf("client: Success!\n");

    /* Send the file contents to the server */
    printf("client: Sending: \"%s\"...\n", filename);
    if (send(resources.sockfd, resources.file_contents, file_size, 0) < 0) {
        fprintf(stderr, "client: ERROR: Send failed.\n");
    } else {
        printf("client: Done.\n");
    }

    /* Close the socket and free memory */
    cleanup();
}



