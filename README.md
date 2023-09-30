
# Process-Mock-OS

### Description
This project mocks the fetch and execute actions of an OS.

Child Process - Acts memory for reading the input instructions.

Parent Process - Acts as the CPU for decoding and executing fetched instructions.

### How to Run:
All the code is in the `main.c` file. The input instructions should be placed in the `input.txt` file. 

Once you place your instructions within the `input.txt` file, run the following commands:

    gcc -std=c99 main.c
    ./a.out main.c <timer length>


The first command is for compiling the main.c file. The `-std=c99` flag is necessary because the `main.c` file contains a for-loop and you need the `-std-c99` flag to compile for-loops properly.

The second command is for executing the `main.c` file. Within the `<timer length>` section of this command enter an integer that represents after how many instrcutions should a time out interrupt occur.

### Sample 5 Program
The `sample5.txt` file contains the last sample file that contains instructions of a custom program I wrote.

In this program, it prints an ASCII image depicting a stick figure dunking a basketball

### Additional Notes
For the interrupt handler, my code includes a variable keeping track of whether or not an interrupt should have occurred while a system call was executing (since interrupts are disabled during system calls). When the system call completes and the program returns to the point it left off in the user program, it uses this variable to check whether an interrupt should have occurred while the system call was executing. If so, then then the interrupt handler executes and then it returns back to user mode again. Due to this, do not set the timer value to be less than the length of the system call being executed, or else it will get stuck in an infinite user program-interrupt handler loop.