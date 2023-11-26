#ifndef PE_TRADER_H
#define PE_TRADER_H

#include "pe_common.h"

int is_alphanumeric(char* str);
void cleanup(char *exchange_pipe_name, char *trader_pipe_name, int exchange_fd, int trader_fd, char *buffer);
int read_fifo(int exchange_fd, char** buffer_ptr);
int write_message(int trader_fd, char* command, int* order_id, char* item, int quantity, int price);
int parse_message(int trader_fd, char* buffer, int* order_id);

#endif
