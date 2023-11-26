#ifndef CHANGE_ORDER_BOOK_H
#define CHANGE_ORDER_BOOK_H

#include "pe_exchange.h"
#include "reporting.h"

extern long exchange_balance;

int sort_buy_order(const void *a, const void *b);
int sort_sell_order(const void *a, const void *b);
int match_orders(TraderInfo *ti, Order **buy_order_book, Order **sell_order_book, int *num_buy_orders, int *num_sell_orders, char *latest_order_type, long *exchange_balance, Product *products, int *num_products);
int add_order(Product* products, int* num_products, TraderInfo* ti, int trader_id, Order **buy_order_book, Order **sell_order_book, int *num_orders_ptr, int* num_buy_orders, int* num_sell_orders, char *command, int order_id, char *product, int quantity, int price, int *num_traders, long *exchange_balance, int *order_time);
int amend_orders(Product* products, int* num_products, TraderInfo* ti, int trader_id, Order **buy_order_book, Order **sell_order_book, int* num_buy_orders, int* num_sell_orders, char *command, int order_id, char *product, int quantity, int price, int *num_traders, long *exchange_balance, int *order_time);
int cancel_orders(Product* products, int* num_products, TraderInfo* ti, int trader_id, Order **buy_order_book, Order **sell_order_book, int* num_buy_orders, int* num_sell_orders, char *command, int order_id, char *product, int *num_traders, long *exchange_balance);

#endif