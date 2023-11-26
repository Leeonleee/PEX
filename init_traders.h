#ifndef INIT_TRADERS_H
#define INIT_TRADERS_H

#include "pe_exchange.h"

#define MAX_INT 11

extern sigset_t signal_mask;
extern int num_active_traders;

int init_traders(char *argv[], TraderInfo* ti, pid_t *pid, char *exchange_pipe_name, char *trader_pipe_name, int exchange_fd, int trader_fd, int num_traders, int num_products);

#endif