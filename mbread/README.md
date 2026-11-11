
# mbread - Very simple custom Modbus TCP client

## How to run?

**All commands must be ran from project's root directory.**

First start the Modbus server simulator. Do this with command *./python/venv/bin/python ./python/modbus_sim.py*
If the server doesn't start, try changing the port in the python script.

After you got the simulator running, run *make -C ./src/*. The program should build successfully

To run the compiled Modbus TCP client run *./src/mbread* with correct options. Program supports 2 modes: writing to a single register and reading sequential register values. Check the TL;DR section below for an example.


**TL;DR**

*./python/venv/bin/python ./python/modbus_sim.py*

*make -C ./src/*

To write value 42 into register 0 *./src/mbread -i 127.0.0.1 -p 1502 -a 0 -w -v 42*
    
To read 10 sequential register values starting from register 0 *./src/mbread -i 127.0.0.1 -p 1502 -a 0 -c 10*


