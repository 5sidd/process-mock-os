#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <ctype.h>

int main(int argc, char* argv[]) {
    //Create pipes for IPC
    int fd1[2]; // parent writes to child; child reads from parent
    int fd2[2]; // child writes to parent; parent reads from child
    
    //Check if any errors occurred
    //Valid = 0
    //Invalid = 1
    int isValid = 0;
    
    if (pipe(fd1) == -1) {
        printf("Error: There was an error while piping fd1. %s\n", strerror(errno));
        isValid = 1;
    }
    
    if (pipe(fd2) == -1) {
        printf("Error: There was an error while piping fd2. %s\n", strerror(errno));
        isValid = 1;
    }
    
    //Fork to create 2 processes - one for CPU and one for Memory
    //Child process is memory, parent process is CPU
    int id = fork();
    if (id < 0) {
        printf("Error: There was an error while forking. %s\n", strerror(errno));
        close(fd1[0]);
        close(fd1[1]);
        close(fd2[0]);
        close(fd2[1]);
        isValid = 1;
    }
    
    //Proceed if no errors occurred
    if (isValid == 0 && id == 0) {
        //Child reads from fd1 --> close write portion of fd1
        //Child writes to fd2 --> close read portion of fd2
        close(fd1[1]);
        close(fd2[0]);
        
        //Memory array
        int memory[2000];
        
        //Read input file
        FILE* inputFile = fopen("./input.txt", "r");
        if (inputFile == NULL) {
            printf("Error: The input file could not be read.\n");
            close(fd1[0]);
            close(fd2[1]);
            isValid = 1;
        }
        
        //Read the instructions into memory array
        int index = 0;
        char* line = NULL;
        size_t lineLength = 0;
        ssize_t lineRead;

        while ((lineRead = getline(&line, &lineLength, inputFile)) != -1) {
            //Process the string to only include the relevant portion 
            int trueLength = 0;
            for (char* i = line; !isspace(*i); i++) {
                trueLength += 1;
            }
            
            char instruction[trueLength + 1];
            memcpy(instruction, &line[0], trueLength);
            instruction[trueLength] = '\0';

            //Jump to an address in memory
            if (instruction[0] == '.') {
                char address[trueLength - 1];
                memcpy(address, &instruction[1], trueLength - 1);
                address[trueLength - 1] = '\0';
            
                index = atoi(address);
                continue;
            }
            
            //Set the current memory address to contain the instruction value
            if (trueLength > 0) {
                int instructionNumber = atoi(instruction);
                memory[index] = instructionNumber;
                index += 1;
            }
        }
        
        //Close the input file
        fclose(inputFile);
        if (line) {
            free(line);
        }
        
        while (isValid == 0) {
            //Action 1 = return something from memory
            //Action 2 = write something to memory
            int action;
            int actionRes = read(fd1[0], &action, sizeof(int));
            
            if (actionRes == -1) {
                printf("Error: There was an error while the child process was reading. %s\n", strerror(errno));
                isValid = 1;
                break;
            }
            else if (actionRes == 0) {
                printf("EOF reached in parent process. Terminating child...");
                isValid = 1;
                break;
            }
            
            if (action == 1) {
                //Read the address from which the memory should retrieve the data
                int address;
                int addressRes1 = read(fd1[0], &address, sizeof(int));
                
                if (addressRes1 == -1) {
                    printf("Error: There was an error while the child process was reading. %s\n", strerror(errno));
                    isValid = 1;
                    break;
                }
                else if (addressRes1 == 0) {
                    printf("EOF reached in parent process. Terminating child...");
                    isValid = 1;
                    break;
                }
                
                //Return the data retrieved from the address to the CPU
                int data = memory[address];
                if (write(fd2[1], &data, sizeof(int)) == -1) {
                    printf("Error: There was an error while the child process was writing. %s\n", strerror(errno));
                    isValid = 1;
                    break;
                }
            }
            else if (action == 2) {
                //Read the address from which the memory should write to
                int address;
                int addressRes2 = read(fd1[0], &address, sizeof(int));
                
                if (addressRes2 == -1) {
                    printf("Error: There was an error while the child process was reading. %s\n", strerror(errno));
                    isValid = 1;
                    break;
                }
                else if (addressRes2 == 0) {
                    printf("EOF reached in parent process. Terminating child...");
                    isValid = 1;
                    break;
                }
                
                //Read the data that should be written
                int data;
                int dataRes = read(fd1[0], &data, sizeof(int));
                
                if (dataRes== -1) {
                    printf("Error: There was an error while the child process was reading.\n");
                    isValid = 1;
                    break;
                }
                else if (addressRes2 == 0) {
                    printf("EOF reached in parent process. Terminating child...");
                    isValid = 1;
                    break;
                }
                
                //Write the given data to the given address
                memory[address] = data;
            }
        }
    }
    else if (isValid == 0 && id > 0) {
        //Parent reads from fd2
        //Parent writes to fd1
        close(fd1[0]);
        close(fd2[1]);
        
        //Register values
        int PC = 0;
        int SP = -1;
        int IR;
        int AC;
        int X;
        int Y;
        
        //Timer length size for timer interrupt
        int timerLength;
        
        //To check if the previous instruction accepts an argument
        int dependency = -1;
        
        //To check if we are currently in user or system mode
        //User mode = 0 (default)
        //System mode = 1
        int mode = 0;
        
        while (isValid == 0) {
            //Check if the user is attempting to access system-level memory
            if (mode == 0 && PC > 999) {
                printf("Error: User attempting to access system-level memory.\n");
                isValid = 1;
                break;
            }
            
            //Inform child address we want to retrieve something from memory
            int retrieve = 1;
            if (write(fd1[1], &retrieve, sizeof(int)) == -1) {
                printf("Error: There was an error while the parent process was writing. %s\n", strerror(errno));
                isValid = 1;
                break;
            }
            
            //Specify the memory address to the child process whose value we want to retrieve
            if (write(fd1[1], &PC, sizeof(int)) == -1) {
                printf("Error: There was an error while the parent process was writing. %s\n", strerror(errno));
                isValid = 1;
                break;
            }
            
            //Load the instruction at memory address PC into the IR
            int loadInstructionRes = read(fd2[0], &IR, sizeof(int));
            
            if (loadInstructionRes == -1) {
                printf("Error: There was an error while the parent process was reading. %s\n", strerror(errno));
                isValid = 1;
                break;
            }
            else if (loadInstructionRes == 0) {
                printf("EOF reached in child process. Terminating parent...");
                isValid = 1;
                break;
            }
            
            //printf("PC: %d, AC: %d, Instruction: %d, X: %d, Y: %d, dependency: %d\n", PC, AC, IR, X, Y, dependency);
            
            //If the previous instruction accepts an argument
            if (dependency != -1) {
                if (dependency == 1) {
                    //store value in IR into the AC
                    AC = IR;
                }
                else if (dependency == 2) {
                    //Load addr --> get the value in addr from child and store in AC --> addr = IR value
                    
                    //Check if we are in user mode and accessing system-level memory
                    if (mode == 0 && IR > 999) {
                        printf("Error: User attempting to access system-level memory.\n");
                        isValid = 1;
                        break;
                    }
                    
                    //Inform child address we want to retrieve something from memory
                    int retrieve = 1;
                    if (write(fd1[1], &retrieve, sizeof(int)) == -1) {
                        printf("Error: There was an error while the parent process was writing. %s\n", strerror(errno));
                        isValid = 1;
                        break;
                    }
                    
                    //Specify to child process we want to retrieve the value at memory address IR
                    if (write(fd1[1], &IR, sizeof(int)) == -1) {
                        printf("Error: There was an error while the parent process was writing. %s\n", strerror(errno));
                        isValid = 1;
                        break;
                    }
                    
                    //Load the value at address IR into the AC
                    int loadInstructionRes = read(fd2[0], &AC, sizeof(int));
            
                    if (loadInstructionRes == -1) {
                        printf("Error: There was an error while the parent process was reading. %s\n", strerror(errno));
                        isValid = 1;
                        break;
                    }
                    else if (loadInstructionRes == 0) {
                        printf("EOF reached in child process. Terminating parent...");
                        isValid = 1;
                        break;
                    }
                }
                else if (dependency == 3) {
                    //LoadInd addr --> x = IR value --> get value in memory address x and store it in AC
                }
                else if (dependency == 4 || dependency == 5) {
                    //LoadIdxX addr --> z = value in IR + value in X/Y register --> store value in address z in AC
                    
                    //The target address whose value we want to retrieve
                    int targetAddress;
                    
                    if (dependency == 4) {
                        targetAddress = IR + X;
                    }
                    else {
                        targetAddress = IR + Y;
                    }
                    
                    //Check if we are in user mode and accessing system level memory
                    if (mode == 0 && targetAddress > 999) {
                        printf("Error: User attempting to access system-level memory.\n");
                        isValid = 1;
                        break;
                    }
                    
                    //Inform child address we want to retrieve something from memory
                    int retrieve = 1;
                    if (write(fd1[1], &retrieve, sizeof(int)) == -1) {
                        printf("Error: There was an error while the parent process was writing. %s\n", strerror(errno));
                        isValid = 1;
                        break;
                    }
                    
                    //Specify to child process we want to retrieve the value at memory address targetAddress
                    if (write(fd1[1], &targetAddress, sizeof(int)) == -1) {
                        printf("Error: There was an error while the parent process was writing. %s\n", strerror(errno));
                        isValid = 1;
                        break;
                    }
                    
                    //Load the value at address targetAddress into the AC
                    int loadTargetAddressRes = read(fd2[0], &AC, sizeof(int));
            
                    if (loadTargetAddressRes== -1) {
                        printf("Error: There was an error while the parent process was reading. %s\n", strerror(errno));
                        isValid = 1;
                        break;
                    }
                    else if (loadTargetAddressRes == 0) {
                        printf("EOF reached in child process. Terminating parent...");
                        isValid = 1;
                        break;
                    }
                }
                else if (dependency == 7) {
                    //Store addr --> store the value in AC into the memory address IR
                }
                else if (dependency == 9) {
                    //Put port --> if IR is 1 write print AC as an int --> if IR is 2 print the ASCII char corresponding to AC
                    
                    //Print as integer
                    if (IR == 1) {
                        printf("%d", AC);
                    }
                    
                    //Print as char
                    if (IR == 2) {
                        //Convert int to ASCII char
                        char a = (char) 0;
                        char b = a + AC;
                        printf("%c", b);
                    }
                }
                else if (dependency == 20) {
                    //Jump addr --> set value of PC to IR --> IR = memory address to jump to
                    
                    //Check if we are in user mode and accessing system level memory
                    if (mode == 0 && IR > 999) {
                        printf("Error: User attempting to access system-level memory.\n");
                        isValid = 1;
                        break;
                    }
                    
                    //Set the value of PC to IR
                    PC = IR;
                    
                    //Do not increment PC
                    dependency = -1;
                    continue;
                }
                else if (dependency == 21) {
                    //JumpIfEqual addr --> set value of PC to IR if AC is equal to 0 --> IR = memory address to jump to
                    
                    if (AC == 0) {
                        //Check if we are in user mode and accessing system level memory
                        if (mode == 0 && IR > 999) {
                            printf("Error: User attempting to access system-level memory.\n");
                            isValid = 1;
                            break;
                        }
                        
                        //Set PC to IR
                        PC = IR;
                        
                        //Do not increment PC
                        dependency = -1;
                        continue;
                    }
                }
                else if (dependency == 22) {
                    //JumpIfNotEqual addr --> set value of PC to IR if AC is not equal to 0
                }
                else if (dependency == 23) {
                    //Call addr --> push value of current PC to stack --> set value of PC to IR
                }
                
                //Continue to next iteration by setting by resetting dependency and incrementing the PC register
                dependency = -1;
                PC = PC + 1;
                continue;
            }
            
            //The value in the IR is an independent instruction and not an argument for a previous instruction
            if (IR < 1 || IR > 50) {
                //This is an invalid instruction
                printf("Error: Invalid instruction at memory address %d\n", PC);
                isValid = 1;
            }
            else if (IR == 50) {
                //Program has ended
                break;
            }
            else if (IR == 1 || IR == 2 || IR == 3 || IR == 4 || IR == 5 || IR == 7 || IR == 9 || IR == 20 || IR == 21 || IR == 22 || IR == 23) {
                //dependency instructions
                dependency = IR;
            }
            else if (IR == 8) {
                //Generate a random number between 1-100 --> store in AC
                AC = (rand() % (100 - 1 + 1)) + 1;
            }
            else if (IR == 10) {
                //Add X to the AC
                AC = AC + X;
            }
            else if (IR == 11) {
                //Add Y to the AC
                AC = AC + Y;
            }
            else if (IR == 12) {
                //Subtract X from the AC
                AC = AC - X;
            }
            else if (IR == 13) {
                //Substract Y from AC
                AC = AC - Y;
            }
            else if (IR == 14) {
                //Copy the value in AC to X
                X = AC;
            }
            else if (IR == 15) {
                //Copy the value X to AC
                AC = X;
            }
            else if (IR == 16) {
                //Copy the value in the AC to Y
                Y = AC;
            }
            else if (IR == 17) {
                //Copy the value in Y to AC
                AC = Y;
            }
            else if (IR == 18) {
                //Copy the value of AC to SP
                SP = AC;
            }
            else if (IR == 19) {
                //Copy the value of SP to AC
                AC = SP;
            }
            else if (IR == 24) {
                //Pop address from stack and jump to the address contained in the popped node
                
            }
            else if (IR == 25) {
                //Increment the value in X
                X = X + 1;
            }
            else if (IR == 26) {
                //Decrement the value in X
                X = X - 1;
            }
            
            //Increment the PC to the next instruction address
            PC = PC + 1;
        }
    }
    
    //Close remaining file descriptors
    if (id == 0) {
        //For child
        close(fd1[0]);
        close(fd2[1]); 
    }
    else if (id > 0) {
        //For parent
        close(fd2[0]);
        close(fd1[1]);
        
        //Wait for child process to terminate
        waitpid(0, NULL, 0);
    }
    
    return 0;
}