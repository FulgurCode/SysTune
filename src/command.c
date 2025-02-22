#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Function to execute a command and capture its output
char* execute_command(const char* command) {
    FILE* fp = popen(command, "r");
    if (!fp) {
        perror("Failed to execute command");
        exit(EXIT_FAILURE);
    }

    // Read the output into a buffer
    char* buffer = malloc(4096);
    size_t len = fread(buffer, 1, 4096, fp);
    buffer[len] = '\0'; // Null-terminate the string

    pclose(fp);
    return buffer;
}
