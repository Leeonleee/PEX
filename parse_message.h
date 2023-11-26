#ifndef PARSE_MESSAGE_H
#define PARSE_MESSAGE_H

#include "pe_exchange.h"
#include "change_order_book.h"

extern sigset_t signal_mask;


int parse_message(Product *products, int *num_products, TraderInfo* ti, int *num_traders, int trader_id, char *buffer, Order **buy_order_book, Order **sell_order_book, int *num_orders_ptr, int* num_buy_orders, int* num_sell_orders, long *exchange_balance, int *order_time);

#endif