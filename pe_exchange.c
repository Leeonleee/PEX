#include "pe_exchange.h"
#include "read_product_file.h"
#include "init_traders.h"
#include "send_market_open.h"
#include "run_exchange.h"
#include "read_fifo.h"

// TODO: use macros instead of numbers for arrays

// Global variables
pid_t sender_pid; // Stores the PID of the trader that sent the signal
pid_t disconnected_pid;
int sig_flag; // Flag to indicate if SIGUSR1 was received
int disconnected_flag; // Flag to indicate if SIGCHLD was received
int num_active_traders;

sigset_t signal_mask;


void handle_sigusr1(int sig, siginfo_t *siginfo, void *context) {
	sender_pid = siginfo->si_pid;
	sig_flag = 1;
}

void handle_sigchld(int sig, siginfo_t *siginfo, void *context) {
	disconnected_pid = siginfo->si_pid; // PID of child process
	disconnected_flag = 1;
}

int main(int argc, char **argv) {
	if (argc < 3) {
		perror("Not enough arguments");
		return 1;
	}
	printf("%s Starting\n", LOG_PREFIX);

	// Register signal handler
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = handle_sigusr1;
	sa.sa_flags = SA_RESTART | SA_SIGINFO;
	if (sigaction(SIGUSR1, &sa, NULL) == -1) {
		perror("Error registering signal handler");
		return 1;
	}

	// Register sigchld handler
	struct sigaction sa_chld;
	memset(&sa_chld, 0, sizeof(sa_chld));
	sa_chld.sa_sigaction = handle_sigchld;
	sa_chld.sa_flags = SA_RESTART | SA_SIGINFO;
	sigemptyset(&sa_chld.sa_mask);
	if (sigaction(SIGCHLD, &sa_chld, NULL) == -1) {
		perror("Error registering sigchld handler");
		return 1;
	}

	// Setting up sigprocmask
	sigemptyset(&signal_mask);
	sigaddset(&signal_mask, SIGUSR1);
	// Initialise globals
	sig_flag = 0;
	sender_pid = -1;
	disconnected_flag = 0;
	disconnected_pid = -1;
	num_active_traders = -1;

	// Read product file
	char *product_file_name = argv[1];
	int num_products = 0;
	Product *products = NULL;
	if (read_product_file(product_file_name, &num_products, &products) == 1) {
		perror("Error reading product file");
		return 1;
	}

	// Declare and initialise pipe variables
	char *exchange_pipe_name = NULL;
	char *trader_pipe_name = NULL;
	int exchange_fd = -1;
	int trader_fd = -1;
	pid_t pid = -1;

	// Initialise traders
	int num_traders = argc - 2;
	// int trader_id = 0;
	TraderInfo *trader_info = calloc(num_traders, sizeof(TraderInfo)); // Array storing each trader's info
	if (init_traders(argv, trader_info, &pid, exchange_pipe_name, trader_pipe_name, exchange_fd, trader_fd, num_traders, num_products) == 1) {
		perror("Error initialising traders");
		return 1;
	}
	num_active_traders = num_traders;
	// Send MARKET OPEN;
	if (send_market_open(trader_info, num_traders) == 1) {
		perror("Error sending MARKET OPEN");
		return 1;
	}

	// Create order book;
	int order_time = 0; // lower number = older order
	int num_orders = 0;
	int num_buy_orders = 0;
	int num_sell_orders = 0;
	Order *buy_order_book = malloc(sizeof(Order)); // Order book that stores BUY orders
	Order *sell_order_book = malloc(sizeof(Order)); // Order book that stores SELL orders
	// Create exchange balance
	long exchange_balance = 0; // Stores the total fees collected by the exchange
	// Run program loop
	if (run_exchange(&products[0], &num_products, &trader_info[0], &num_traders, exchange_fd, trader_fd, &buy_order_book, &sell_order_book, &num_orders, &num_buy_orders, &num_sell_orders, &exchange_balance, &order_time) == 1) {
		perror("Error running exchange");
		return 1;
	}

	// Cleanup
	// Cleaning stuff from reading product file
	for (int i = 0; i < num_products; i++) {
        free(products[i].name);
    }
    free(products);
	// Cleaning order book
	free(buy_order_book);
	free(sell_order_book);
	// Closing and unlinking FIFOs, and freeing trader info
	for (int i = 0; i < num_traders; i++) {
		
		close(trader_info[i].exchange_fd);
		close(trader_info[i].trader_fd);
		// Get FIFO names
		exchange_pipe_name = malloc(sizeof(char) * strlen("/tmp/pe_exchange_") + 11);
		sprintf(exchange_pipe_name, FIFO_EXCHANGE, i);
		trader_pipe_name = malloc(sizeof(char) * strlen("/tmp/pe_trader_") + 11);
		sprintf(trader_pipe_name, FIFO_TRADER, i);
		// Unlink FIFOs
		unlink(exchange_pipe_name);
		unlink(trader_pipe_name);
		free(exchange_pipe_name);
		free(trader_pipe_name);
	}
	for (int i = 0; i < num_traders; i++) {
		free(trader_info[i].product_quantity);
	}
	for (int i = 0; i < num_traders; i++) {
		free(trader_info[i].product_profit);
	}
	free(trader_info);

	
}