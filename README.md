# CSM116Cproj1
This is C++ emulation of single-cycle RISC-V processor that can run the following instructions: ADD, SUB, XOR, ADDI, ANDI, SRA LW, SW BLT, JALR


## How to build and run the project

Download all the files in one directory. Then, navigate to the directory and run

$ g++ *.cpp -o cpusim

to build an executable called cpusim

Then, run 
./cpusim <file-name>

to execute the commands. The output would be (a0,a1) where a0 and a1 are the two most recently updated register values.
