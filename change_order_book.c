#include "change_order_book.h"
#include <signal.h> // had to add this line because of implicit declaration error

int matching_index;

int sort_buy_order(const void *a, const void *b) {
    // if price 1 < price 2 return 1
    // if price == and time 1 > time 2 return 1
    // if price 1 > price 2 return -1
    // else return -1
    Order *order_a = (Order *)a;
    Order *order_b = (Order *)b;
    if (order_a->price < order_b->price) {
        return 1;
    }
    else if (order_a->price == order_b->price) {
        if (order_a->time > order_b->time) {
            return 1;
        }
        else {
            return -1;
        }
    }
    else {
        return -1;
    }
}

int sort_sell_order(const void *a, const void *b) {
    Order *order_a = (Order *)a;
    Order *order_b = (Order *)b;
    if (order_a->price > order_b->price) {
        return 1;
    }
    else if (order_a->price == order_b->price) {
        if (order_a->time > order_b->time) {
            return 1;
        }
        else {
            return -1;
        }
    }
    else {
        return -1;
    }
}

int match_orders(TraderInfo *ti, Order **buy_order_book, Order **sell_order_book, int *num_buy_orders, int *num_sell_orders, char *latest_order_type, long *exchange_balance, Product *products, int *num_products) {
    int qty_matched;
    // Check the type of the latest order
    if (strcmp(latest_order_type, "BUY") == 0) {
        if (*num_sell_orders == 0) {
            return 0;
        }
        // Loop through each SELL order
        for (int i = 0; i < *num_sell_orders; i++) {
            // Check if product matches
            if (strcmp((*sell_order_book)[i].product, (*buy_order_book)[matching_index].product) == 0){
                // Exclude ones with quantity = 0
                if ((*sell_order_book)[i].quantity > 0) {
                    // Match the lowest price SELL with price <= BUY price
                    if ((*sell_order_book)[i].price <= (*buy_order_book)[matching_index].price) {
                        // Check quantities
                        if ((*sell_order_book)[i].quantity <= (*buy_order_book)[matching_index].quantity) {
                            qty_matched = (*sell_order_book)[i].quantity;
                            // if (qty_matched == 0) {
                            //     return 1;
                            // }
                            (*buy_order_book)[matching_index].quantity -= (*sell_order_book)[i].quantity;
                            (*sell_order_book)[i].quantity = 0;
                        } else {
                            qty_matched = (*buy_order_book)[matching_index].quantity;
                            // if (qty_matched == 0) {
                            //     return 1;
                            // }
                            (*sell_order_book)[i].quantity -= (*buy_order_book)[matching_index].quantity;
                            (*buy_order_book)[matching_index].quantity = 0;
                        }
                        long value = qty_matched * (*sell_order_book)[i].price;
                        // Fee = 1% of value rounded to the nearest dollar
                        long fee = round(value * 0.01);
                        *exchange_balance += fee;
                        // Logging
                        printf("%s Match: Order %d [T%d], New Order %d [T%d], value: $%ld, fee: $%ld.\n",LOG_PREFIX, (*sell_order_book)[i].order_id, (*sell_order_book)[i].trader_id, (*buy_order_book)[matching_index].order_id, (*buy_order_book)[matching_index].trader_id, value, fee);
                        // Update the positions
                        // Get index of product
                        int product_index = -1;
                        for (int j = 0; j < *num_products; j++) {
                            if (strcmp((*sell_order_book)[i].product, products[j].name) == 0) {
                                product_index = j;
                                break;
                            }
                        }
                        // Update buyer's quantity
                        ti[(*buy_order_book)[matching_index].trader_id].product_quantity[product_index] += qty_matched;
                        // Update buyer's profit
                        ti[(*buy_order_book)[matching_index].trader_id].product_profit[product_index] -= qty_matched * (*sell_order_book)[i].price;
                        // Subtract fee from buyer's profit
                        // Subtract fee from newest order
                        if ((*buy_order_book)[matching_index].time > (*sell_order_book)[i].time) {
                            ti[(*buy_order_book)[matching_index].trader_id].product_profit[product_index] -= fee;
                        } else {
                            ti[(*sell_order_book)[i].trader_id].product_profit[product_index] -= fee;
                        }
                        
                        // ti[(*buy_order_book)[*num_buy_orders - 1].trader_id].product_profit[product_index] -= fee;
                        // Update the seller's quantity
                        ti[(*sell_order_book)[i].trader_id].product_quantity[product_index] -= qty_matched;
                        // Update the seller's profit
                        ti[(*sell_order_book)[i].trader_id].product_profit[product_index] += qty_matched * (*sell_order_book)[i].price;
                        // Sending FILL
                        
                        // Only write to connected traders
                        if (ti[(*buy_order_book)[matching_index].trader_id].connected == 1) {
                            // Send to BUY trader first
                            char *fill_message = malloc(strlen("FILL") + 6 + 6 + 4); // 6 for ORDER_ID, 6 for QTY, 4 for spaces, ;, newline 
                            sprintf(fill_message, "FILL %d %d;", (*buy_order_book)[matching_index].order_id, qty_matched);
                            if (write(ti[(*buy_order_book)[matching_index].trader_id].exchange_fd, fill_message, strlen(fill_message)) == -1) {
                                perror("Error writing to exchange FIFO");
                                return 1;
                            }
                            if (kill(ti[(*buy_order_book)[matching_index].trader_id].pid, SIGUSR1) == -1) {
                                perror("Error sending SIGUSR1 to trader");
                                return 1;
                            }
                            free(fill_message);
                        }
                        if (ti[(*sell_order_book)[i].trader_id].connected == 1) {
                            // Send to SELL trader
                            char *fill_message = malloc(strlen("FILL") + 6 + 6 + 4); // 6 for ORDER_ID, 6 for QTY, 4 for spaces, ;, newline 
                            sprintf(fill_message, "FILL %d %d;", (*sell_order_book)[i].order_id, qty_matched);
                            if (write(ti[(*sell_order_book)[i].trader_id].exchange_fd, fill_message, strlen(fill_message)) == -1) {
                                perror("Error writing to exchange FIFO");
                                return 1;
                            }
                            if (kill(ti[(*sell_order_book)[i].trader_id].pid, SIGUSR1) == -1) {
                                perror("Error sending SIGUSR1 to trader");
                                return 1;
                            }
                            free(fill_message);

                        }
                                         

                    }
                }
            }
        }
    }
    if (strcmp(latest_order_type, "SELL") == 0) {
        if (*num_buy_orders == 0) {
            return 0;
        }
        // Loop through each BUY order
        for (int i = 0; i < *num_buy_orders; i++) {
            // Check if product matches
            if (strcmp((*buy_order_book)[i].product, (*sell_order_book)[matching_index].product) == 0) {
                // Exclude ones with quantity = 0
                if ((*buy_order_book)[i].quantity > 0) {
                    // Match the highest price BUY with price >= SELL price
                    if ((*buy_order_book)[i].price >= (*sell_order_book)[matching_index].price) {
                        // Check quantities
                        if ((*buy_order_book)[i].quantity <= (*sell_order_book)[matching_index].quantity) {
                            qty_matched = (*buy_order_book)[i].quantity;
                            // if (qty_matched == 0) {
                            //     return 1;
                            // }
                            (*sell_order_book)[matching_index].quantity -= (*buy_order_book)[i].quantity;
                            (*buy_order_book)[i].quantity = 0;
                        } else {
                            qty_matched = (*sell_order_book)[matching_index].quantity;
                            // if (qty_matched == 0) {
                            //     return 1;
                            // }
                            (*buy_order_book)[i].quantity -= (*sell_order_book)[matching_index].quantity;
                            (*sell_order_book)[matching_index].quantity = 0;
                        }
                        long value = qty_matched * (*buy_order_book)[i].price;
                        // Fee = 1% of value rounded to the nearest dollar
                        long fee = round(value * 0.01);
                        *exchange_balance += fee;
                        // Logging
                        printf("%s Match: Order %d [T%d], New Order %d [T%d], value: $%ld, fee: $%ld.\n",LOG_PREFIX, (*buy_order_book)[i].order_id, (*buy_order_book)[i].trader_id, (*sell_order_book)[matching_index].order_id, (*sell_order_book)[matching_index].trader_id, value, fee);
                        
                        // Update the positions
                        // Get index of product
                        int product_index = -1;
                        for (int j = 0; j < *num_products; j++) {
                            if (strcmp((*buy_order_book)[i].product, products[j].name) == 0) {
                                product_index = j;
                                break;
                            }
                        }
                        // Update buyer's quantity
                        ti[(*buy_order_book)[i].trader_id].product_quantity[product_index] += qty_matched;
                        // Update buyer's profit
                        ti[(*buy_order_book)[i].trader_id].product_profit[product_index] -= qty_matched * (*buy_order_book)[i].price;
                        // Update the seller's quantity
                        ti[(*sell_order_book)[matching_index].trader_id].product_quantity[product_index] -= qty_matched;
                        // Update the seller's profit
                        // printf("Seller profit: %d x $%d = %d\n", qty_matched, (*buy_order_book)[i].price , qty_matched * (*buy_order_book)[i].price);
                        ti[(*sell_order_book)[matching_index].trader_id].product_profit[product_index] += qty_matched * (*buy_order_book)[i].price;
                        // Subtract fee from seller's profit
                        // Subtract fee from newer order
                        if ((*sell_order_book)[matching_index].time > (*buy_order_book)[i].time) {
                            ti[(*sell_order_book)[matching_index].trader_id].product_profit[product_index] -= fee;
                        } else {
                            ti[(*buy_order_book)[i].trader_id].product_profit[product_index] -= fee;
                        }
                        // ti[(*sell_order_book)[*num_sell_orders - 1].trader_id].product_profit[product_index] -= fee;
                        // Sending FILL
                        char *fill_message = malloc(strlen("FILL") + 6 + 6 + 4); // 6 for ORDER_ID, 6 for QTY, 4 for spaces, ;, newline
                        // Only write to connected traders
                        if (ti[(*buy_order_book)[i].trader_id].connected == 1) {
                            // Send to BUY trader first
                            sprintf(fill_message, "FILL %d %d;", (*buy_order_book)[i].order_id, qty_matched);
                            if (write(ti[(*buy_order_book)[i].trader_id].exchange_fd, fill_message, strlen(fill_message)) == -1) {
                                perror("Error writing to exchange FIFO");
                                return 1;
                            }
                            if (kill(ti[(*buy_order_book)[i].trader_id].pid, SIGUSR1) == -1) {
                                perror("Error sending SIGUSR1 to trader");
                                return 1;
                            }
                        }
                        if (ti[(*sell_order_book)[matching_index].trader_id].connected == 1) {
                            // Send to SELL trader
                            sprintf(fill_message, "FILL %d %d;", (*sell_order_book)[matching_index].order_id, qty_matched);
                            if (write(ti[(*sell_order_book)[matching_index].trader_id].exchange_fd, fill_message, strlen(fill_message)) == -1) {
                                perror("Error writing to exchange FIFO");
                                return 1;
                            }
                            if (kill(ti[(*sell_order_book)[matching_index].trader_id].pid, SIGUSR1) == -1) {
                                perror("Error sending SIGUSR1 to trader");
                                return 1;
                            }
                        }
                        free(fill_message);   
                        // Check if sell quantity is 0
                        if ((*sell_order_book)[matching_index].quantity == 0) {
                            break;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

int add_order(Product *products, int *num_products, TraderInfo* ti, int trader_id, Order **buy_order_book, Order **sell_order_book, int *num_orders_ptr, int* num_buy_orders, int* num_sell_orders, char *command, int order_id, char *product, int quantity, int price, int *num_traders, long *exchange_balance, int *order_time) {

    // Create new order
    Order new_order = {0};
    strcpy(new_order.type, command);
    new_order.order_id = order_id;
    strcpy(new_order.product, product);
    new_order.quantity = quantity;
    new_order.price = price;
    new_order.trader_id = ti[trader_id].trader_id;
    new_order.time = *order_time;
    (*order_time)++;
    // Check if order is BUY or SELL
    if (strcmp(command, "BUY") == 0) {
        *buy_order_book = realloc(*buy_order_book, (*num_buy_orders + 1) * sizeof(Order));
        // Resizing order_book
        if (*buy_order_book == NULL) {
            printf("Error reallocating order book\n");
        }
        // Add new order to order_book array
        (*buy_order_book)[*num_buy_orders] = new_order; 
        // Increment buy_orders
        (*num_buy_orders)++;
        matching_index = *num_buy_orders - 1;
    }
    if (strcmp(command, "SELL") == 0) {
        *sell_order_book = realloc(*sell_order_book, (*num_sell_orders + 1) * sizeof(Order));
        // Resizing order_book
        if (*sell_order_book == NULL) {
            printf("Error reallocating order book\n");
        }
        // Add new order to order_book array
        (*sell_order_book)[*num_sell_orders] = new_order; 
        // Increment sell_orders
        (*num_sell_orders)++;
        matching_index = *num_sell_orders - 1;
    }
    if (strcmp(command, "BUY") == 0) {
        // Match orders
        if (match_orders(ti, buy_order_book, sell_order_book, num_buy_orders, num_sell_orders, command, exchange_balance, products, num_products) == 1) {
            return 1;
        }
        // Sort array
        qsort(*buy_order_book, *num_buy_orders, sizeof(Order), sort_buy_order);
    }
    if (strcmp(command, "SELL") == 0) {
        // Match orders
        if (match_orders(ti, buy_order_book, sell_order_book, num_buy_orders, num_sell_orders, command, exchange_balance, products, num_products) == 1) {
            return 1;
        }
        // Sort array
        qsort(*sell_order_book, *num_sell_orders, sizeof(Order), sort_sell_order);
    }
    
    
    (*num_orders_ptr)++; // Increment num_orders

    // Reporting
    order_books(products, num_products, buy_order_book, num_buy_orders, sell_order_book, num_sell_orders);
    positions(ti, num_traders, products, num_products);
    // Increment last_order_id by 1
    ti[trader_id].last_order_id++;    
    return 0;
}

int amend_orders(Product* products, int* num_products, TraderInfo* ti, int trader_id, Order **buy_order_book, Order **sell_order_book, int* num_buy_orders, int* num_sell_orders, char *command, int order_id, char *product, int quantity, int price, int *num_traders, long *exchange_balance, int *order_time){
    char type[5] = ""; // Stores if the order being amended is BUY or SELL
    char temp_product[16] = "";
    int temp_quantity;
    int temp_price;
    // Find order
    int order_found = 0; // 0 = not found, 1 = found
    for (int i = 0; i < *num_buy_orders; i++) {
        if ((order_id == (*buy_order_book)[i].order_id && trader_id == (*buy_order_book)[i].trader_id && (*buy_order_book)[i].quantity > 0)) {
            order_found = 1;
            // Amend values
            (*buy_order_book)[i].quantity = quantity;
            (*buy_order_book)[i].price = price;
            (*buy_order_book)[i].time = *order_time;
            (*order_time)++;
            // Assign variables
            strcpy(type, "BUY");
            strcpy(temp_product, (*buy_order_book)[i].product);
            temp_quantity = (*buy_order_book)[i].quantity;
            temp_price = (*buy_order_book)[i].price;
            matching_index = i;
            break;
        }
    }
    if (order_found == 0) {
        for (int i = 0; i < *num_sell_orders; i++) {
            if ((order_id == (*sell_order_book)[i].order_id && trader_id == (*sell_order_book)[i].trader_id && (*sell_order_book)[i].quantity > 0)) {
                order_found = 1;
                // Amend values
                (*sell_order_book)[i].quantity = quantity;
                (*sell_order_book)[i].price = price;
                (*sell_order_book)[i].time = *order_time;
                (*order_time)++;
                // Assign type
                strcpy(type, "SELL");
                strcpy(temp_product, (*sell_order_book)[i].product);
                temp_quantity = (*sell_order_book)[i].quantity;
                temp_price = (*sell_order_book)[i].price;
                matching_index = i;
                break;
            }
        }
    }
    if (order_found == 0) {
        return 1;
    }
    // Only write to connected trader
    if (ti[trader_id].connected == 1) {
        // Write AMENDED to trader
        char *amended_message = malloc(strlen("AMENDED") + 6 + 2); // 6 for ORDER_ID, 1 for ; and 1 for space
        sprintf(amended_message, "AMENDED %d;", order_id);
        if (write(ti[trader_id].exchange_fd, amended_message, strlen(amended_message)) == -1) {
            perror("Error writing to exchange FIFO");
            return 1;
        }
        // Send SIGUSR1 to trader
        if (kill(ti[trader_id].pid, SIGUSR1) == -1) {
            perror("Error sending SIGUSR1 to trader");
            return 1;
        }
        free(amended_message);  
    }
    // Send MARKET to all other traders
    for (int i = 0; i < *num_traders; i++) {
        if (i != trader_id && ti[i].connected == 1) {
            char *accepted_message = malloc(strlen("MARKET") + strlen(type) + strlen(product) + 18); // 17 is for qty, price, spaces and ;
            sprintf(accepted_message, "MARKET %s %s %d %d;", type, temp_product, temp_quantity, temp_price);
            if (write(ti[i].exchange_fd, accepted_message, strlen(accepted_message)) == -1) {
                perror("Error writing to exchange FIFO");
                return 1;
            }
            if (kill(ti[i].pid, SIGUSR1) == -1) {
                perror("Error sending SIGUSR1 to trader");
                return 1;
            }
            free(accepted_message);
        }
    }
    // Match orders
    
    if (strcmp(type, "BUY") == 0) {
        
        // Match orders
        if (match_orders(ti, buy_order_book, sell_order_book, num_buy_orders, num_sell_orders, command, exchange_balance, products, num_products) == 1) {
            return 1;
        }
        // Sort array
        qsort(*buy_order_book, *num_buy_orders, sizeof(Order), sort_buy_order);
        
    }
    if (strcmp(type, "SELL") == 0) {
         
        // Match orders
        if (match_orders(ti, buy_order_book, sell_order_book, num_buy_orders, num_sell_orders, command, exchange_balance, products, num_products) == 1) {
            return 1;
        }
        // Sort array
        qsort(*sell_order_book, *num_sell_orders, sizeof(Order), sort_sell_order);
       
    }
    // Reporting
    order_books(products, num_products, buy_order_book, num_buy_orders, sell_order_book, num_sell_orders);
    positions(ti, num_traders, products, num_products);
    
    return 0;
}

int cancel_orders(Product* products, int* num_products, TraderInfo* ti, int trader_id, Order **buy_order_book, Order **sell_order_book, int* num_buy_orders, int* num_sell_orders, char *command, int order_id, char *product, int *num_traders, long *exchange_balance) {
    char type[5] = ""; // Stores if the order being amended is BUY or SELL
    char temp_product[17] = "";
    // int temp_quantity;
    // int temp_price;
     // Find order
    int order_found = 0; // 0 = not found, 1 = found
    for (int i = 0; i < *num_buy_orders; i++) {
        if ((order_id == (*buy_order_book)[i].order_id && trader_id == (*buy_order_book)[i].trader_id && (*buy_order_book)[i].quantity > 0)) {
            order_found = 1;
            // Make quantity = 0
            (*buy_order_book)[i].quantity = 0;
            (*buy_order_book)[i].price = 0;
            // Assign type
            strcpy(type, "BUY");
            strcpy(temp_product, (*buy_order_book)[i].product);
            matching_index = i;
            break;
        }
    }
        
    if (order_found == 0) {
        for (int i = 0; i < *num_sell_orders; i++) {
            if ((order_id == (*sell_order_book)[i].order_id && trader_id == (*sell_order_book)[i].trader_id && (*sell_order_book)[i].quantity > 0)) {
                order_found = 1;
                // Amend values
                (*sell_order_book)[i].quantity = 0;
                (*sell_order_book)[i].price = 0;
                // Assign type
                strcpy(type, "SELL");
                strcpy(temp_product, (*sell_order_book)[i].product);
                matching_index = i;
                break;
            }
        }
    }
    if (order_found == 0) {
        return 1;
    }
    // Only write to connected traders
    if (ti[trader_id].connected == 1) {
        // Write CANCELLED to trader
        char *cancelled_message = malloc(strlen("CANCELLED") + 6 + 2); // 6 for ORDER_ID, 1 for ; and 1 for space
        sprintf(cancelled_message, "CANCELLED %d;", order_id);
        if (write(ti[trader_id].exchange_fd, cancelled_message, strlen(cancelled_message)) == -1) {
            perror("Error writing to exchange FIFO");
            return 1;
        }
        // Send SIGUSR1 to trader
        if (kill(ti[trader_id].pid, SIGUSR1) == -1) {
            perror("Error sending SIGUSR1 to trader");
            return 1;
        }
        free(cancelled_message);
    }
     // Send MARKET to all other traders
    for (int i = 0; i < *num_traders; i++) {
        if (i != trader_id && ti[i].connected == 1) {
            char *accepted_message = malloc(strlen("MARKET") + strlen(type) + strlen(product) + 18); // 17 is for qty, price, spaces and ;
            sprintf(accepted_message, "MARKET %s %s %d %d;", type, temp_product, 0, 0);
            if (write(ti[i].exchange_fd, accepted_message, strlen(accepted_message)) == -1) {
                perror("Error writing to exchange FIFO");
                return 1;
            }
            if (kill(ti[i].pid, SIGUSR1) == -1) {
                perror("Error sending SIGUSR1 to trader");
                return 1;
            }
            free(accepted_message);
        }
    }
   
    // Match orders
    if (strcmp(type, "BUY") == 0) {
        
        // Match orders
        if (match_orders(ti, buy_order_book, sell_order_book, num_buy_orders, num_sell_orders, command, exchange_balance, products, num_products) == 1) {
            return 1;
        }
        // Sort array
        qsort(*buy_order_book, *num_buy_orders, sizeof(Order), sort_buy_order);
    }
    if (strcmp(type, "SELL") == 0) {
        
        // Match orders
        if (match_orders(ti, buy_order_book, sell_order_book, num_buy_orders, num_sell_orders, command, exchange_balance, products, num_products) == 1) {
            return 1;
        }
        // Sort array
        qsort(*sell_order_book, *num_sell_orders, sizeof(Order), sort_sell_order);
    }
    
    
    // Reporting
    order_books(products, num_products, buy_order_book, num_buy_orders, sell_order_book, num_sell_orders);
    positions(ti, num_traders, products, num_products);
    return 0;
}