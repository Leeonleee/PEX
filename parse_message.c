#include "parse_message.h"

int is_alphanumeric(char* str) {
    int i = 0;
    while (str[i]) {
        if (!isalnum(str[i])) {
            return 0; // not alphanumeric
        }
        i++;
    }
    return 1; // all characters are alphanumeric
} 

int parse_message(Product *products, int *num_products, TraderInfo* ti, int *num_traders, int trader_id, char *buffer, Order **buy_order_book, Order **sell_order_book, int *num_orders_ptr, int* num_buy_orders, int* num_sell_orders, long *exchange_balance, int *order_time) {
    // Parsing command
    char *command = strtok(buffer, " ");
    // Check if command is missing
    if (command == NULL) {
        // perror("Error parsing command");
        return 1;
    }
    // Parsing order ID
    char *order_id = strtok(NULL, " ");
    // Check if order_id is missing
    if (order_id == NULL) {
        // perror("Error parsing order ID");
        return 1;
    }
    // Check order_id is between 0-999999
    if (atoi(order_id) < 0 || atoi(order_id) > 999999) {
        // perror("Order ID out of range");
        return 1;
    }

    // If command is BUY or SELL
    if (strcmp(command, "BUY") == 0 || strcmp(command, "SELL") == 0) {
        
        // Parse other arguments
        char *product = strtok(NULL, " ");
        char *quantity = strtok(NULL, " ");
        char *price = strtok(NULL, " ");

        // Check for too many arguments
        char *extra = strtok(NULL, " ");
        if (extra != NULL) {
            // perror("Too many arguments");
            return 1;
        }
        // Check for missing arguments
        if (product == NULL || quantity == NULL || price == NULL) {
            // perror("Missing arguments");
            return 1;
        }
        // Check of product is alphanumeric and up to 16 characters
        if (strlen(product) > 16 || !is_alphanumeric(product)) {
            // perror("Product name is not alphanumeric or is too long");
            return 1;
        }
        // Check if quantity and price are between 1-999999
        if (atoi(quantity) < 1 || atoi(quantity) > 999999 || atoi(price) < 1 || atoi(price) > 999999) {
            // perror("Quantity or price is out of range");
            return 1;
        }
        // Check if product is in the product array
        int product_found = 0;
        for (int i = 0; i < *num_products; i++) {
            if (strcmp(products[i].name, product) == 0) {
                product_found = 1;
                break;
            }
        }
        if (product_found == 0) {
            // perror("Product not found");
            return 1;
        }
        // Check if order_id is valid for the trader that sent the message
        if (atoi(order_id) != ti[trader_id].last_order_id + 1) {
            // perror("Order ID is not 1 higher than previous");
            return 1;
        }
        // Send ACCEPTED order to trader
        if (ti[trader_id].connected == 1) {
            char *accepted_message = malloc(sizeof(char) * 20);
            sprintf(accepted_message, "ACCEPTED %d;", atoi(order_id));
            if (write(ti[trader_id].exchange_fd, accepted_message, strlen(accepted_message)) == -1) {
                perror("Error writing to exchange FIFO");
                return 1;
            }
            // Send SIGUSR1 to trader
            if (kill(ti[trader_id].pid, SIGUSR1) == -1) {
                perror("Error sending SIGUSR1 to trader");
                return 1;
            }
            free(accepted_message);
        } else {
            return 0;
        }
        // Send MARKET to all other traders
        for (int i = 0; i < *num_traders; i++) {
            if (i != trader_id && ti[i].connected == 1) {
                char *accepted_message = malloc(strlen("MARKET") + strlen(command) + strlen(product) + 18);
                sprintf(accepted_message, "MARKET %s %s %d %d;", command, product, atoi(quantity), atoi(price));
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

        // Add order to order book
        if (add_order(products, num_products, ti, trader_id, buy_order_book, sell_order_book, num_orders_ptr, num_buy_orders, num_sell_orders, command, atoi(order_id), product, atoi(quantity), atoi(price), num_traders, exchange_balance, order_time) == 1) {
            // perror("Error add_order");
            return 1;
        }
    }
    else if (strcmp(command, "AMEND") == 0) {
        // Parsing other arguments
        char *quantity = strtok(NULL, " ");
        char *price = strtok(NULL, " ");
        // Check for too many arguments
        char *extra = strtok(NULL, " ");
        if (extra != NULL) {
            // perror("Too many arguments");
            return 1;
        }
        // Check for missing arguments
        if (quantity == NULL || price == NULL) {
            // perror("Missing arguments");
            return 1;
        }
        // Check if quantity and price are between 1-999999
        if (atoi(quantity) < 1 || atoi(quantity) > 999999 || atoi(price) < 1 || atoi(price) > 999999) {
            // perror("Quantity or price is out of range");
            return 1;
        }
        // Check if order ID from the trader is in the order book
        char product[16] = "";
        int order_found = 0; // 0 = not found, 1 = found
        for (int i = 0; i < *num_buy_orders; i++) {
            if ((atoi(order_id) == (*buy_order_book)[i].order_id && trader_id == (*buy_order_book)[i].trader_id)) {
                order_found = 1;
                // Assign product name to be passed to amend_orders
                strcpy(product, (*buy_order_book)[i].product);
                break;
            }
        }
        if (order_found == 0) {
            for (int i = 0; i < *num_sell_orders; i++) {
                if ((atoi(order_id) == (*sell_order_book)[i].order_id && trader_id == (*sell_order_book)[i].trader_id)) {
                    order_found = 1;
                    strcpy(product, (*sell_order_book)[i].product);
                    break;
                }
            }
        }
        if (order_found == 0) {
            // perror("Order to be amended not found");
            return 1;
        }
        // Amend order
        if (amend_orders(products, num_products, ti, trader_id, buy_order_book, sell_order_book, num_buy_orders, num_sell_orders, command, atoi(order_id), product, atoi(quantity), atoi(price), num_traders, exchange_balance, order_time) == 1) {
            // perror("Failed to amend_orders");
            return 1;
        }
    }
    else if (strcmp(command, "CANCEL") == 0) {
        // Check for too many arguments
        char *extra = strtok(NULL, " ");
        if (extra != NULL) {
            // perror("Too many arguments");
            return 1;
        }
        // Check if order ID from the trader is in the order book
        char product[16] = "";
        // int quantity = 0;
        // int price = -1; // Used to allow matching to work
        int order_found = 0; // 0 = not found, 1 = found
        for (int i = 0; i < *num_buy_orders; i++) {
            if ((atoi(order_id) == (*buy_order_book)[i].order_id && trader_id == (*buy_order_book)[i].trader_id)) {
                order_found = 1;
                // Assign product name to be passed to amend_orders
                strcpy(product, (*buy_order_book)[i].product);
                // price = (*buy_order_book)[i].price;
                break;
            }
        }
        if (order_found == 0) {
            for (int i = 0; i < *num_sell_orders; i++) {
                if ((atoi(order_id) == (*sell_order_book)[i].order_id && trader_id == (*sell_order_book)[i].trader_id)) {
                    order_found = 1;
                    strcpy(product, (*sell_order_book)[i].product);
                    // price = (*sell_order_book)[i].price;
                    break;
                }
            }
        }
        if (order_found == 0) {
            // perror("Order to be cancelled not found");
            return 1;
        }
        // Cancel order
        if (cancel_orders(products, num_products, ti, trader_id, buy_order_book, sell_order_book, num_buy_orders, num_sell_orders, command, atoi(order_id), product, num_traders, exchange_balance) == 1) {
            // perror("Failed to cancel_orders");
            return 1;
        }

    } else {
        // Only write to connected traders
        if (ti[trader_id].connected == 1) {
            // Write invalid
            if (write(ti[trader_id].exchange_fd, "INVALID;", strlen("INVALID;")) == -1) {
                perror("Error writing to exchange FIFO");
                return 1;
            }
            // Send SIGUSR1 to trader
            if (kill(ti[trader_id].pid, SIGUSR1) == -1) {
                perror("Error sending SIGUSR1 to trader");
                return 1;
            }
        }
        
    }

    return 0;
}