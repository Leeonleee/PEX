#include "reporting.h"

int compare_orders(const void *a, const void *b) {
    Order *order_a = (Order *)a;
    Order *order_b = (Order *)b;
    // Sort from price high->low, oldest->newest
    if (order_a->price < order_b->price) {
        return 1;
    } else if (order_a->price == order_b->price) {
        if (order_a->time > order_b->time) {
            return 1;
        } else {
            return -1;
        }
    } else {
        return -1;
    }
}

int order_books(Product *products, int *num_products, Order **buy_order_book, int *num_buy_orders, Order **sell_order_book, int *num_sell_orders) {
    sigprocmask(SIG_BLOCK, &signal_mask, NULL);
    printf("%s\t--ORDERBOOK--\n", LOG_PREFIX);
    sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
    // Loop through products
    for (int i = 0; i < *num_products; i++) {
        // Calculate buy levels and sell levels
        int num_buy_levels = 0;
        int num_sell_levels = 0;
        int last_buy_price = -1;
        int last_sell_price = -1;
        // Loop through BUY order book
        for (int j = 0; j < *num_buy_orders; j++) {
            // Only add to buy levels if product matches and if quantity is not 0
            if (strcmp(products[i].name, (*buy_order_book)[j].product) == 0 && 
                (*buy_order_book)[j].quantity > 0) {
                // Check if price level already exists
                if ((*buy_order_book)[j].price != last_buy_price) {
                    num_buy_levels++;
                    last_buy_price = (*buy_order_book)[j].price;
                }

            }
            products[i].buy_levels = num_buy_levels;
        }
        // Loop through SELL order book
        for (int j = 0; j < *num_sell_orders; j++) {
            // Only add to sell levels if product matches and if quantity is not 0
            if (strcmp(products[i].name, (*sell_order_book)[j].product) == 0 &&
                (*sell_order_book)[j].quantity > 0) {
                // Check if price level already exists
                if ((*sell_order_book)[j].price != last_sell_price) {
                    num_sell_levels++;
                    last_sell_price = (*sell_order_book)[j].price;
                }
            }
            products[i].sell_levels = num_sell_levels;      
        }
        printf("%s\tProduct: %s; Buy levels: %d; Sell levels: %d\n", LOG_PREFIX, products[i].name, products[i].buy_levels, products[i].sell_levels);
        // Create combined order book
        // Create a combined order book
        int total_orders = *num_buy_orders + *num_sell_orders;
        Order *combined_order_book = malloc((total_orders) * sizeof(Order));
        if (combined_order_book == NULL) {
            perror("Error allocating memory");
            return 1;
        }
        // Copy buy order book into combined order book
        memcpy(combined_order_book, *buy_order_book, *num_buy_orders * sizeof(Order));
        // Copy sell order book into combined order book
        memcpy(combined_order_book + *num_buy_orders, *sell_order_book, *num_sell_orders * sizeof(Order));
        // Sort combined order book
        qsort(combined_order_book, total_orders, sizeof(Order), compare_orders);
        // Print orders
        int reporting_order_book_size = 0; // number of sell + buy price levels
        ReportingOB *reporting_order_book = malloc(sizeof(ReportingOB)); // 
        char type[5];
        int quantity = -1; // Total quantity at the price level
        int price = -1; // Price 
        int found_price_index = -1; // Index of the price level in the reporting order book
        int found_price = 0; // 1 if price level already exists, 0 if price level does not exist
        // Loop through combined order book
        for (int j = 0; j < total_orders; j++) {
            // Reset found price
            found_price = 0;
            // Assign variables
            strcpy(type, combined_order_book[j].type);
            quantity = combined_order_book[j].quantity;
            price = combined_order_book[j].price;

            // Only do something if product matches and if quantity is > 0
            if (strcmp(products[i].name, combined_order_book[j].product) == 0 && quantity > 0) {
                // Loop through reporting order book
                for (int k = 0; k < reporting_order_book_size; k++) {
                    // Check if price level of type exists
                    if (price == reporting_order_book[k].price_level &&
                        strncmp(type, reporting_order_book[k].type, 4) == 0) {
                        // If price level already exists, increment quantity and order count
                        found_price = 1;
                        found_price_index = k;
                        break;
                    }
                }
                // If price exists
                if (found_price == 1) {
                    // Add quantity to existing quantity and increment order count
                    reporting_order_book[found_price_index].quantity += quantity;
                    reporting_order_book[found_price_index].order_count++;
                } else { // Price doesn't exist
                    // Add new price level
                    reporting_order_book = realloc(reporting_order_book, (reporting_order_book_size + 1) * sizeof(ReportingOB));
                    if (reporting_order_book == NULL) {
                        perror("Error allocating memory");
                        return 1;
                    }
                    // Assigning values to new price level
                    reporting_order_book[reporting_order_book_size].price_level = price;
                    reporting_order_book[reporting_order_book_size].quantity = quantity;
                    reporting_order_book[reporting_order_book_size].order_count = 1;
                    strcpy(reporting_order_book[reporting_order_book_size].type, type);
                    
                    reporting_order_book_size++;
                }
            }
            found_price = 0;
        }
        // Print reporting order book
        for (int j = 0; j < reporting_order_book_size; j++) {
            printf("%s\t\t%s %d @ $%d (%d %s)\n", LOG_PREFIX, reporting_order_book[j].type, reporting_order_book[j].quantity, reporting_order_book[j].price_level, reporting_order_book[j].order_count, reporting_order_book[j].order_count > 1 ? "orders" : "order");
        }
        free(reporting_order_book);
        free(combined_order_book);
    }
    return 0;
}

int positions(TraderInfo *ti, int *num_traders, Product *products, int *num_products) {
    sigprocmask(SIG_BLOCK, &signal_mask, NULL);
    printf("%s\t--POSITIONS--\n", LOG_PREFIX);
    sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
    for (int i = 0; i < *num_traders; i++) {
        printf("%s\tTrader %d:", LOG_PREFIX, ti[i].trader_id);
        for (int j = 0; j < *num_products; j++) {
            printf(" %s %ld ($%ld)", products[j].name, ti[i].product_quantity[j], ti[i].product_profit[j]);
            if (j != *num_products - 1) {
                printf(",");
            }
        }
        printf("\n");
    }
    return 0;

}