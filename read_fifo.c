#include "read_fifo.h"

int read_fifo(TraderInfo* ti, int trader_id, char** buffer_ptr) {
    int i = 0;
    char c;
    char* buffer = *buffer_ptr;
    int trader_fd = ti[trader_id].trader_fd;
    while (read(trader_fd, &c, 1) == 1) {
        if (c == ';') {
            break; // found delimiter
        }
        buffer = realloc(buffer, (i + 2) * sizeof(char));
        if (buffer == NULL) {
            perror("Error reallocating memory for buffer");
            return 1;
        }
        buffer[i] = c;
        i++;
    }
    buffer[i] = '\0'; // Add null terminator
    *buffer_ptr = buffer; // Update buffer pointer
    return 0;
}