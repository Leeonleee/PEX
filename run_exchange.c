#include "run_exchange.h"


int run_exchange(Product *products, int* num_products, TraderInfo* ti, int *num_traders, int exchange_fd, int trader_fd, Order **buy_order_book, Order **sell_order_book, int* num_orders_ptr, int* num_buy_orders, int* num_sell_orders, long *exchange_balance, int *order_time) {
    int trader_id = -1; // Index of trader in trader_info array

    while (1) {
        // TODO: check if a trader closes their end of the pipe
        // If a trader has disconnected
        if (disconnected_flag) {
            // Get the index of the trader that disconnected
            for (int i = 0; i < *num_traders; i++) {
                if (disconnected_pid == ti[i].pid) {
                    ti[i].connected = 0;
                    sigprocmask(SIG_BLOCK, &signal_mask, NULL);
                    printf("%s Trader %d disconnected\n", LOG_PREFIX, ti[i].trader_id);
                    sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
                    num_active_traders--;
                    break;
                } 
            }
            disconnected_flag = 0; // Reset disconnected_flag
            if (num_active_traders == 0) {
                sigprocmask(SIG_BLOCK, &signal_mask, NULL);
                printf("%s Trading completed\n", LOG_PREFIX);
                sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
                sigprocmask(SIG_BLOCK, &signal_mask, NULL);
                printf("%s Exchange fees collected: $%ld\n", LOG_PREFIX, *exchange_balance);
                sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
                return 0;
            }
        }

        // Buffer to store reads
        char *buffer = malloc(sizeof(char));
        if ((buffer) == NULL) {
            perror("Error allocating memory for buffer");
            return 1;
        }
        // Wait for signal from trader
        pause();
        if (sig_flag) { // If SIGUSR1 was received
            // Get the index of the trader in the trader_info array
            for (int i = 0; i < *num_traders; i++) {
                if (sender_pid == ti[i].pid) {
                    trader_id = i;
                    break;
                }
            }
            if (trader_id == -1) {
                perror("Error finding trader in trader_info array");
                free(buffer);
                return 1;
            }
            // Read FIFO
            if (read_fifo(ti, trader_id, &buffer) == 1) {
                perror("Error reading from FIFO");
                free(buffer);
                return 1;
            }
            sigprocmask(SIG_BLOCK, &signal_mask, NULL);
            printf("%s [T%d] Parsing command: <%s>\n", LOG_PREFIX, ti[trader_id].trader_id, buffer);
            sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
            // Parse message
            if (parse_message(products, num_products, ti, num_traders, trader_id, buffer, buy_order_book, sell_order_book, num_orders_ptr, num_buy_orders, num_sell_orders, exchange_balance, order_time) == 1) {
                // Only write to trader if connected
                // perror("Sending invalid");
                if (ti[trader_id].connected == 1) {
                    // Send INVALID; and SIGUSR1
                    if (write(ti[trader_id].exchange_fd, "INVALID;", strlen("INVALID;")) == -1) {
                        perror("Error writing to exchange FIFO");
                        return 1;
                    }
                    if (kill(ti[trader_id].pid, SIGUSR1) == -1) {
                        perror("Error sending SIGUSR1 to trader");
                        return 1;
                    }
                }
                
            }
        }
        free(buffer);
        sig_flag = 0; // Resets sig_flag
    }
    return 0;
}