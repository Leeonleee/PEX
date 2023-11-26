#define _DEFAULT_SOURCE
#define _GNU_SOURCE

#include "pe_trader.h"


int sig_flag;
sigset_t signal_mask;
void handle_sigusr1(int sig) {
    sigprocmask(SIG_BLOCK, &signal_mask, NULL);
    // critical section of code
    sig_flag = 1;
    // unblock signal
    sigprocmask(SIG_UNBLOCK, &signal_mask, NULL);
}


int is_alphanumeric(char* str) {
    int i = 0;
    while (str[i]) {
        if (!isalnum(str[i])) {
            return 0; // non alphanumeric character found
        }
        i++;
    }
    return 1; // all characters are alphanumeric
}

void cleanup(char *exchange_pipe_name, char *trader_pipe_name, int exchange_fd, int trader_fd, char *buffer) {
    if (exchange_pipe_name != NULL) {
        unlink(exchange_pipe_name);
    }
    if (trader_pipe_name != NULL) {
        unlink(trader_pipe_name);
    }
    if (exchange_fd != -1) {
        close(exchange_fd);
        free(exchange_pipe_name);
    }
    if (trader_fd != -1) {
        close(trader_fd);
        free(trader_pipe_name);
    }
    if (buffer != NULL) {
        free(buffer);
    }
}

int read_fifo(int exchange_fd, char** buffer_ptr) {
    int i = 0;
    char c;
    char* buffer = *buffer_ptr;
    while (read(exchange_fd, &c, 1) == 1){
        if (c == ';') {
            break; // found delimiter
        }
        buffer = realloc(buffer, (i + 2) * sizeof(char));
        if (buffer == NULL) {
            perror("Trader: Failed to reallocate memory");
            return 1;
        }
        buffer[i] = c; // add character to buffer
        i++;
    }
    buffer[i] = '\0'; // null terminate string
    *buffer_ptr = buffer; // update pointer
    return 0;
}

int write_message(int trader_fd, char* command, int* order_id, char* item, int quantity, int price) {
    // check if command is SELL
    if (strcmp(command, "SELL") == 0) {
        char *message;
        if (asprintf(&message, "BUY %d %s %d %d;", *order_id, item, quantity, price) == -1) {
            perror("Trader: Failed to allocate memory for message");
            return 1;
        }
         // write message to trader pipe
        if (write(trader_fd, message, strlen(message)) == -1 ) {
            perror("Trader: Failed to write to trader pipe");
            free(message);
            return 1;
        }
        free(message);
    }
    kill(getppid(), SIGUSR1); // send signal to exchange
    (*order_id)++; // increment order id
    return 0;
}

int parse_message(int trader_fd, char* buffer, int* order_id) {

    // parse command
    char *command = strtok(buffer + strlen("MARKET"), " ");
    // if command is SELL
    if (strcmp(command, "SELL") == 0) {
        // parse other arguments
        char *item = strtok(NULL, " ");
        char *quantity = strtok(NULL, " ");
        char *price = strtok(NULL, " ");
        // convert arguments to integers
        int quantity_int = atoi(quantity);
        int price_int = atoi(price);



        // check if item is alphanumeric and up to 16 characters
        if (strlen(item) > 16 || !is_alphanumeric(item)) {
            perror("Trader: Invalid item name");
            return 1;
        }

        // check if quantity, price and order id are over limits
        if (quantity_int < 1 || quantity_int > 999999) {
            perror("Trader: Invalid quantity");
            return 1;
        }
        if (price_int < 1 || price_int > 999999) {
            perror("Trader: Invalid price");
            return 1;
        }
        if (*order_id < 0 || *order_id > 999999) {
            perror("Trader: Invalid order id");
            return 1;
        }

        // check if quantity is over 999 and gracefully terminate
        if (quantity_int > 999) {
            return 2;
        }

        // call write message function
        write_message(trader_fd, command, order_id, item, quantity_int, price_int);
    }
    return 0;
}

int main(int argc, char ** argv) {
	printf("Trader: starting\n");
    // checking for command line arguments
    if (argc < 2) {
        printf("Not enough arguments\n");
        return 1;
    }
    if (argc > 2) {
        printf("Too many arguments\n");
        return 1;
    }

    // define FIFOs for cleanup function
    char *exchange_pipe_name = NULL;
    char *trader_pipe_name = NULL;
    int exchange_fd = -1;
    int trader_fd = -1;

    // connect to named pipes
    exchange_pipe_name = malloc(sizeof(char) * (strlen("/tmp/pe_exchange_") + strlen(argv[1]) + 1));
    sprintf(exchange_pipe_name, "/tmp/pe_exchange_%s", argv[1]);
    exchange_fd = open(exchange_pipe_name, O_RDONLY);

    if (exchange_fd == -1) {
        perror("Trader: Failed to open exchange pipe: trader");
        cleanup(exchange_pipe_name, trader_pipe_name, exchange_fd, trader_fd, NULL);
        return 1;
    }

    trader_pipe_name = malloc(sizeof(char) * (strlen("/tmp/pe_trader_") + strlen(argv[1]) + 1));
    sprintf(trader_pipe_name, "/tmp/pe_trader_%s", argv[1]);
    trader_fd = open(trader_pipe_name, O_WRONLY);

    if (trader_fd == -1) {
        perror("Trader: Failed to open trader pipe");
        cleanup(exchange_pipe_name, trader_pipe_name, exchange_fd, trader_fd, NULL);
        return 1;
    }
    // register signal handler

    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = handle_sigusr1;
    sa.sa_flags = SA_RESTART; // TODO: might be optional, idk
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("Trader: Failed to register signal handler");
        cleanup(exchange_pipe_name, trader_pipe_name, exchange_fd, trader_fd, NULL);
        return 1;
    }



    // sigqueue stuff
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGUSR1);

    // creating variables for event loop
    sig_flag = 0;
    int order_id = 0;
    int market_open = 0; // 0 if MARKET OPEN not received, 1 if received

    char *buffer = malloc(sizeof(char)); // buffer for reading messages
    if (buffer == NULL) {
        perror("Trader: Failed to allocate memory for buffer");
        cleanup(exchange_pipe_name, trader_pipe_name, exchange_fd, trader_fd, NULL);
        return 1;
    }
    

    // event loop
    while (1) { // infinite loop, only terminates when quantity > 999
        // Check if parent has exit
        if (getppid() == 1) {
            break;
        }
        pause();

        if (sig_flag) { // runs if signal received is SIGUSR1
            if (!market_open) { // MARKET OPEN not received
                if (read_fifo(exchange_fd, &buffer) == 1) {
                    perror("Trader: Failed to read from exchange pipe");
                    cleanup(exchange_pipe_name, trader_pipe_name, exchange_fd, trader_fd, buffer);
                    return 1;
                } else {
                    if (strcmp(buffer, "MARKET OPEN") == 0) {
                        printf("Trader: Market is open\n");
                        market_open = 1;
                    }
                }
            } else { // market is open
                if (read_fifo(exchange_fd, &buffer) == 1) {
                    perror("Trader: Failed to read from exchange pipe");
                    cleanup(exchange_pipe_name, trader_pipe_name, exchange_fd, trader_fd, buffer);
                    return 1;
                }
                // check first work of message
                if (strncmp(buffer, "MARKET", strlen("MARKET")) == 0) {
                    int parse_result = parse_message(trader_fd, buffer, &order_id);
                    if (parse_result == 1) {
                        perror("Trader: Failed to parse message");
                        cleanup(exchange_pipe_name, trader_pipe_name, exchange_fd, trader_fd, buffer);
                        return 1;
                    } else if (parse_result == 2) {
                        cleanup(exchange_pipe_name, trader_pipe_name, exchange_fd, trader_fd, buffer);
                        return 0;
                    }
                }      
            }
        }
        sig_flag = 0;
        
        
    }
    
    cleanup(exchange_pipe_name, trader_pipe_name, exchange_fd, trader_fd, buffer);

    return 0;
}