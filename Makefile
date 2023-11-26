CC=gcc
# CFLAGS=-Wall -Werror -Wextra -Wuninitialized -Wvla -O0 -std=c11 -g -fsanitize=address,leak, -O3 -march=native
CFLAGS=-Wall -Werror -Wvla -O0 -std=c11 -g -fsanitize=address,leak, -O3 -march=native
# CFLAGS=-Wall -Werror -Wvla -O0 -std=c11 -g -fsanitize=address,leak, -O3 
LDFLAGS=-lm
BINARIES=pe_exchange pe_trader 

all: $(BINARIES)

.PHONY: clean
# clean:
# 	rm -f $(BINARIES)


# --------------------------

CC=gcc
# CFLAGS=-Wall -Werror -Wvla -O0 -std=c11 -g -fsanitize=address,leak
LDFLAGS=-lm
SOURCE=$(wildcard *.c)
OBJ=$(patsubst %.c, %.o, $(filter-out pe_trader.c, $(SOURCE)))
BINARIES=pe_exchange

all: $(BINARIES)

$(BINARIES): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(BINARIES) $(OBJ)

# --------------------------

test:
	echo what are we testing?!

tests:
	bash run_tests.sh

run_tests:
	bash run_tests.sh