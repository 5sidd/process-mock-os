
# Process-Mock-OS

### Description
This project mocks the fetch and execute actions of an OS.

Child Process - Acts memory for reading the input instructions. This process also retrieves/updates an address in memory upon request.

Parent Process - Acts as the CPU for decoding and executing fetched instructions.

These are the different registers within the system:
    PC: Program Counter
    SP: Stack Pointer
    IR: Instruction Register
    AC: General purpose register
    X: General purpose register
    Y: General purpose register

The memory for this system can hold up 2000 instructions. The instructions are read from the `input.txt` file and uploaded into a memory array. Within the memory array, addresses 0-999 are designated for the user program, and addresses 1000-1999 are designated for the system. If the user attempts to access system memory, then an error is thrown.

System mode is entered if the user program makes a system call or if an interrupt occurs due to a timeout. A timer value must be provided as a command line argument to determine after how many instructions a timeout should occur. The timeout interrupt handler instructions begin at memory address 1000. The system call handler instructions begin at memory address 1500.

The user stack begins at address 999 and grows up towards address 0. Similarly, the system stack begins at address 1999 and grows upward.

The following is the instruction set that can be executed by the CPU:

    1 = Load value: Load the value into the AC                
    2 = Load addr: Load the value at the address into the AC
    3 = LoadInd addr: Load the value from the address found in the given address into the AC. (for example, if LoadInd 500, and 500 contains 100, then load from 100).
    4 = LoadIdxX addr: Load the value at (address+X) into the AC. (for example, if LoadIdxX 500, and X contains 10, then load from 510).
    5 = LoadIdxY addr: Load the value at (address+Y) into the AC
    6 = LoadSpX: Load from (Sp+X) into the AC (if SP is 990, and X is 1, load from 991).
    7 = Store addr: Store the value in the AC into the address
    8 = Get: Gets a random int from 1 to 100 into the AC
    9 = Put port: If port=1, writes AC as an int to the screen. If port=2, write the char value that maps to the number in the AC (for example if AC = 65, print "A").
    10 = AddX: Add the value in X to the AC
    11 = AddY: Add the value in Y to the AC
    12 = SubX: Subtract the value in X from the AC
    13 = SubY: Subtract the value in Y from the AC
    14 = CopyToX: Copy the value in the AC to X
    15 = CopyFromX: Copy the value in X to the AC
    16 = CopyToY: Copy the value in the AC to Y
    17 = CopyFromY: Copy the value in Y to the AC
    18 = CopyToSp: Copy the value in AC to the SP
    19 = CopyFromSp: Copy the value in SP to the AC    
    20 = Jump addr: Jump to the address
    21 = JumpIfEqual addr: Jump to the address only if the value in the AC is zero
    22 = JumpIfNotEqual addr: Jump to the address only if the value in the AC is not zero
    23 = Call addr: Push return address onto stack, jump to the address
    24 = Ret: Pop return address from the stack, jump to the address
    25 = IncX: Increment the value in X
    26 = DecX: Decrement the value in X
    27 = Push: Push AC onto stack
    28 = Pop: Pop from stack into AC
    29 = Int: Perform system call
    30 = IRet: Return from system call
    50 = End: End execution
   
### How to Run:
All the code is in the `main.c` file. The input instructions should be placed in the `input.txt` file.

In the `input.txt` file, do not include any leading spaces within an instruction or else it will not be read.

Once you place your instructions within the `input.txt` file, run the following commands:

    gcc -std=c99 main.c
    ./a.out main.c <timer length>


The first command is for compiling the main.c file. The `-std=c99` flag is necessary because the `main.c` file contains a for-loop and you need the `-std-c99` flag to compile for-loops properly.

The second command is for executing the `main.c` file. Within the `<timer length>` section of this command enter an integer that represents after how many instrcutions should a time out interrupt occur.

### Sample 5 Program
The `sample5.txt` file contains the last sample file that contains instructions of a custom program I wrote.

In this program, it prints an ASCII image depicting a stick figure dunking a basketball.

### Additional Notes
For the interrupt handler, my code includes a variable keeping track of whether or not an interrupt should have occurred while a system call was executing (since interrupts are disabled during system calls). When the system call completes and the program returns to the point it left off in the user program, it uses this variable to check whether an interrupt should have occurred while the system call was executing. If so, then then the interrupt handler executes and then it returns back to user mode again. Due to this, do not set the timer value to be less than the length of the system call being executed, or else it will get stuck in an infinite user program-interrupt handler loop.
