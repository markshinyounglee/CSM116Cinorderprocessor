#include <iostream>
#include <bitset> // for bitset
#include <stdio.h>
#include<stdlib.h>
#include <string>
#include <stdint.h> // for uint8_t
#include <errno.h> // for stderr
using namespace std;

class Controller;
class ALU;

enum ALUOP // enumerate all possible instructions
{
	IDLE = 0b0000,
	AND = 0b1111,
	XOR = 0b0001,
	ADD = 0b0010,
	SUB = 0b0110,
	SHIFT = 0b1001
};

enum OPTYPE 
{
	RTYPE = 0b0110011, // includes SRA
	ITYPE = 0b0010011,
	// RSHIFT = 0b0110011,
	LOAD = 0b0000011,
	STORE = 0b0100011,
	BTYPE = 0b1100011,
	JTYPE = 0b1100111
};

class instruction {
public:
	bitset<32> instr;//instruction
	instruction(bitset<32> fetch); // constructor

};

class CPU {
private:
	int dmemory[4096]; //data memory byte addressable in little endian fashion;
	int reg[32]; // 32 RISC-V registers
	unsigned long PC; //pc 
	unsigned long curr_PC;
	unsigned long rs1, rs2; // source registers
	unsigned long rd; // destination register
	uint32_t imm; // 32-bit immediate value
	Controller* controller; //controller
	ALU* alu; //alu
	uint32_t aluResult; // everything besides LW
	uint32_t memResult; // LW
	void Execute();
	void Memory(bitset<32> address);

public:
	int cache[2];// two most recently accessed registers
	CPU();
	~CPU();
	unsigned long readPC();
	bitset<32> Fetch(bitset<8> *instmem);
	bool Decode(instruction* instr); // call computeCtrlSignals
	void Run();
};


// add other functions and objects here
class Controller {
public:
	bool regW, aluSrc, branch, memR, memW, memToReg, jump;
	ALUOP aluOp;
	void computeCtrlSignals(bitset<32> instruction);
	
};

class ALU {
public:
	bool lessThanFlag; // for BLT
	bitset<32> computeALU(uint8_t control, bitset<32> data1, bitset<32> data2);

};
// end of extra classes
