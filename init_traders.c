#include "init_traders.h"

int init_traders(char *argv[], TraderInfo* ti, pid_t *pid, char *exchange_pipe_name, char *trader_pipe_name, int exchange_fd, int trader_fd, int num_traders, int num_products) {
    for (int i = 0; i < num_traders; i++) {
        // Making FIFO names
        exchange_pipe_name = malloc(sizeof(char) * strlen("/tmp/pe_exchange_") + 11);
        trader_pipe_name = malloc(sizeof(char) * strlen("/tmp/pe_trader_") + 11);
        sprintf(exchange_pipe_name, FIFO_EXCHANGE, i);
        sprintf(trader_pipe_name, FIFO_TRADER, i);

        // Making the FIFOs
        if (mkfifo(exchange_pipe_name, 0666) == -1) {
            perror("Error making exchange FIFO");
            return 1;
        }
        if (mkfifo(trader_pipe_name, 0666) == -1) {
            perror("Error making trader FIFO");
            return 1;
        }
        printf("%s Created FIFO %s\n", LOG_PREFIX, exchange_pipe_name);
        printf("%s Created FIFO %s\n", LOG_PREFIX, trader_pipe_name);
        
        // Forking and starting traders
        (*pid) = fork();
        if (*pid == -1) {
            perror("Error forking");
            return 1;
        }
        if (*pid == 0) {
            // Child process
            sigprocmask(SIG_BLOCK, &signal_mask, NULL);
            printf("%s Starting trader %d (%s)\n", LOG_PREFIX, i, argv[i + 2]);
            sigprocmask(SIG_UNBLOCK, &signal_mask, NULL); 


            // Freeing
            // free(exchange_pipe_name);
            // free(trader_pipe_name);
            // free(ti[i].product_quantity);
            // free(ti[i].product_profit);
            // free(ti);

            char trader_id_str[MAX_INT]; // max length of an int + 1 for null terminator
            sprintf(trader_id_str, "%d", i); // convert trader id to string

            char *trader_name = basename(argv[i + 2]);
            execl(argv[i + 2], trader_name, trader_id_str, NULL);
            perror("Error starting trader binary");
            exit(1);
        } else {
            // Parent process

            // Assigning trader info except FD
            ti[i].pid = *pid;
            ti[i].trader_id = i;
            ti[i].product_quantity = calloc(num_products, sizeof(long));
            ti[i].product_profit = calloc(num_products, sizeof(long));
            ti[i].last_order_id = -1;
            ti[i].connected = 1;

            // Opening FIFOs
            exchange_fd = open(exchange_pipe_name, O_WRONLY);
            if ((exchange_fd) == -1) {
                perror("Error opening exchange FIFO");
                return 1;
            }
            trader_fd = open(trader_pipe_name, O_RDONLY);
            if ((trader_fd) == -1) {
                perror("Error opening trader FIFO");
                return 1;
            }

            sigprocmask(SIG_BLOCK, &signal_mask, NULL);
            printf("%s Connected to %s\n", LOG_PREFIX, exchange_pipe_name);
            sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
            sigprocmask(SIG_BLOCK, &signal_mask, NULL);
            printf("%s Connected to %s\n", LOG_PREFIX, trader_pipe_name);
            sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);

            // Assigning FD to trader info
            ti[i].exchange_fd = exchange_fd;
            ti[i].trader_fd = trader_fd;

        }
         
        // Freeing FIFO names
        free(exchange_pipe_name);
        free(trader_pipe_name);
    }
    return 0;
}