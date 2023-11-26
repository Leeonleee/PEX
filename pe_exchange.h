#ifndef PE_EXCHANGE_H
#define PE_EXCHANGE_H

#include "pe_common.h"


#define LOG_PREFIX "[PEX]"

typedef struct {
	pid_t pid;
	int trader_id;
	int exchange_fd;
	int trader_fd;
	int last_order_id; // ensures new order is is last order id + 1
	long* product_quantity; // stores quantity of each product
    long* product_profit; // stores money spent/received of each product
    int connected; // 1 if connected, 0 if disconnected
} TraderInfo;

typedef struct {
    char *name;
    int buy_levels;
    int sell_levels;
    int product_index; // index of the product
} Product;

typedef struct {
    char type[5]; // BUY or SELL
    int order_id; // Order ID from the trader
    char product[16]; // Product name
    long quantity;
    long price;
    int trader_id; // index of trader than sent order/trader id
    int time; // age of order (lower number = older order)
} Order;



#endif
