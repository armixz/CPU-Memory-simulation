# CPU-Memory-simulation
Multiple Processes and IPC; The project will simulate a simple computer system consisting of a CPU and Memory. The CPU and Memory will be simulated by separate processes that communicate. Memory will contain one program that the CPU will execute and then the simulation will end.

/*
 +--------------+----------------------------------------------------+
 |  Assignment	|  Project #1: Exploring Multiple Processes and IPC  |
 |  Author	    |  Armin Ziaei                                       |
 |  To Compile	|  gcc Main.cpp                                      |
 |  To RUN	    |  ./a.out <int> <filename>                          |
 +--------------+----------------------------------------------------+
*/


sample5.txt
   Tests the int/iret instructions.
   The main loop is printing the String "Timer" (port:2) followed by a Space (Dec: 32)
   and then print the number (port:1) followed by a newline (Dec: 10)
   The number is being periodically incremented by the timer.
   The number will increment faster if the timer period is shorter.
