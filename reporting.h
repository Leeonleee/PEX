#ifndef REPORTING_H
#define REPORTING_H

#include "pe_exchange.h"

extern sigset_t signal_mask;

typedef struct {
    char type[5];
    int quantity; // Total quantity at the price level
    int price_level;
    int order_count; // Number of orders at the price level
} ReportingOB;

int compare_orders(const void *a, const void *b);
int order_books(Product* products, int* num_products, Order **buy_order_book, int *num_buy_orders, Order **sell_order_book, int *num_sell_orders); // prints the order book for each product
int positions(TraderInfo* ti, int* num_traders, Product *products, int *num_products); // prints the positions of each trader

#endif