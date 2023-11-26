#include "send_market_open.h"

int send_market_open(TraderInfo* ti, int num_traders) {
    // Loop through each trader
    for (int i = 0; i < num_traders; i++) {
        if (write(ti[i].exchange_fd, "MARKET OPEN;", strlen("MARKET OPEN;")) == -1) {
            perror("Error writing to trader FIFO");
            return 1;
        }
        if (kill(ti[i].pid, SIGUSR1) == -1) {
            perror("Error sending SIGUSR1 to trader");
            return 1;
        }
    }
    return 0;
}