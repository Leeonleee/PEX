1. Describe how your exchange works.
The exchange will first check if there are enough comand line arguments.
After checking the about of arguents, it will register the signal handlers
It will then read the product file, and save the products to a name, and assign the first line number to num_traders
It will then initialise the traders. During initialisation, the exchange will loop num_traders times, and create 2 FIFOs. It will them fork, and the child will exec the trader binary, while the parent will open the FIFOs and save the new trader to an array
The main program loop will check for disconnections, then read from the FIFO of the trader that sent the signal. It will then parse the message, and determine which command to run. Everything except INVALID and CANCEL will cause match orders and the reporting to happen. Once all traders disconnect, it will free all of the remaining memory

2. Describe your design decisions for the trader and how it's fault-tolerant.
The trader uses pause, so it'll wait for a signal before it does anything. The trader also checks every loop whether the exchange has exited, and if it has then it will cleanup all the memory/FIFOs and exit.

3. Describe your tests and how to run them.
I don't have any tests, but make run_tests will run a bash script that would have tests