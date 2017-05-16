# cloudenabler-stress-test
The command is to stimulate thousand of cloud-enabler devices to connect to server (POST every 1/s)

# Prerequisites
 - `curl` library 
 - `pthread` library

# Setup
 - `git clone` this project
 - Type `make` to compile 
 - Deploy `rndpost` to multiple machines if you want to stimulate more cloud-enabler devices (more than thousand)

# Usage (Only one test machine) 
 - Run the stress by `./rndpost -n 1000` 
   The machine will create 1000 isolated threads, which will connect to the server. 
   Then, the threads will post 128 registers' data to the server every 1 second until 
   the user stops the test.

 - Run the stree test on the second machine (if you have) by `./rndpost -n 1000 -s 1000` 
   The second machine will generate another 1000 threads to do the stress test. 