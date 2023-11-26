#ifndef RUN_EXCHANGE_H
#define RUN_EXCHANGE_H

#include "pe_exchange.h"
#include "read_fifo.h"
#include "parse_message.h"

extern int sig_flag;
extern int sender_pid;
extern int disconnected_flag;
extern int disconnected_pid;
extern int num_active_traders;

extern sigset_t signal_mask;


int run_exchange(Product *products, int* num_products, TraderInfo* ti, int *num_traders, int exchange_fd, int trader_fd, Order **buy_order_book, Order **sell_order_book, int* num_orders_ptr, int* num_buy_orders, int* num_sell_orders, long *exchange_balance, int *order_time);

#endif