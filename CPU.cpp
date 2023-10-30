#include "CPU.h"

unsigned long extractBits(bitset<32> instruction, int lsb, int cnt=5)
{
	// extract bits from least significant bit
	// by default, extract five bits
	unsigned long num = 0;
	for(int i = 0; i < cnt; i++) {
		num += instruction[lsb+i]<<i;
	}
	return num;
}

instruction::instruction(bitset<32> fetch)
{
	// cout << fetch << endl;
	instr = fetch;
	// cout << instr << endl;
}

CPU::CPU()
{
	PC = 0; //set PC to 0
	controller = new Controller(); //initialize controller
	alu = new ALU(); // initialize ALU
	for (int i = 0; i < 4096; i++) //copy instrMEM
	{
		dmemory[i] = (0);
	}
}

CPU::~CPU()
{
	delete controller;
	delete alu;
}

bitset<32> CPU::Fetch(bitset<8> *instmem) {
	// last byte is the greatest byte because it is little endian machine
	
	// DEBUGGING CODE
	// printf("PC is currently at: %X\n", PC);

	bitset<32> instr = ((((instmem[PC + 3].to_ulong()) << 24)) + ((instmem[PC + 2].to_ulong()) << 16) + ((instmem[PC + 1].to_ulong()) << 8) + (instmem[PC + 0].to_ulong()));  //get 32 bit instruction
	curr_PC = PC;//store current PC address
	PC += 4;//increment PC
	return instr;
}


bool CPU::Decode(instruction* curr) // decode 32-bit instruction
{
	// cout<<curr->instr<<endl;
	// curr->instr is bitset<32> so we should convert to ulong before doing any operation
	unsigned long currInstr = curr->instr.to_ulong();
	bitset<32> currInstrBit = curr->instr;

	if(!currInstr) { // if all zeros, then we are done
		// cout << "end of instructions" << endl;
		// end of instruction if all bytes are zeros
		return false;
	}
	controller->computeCtrlSignals(currInstrBit);

	rs1 = extractBits(currInstrBit, 15);
	rs2 = extractBits(currInstrBit, 20);
	rd = extractBits(currInstrBit, 7);
	//generate imm for ITYPE, LOAD, STORE, BTYPE, JTYPE
	switch(currInstr & 0x7F)
	{
		case OPTYPE::RTYPE:
			rs1 = extractBits(currInstrBit, 15);
	        rs2 = extractBits(currInstrBit, 20);
			rd = extractBits(currInstrBit, 7);
			imm = 0;
			break;
		case OPTYPE::ITYPE:
		case OPTYPE::LOAD:
		case OPTYPE::JTYPE:
			rs1 = extractBits(currInstrBit, 15);
			rs2 = 0;
			rd = extractBits(currInstrBit, 7);
			imm = extractBits(currInstrBit, 20, 12);
			break;
		case OPTYPE::STORE:
			rs1 = extractBits(currInstrBit, 15);
			rs2 = extractBits(currInstrBit, 20);
			rd = 0;
			imm = extractBits(currInstrBit, 7, 5) + extractBits(currInstrBit, 25, 7);
			break;
		case OPTYPE::BTYPE:
			rs1 = extractBits(currInstrBit, 15);
			rs2 = extractBits(currInstrBit, 20);
			rd = 0;
			imm = (extractBits(currInstrBit, 8, 4)<<1) + (extractBits(currInstrBit, 25, 6)<<5) + (currInstrBit[7]<<11) + (currInstrBit[31]<<12);
			break;
	}
/*
Strategy:
1. take the lower 7-bits (opcode) to determine which type of instruction it is
2. according to the instruction scheme, split instruction
*/
	return true;
}

unsigned long CPU::readPC()
{
	return PC;
}

void CPU::Run()
{
	Execute();
	Memory(aluResult);

	// this segment acts like a MUX
	if(controller->memToReg && !controller->jump)
	{
		reg[rd] = memResult;
	}
	else if(!controller->memToReg && controller->jump)
	{
		reg[rd] = PC;
	}
	else if(!controller->memToReg && !controller->jump)
	{
		reg[rd] = aluResult;
	}
	// end of MUX


	// MUX for PC
	if(controller->branch && alu->lessThanFlag)
	{
		// DEBUGGING CODE
		/*
		cout << "Less Than Flag Raised" << endl;
		cout << "curr_PC: " << curr_PC << " imm: " << imm << endl;
		*/


		PC = curr_PC + imm; // branch to label
		// REMEMBER that label is always relative to current PC address
	}
	else if(controller->jump)
	{
		PC = aluResult; // jump to result from ALU
	}
	// else PC = PC + 4, which was already calculated in CPU::Fetch()
	// end of MUX

	reg[0] = 0; // x0 is hard-coded to 0 so we always flush the result
	alu->lessThanFlag = 1; // return this back to 1

	// update cache
	cache[1] = cache[0]; //most recent one goes to [1]
	cache[0] = reg[rd]; //most recent one is from rd

	// DEBUGGING CODE
	/*
	printf("Our next PC: %X\n", PC);
	for(int i = 0; i < 32; i++)
	{	
		if(i % 8 == 0)
		{
			cout << '\t';
		}
		cout << reg[i] << ' ';
	}
	cout << "\n================" << endl;
	*/
}

void CPU::Execute()
{
	bitset<32> op1(reg[rs1]);
	bitset<32> op2;
	// this segment acts like a MUX
	if(!controller->aluSrc) {
		op2 = bitset<32>(reg[rs2]);
	} else {
		op2 = bitset<32>(imm);
	}
	// end of MUX

	aluResult = (alu->computeALU(controller->aluOp, op1, op2)).to_ulong();
	if(controller->jump) {
		aluResult &= 0xFFFFFFFE;
	}

	// if R-TYPE (including RSHIFT), result goes to rd
	
	// if I-TYPE (or LOAD), result goes to rd

	// if STORE, alu computes the address and computed result is stored in the address through CPU::Memory()

	// if BTYPE, alu computes rs1-rs2 to see whether we should branch

	// if JTYPE, alu computes the address to jump to

}

// Add other functions here ... 

void CPU::Memory(bitset<32> address)
{
	/*! 
	TO DO: implement the program that retrieves 32-bits of data from
	target address to write to the destination register 
	*/
	if (controller->memW) {
		dmemory[address.to_ulong()] = reg[rs2]; // stored at aluResult address
		
		// DEBUGGING CODE
		/*
		cout << "address where memory is stored: " << address.to_ulong();
		cout << " and we are storing: " << reg[rs2] << endl;
		*/
	} 
	else if (controller->memR) {
		memResult = dmemory[address.to_ulong()]; // return 4 bytes
		
		// DEBUGGING CODE
		/*
		cout << "address where memory is retrieved: " << address.to_ulong();
		cout << " and we are getting: " << memResult << endl;
		*/
	}
}

void Controller::computeCtrlSignals(bitset<32> instruction)
{
	uint8_t opcode = instruction.to_ulong() & 0x7F; // take lower 7 bits
	switch(opcode)
	{
		case OPTYPE::RTYPE:
			regW = 1;
			aluSrc = 0;
			branch = 0;
			memR = 0;
			memW = 0;
			memToReg = 0;
			// first instruction[14], then instruction[30]
			if (instruction[12]) // SRA
			{
				aluOp = ALUOP::SHIFT;
			}
			else if (instruction[14]) // XOR
			{
				aluOp = ALUOP::XOR;
			} else if (instruction[30]) { // SUB
				aluOp = ALUOP::SUB;
			} else { // ADD
				aluOp = ALUOP::ADD;
			}
			jump = 0;
			break;
		case OPTYPE::ITYPE:
			regW = 1;
			aluSrc = 1;
			branch = 0;
			memR = 0;
			memW = 0;
			memToReg = 0;	
			if (instruction[14])  // ANDI
			{
				aluOp = ALUOP::AND;
			} else { // ADDI
				aluOp = ALUOP::ADD;
			}
			jump = 0;
			break;
		case OPTYPE::LOAD: // LW
			regW = 1;
			aluSrc = 1;
			branch = 0;
			memR = 1;
			memW = 0;
			memToReg = 1;	
			aluOp = ALUOP::ADD;
			jump = 0;
			break;
		case OPTYPE::STORE: // SW
			regW = 0;
			aluSrc = 1;
			branch = 0;
			memR = 0;
			memW = 1;
			memToReg = 0;	
			aluOp = ALUOP::ADD;
			jump = 0;
			break;
		case OPTYPE::BTYPE: // BLT
			regW = 0;
			aluSrc = 1;
			branch = 1;
			memR = 0;
			memW = 0;
			memToReg = 0;	
			aluOp = ALUOP::SUB;
			jump = 0;
			break;
		case OPTYPE::JTYPE: // JALR
			regW = 0;
			aluSrc = 1;
			branch = 0;
			memR = 0;
			memW = 0;
			memToReg = 0;	
			aluOp = ALUOP::ADD;
			jump = 1;
			break;
		default:
			fprintf(stderr, "Wrong opcode generated");
			exit(1);
	}
}

bitset<32> ALU::computeALU(uint8_t control, bitset<32> data1, bitset<32> data2)
{
	bitset<32> result;
	switch(control)
	{
		case ALUOP::AND:
			result = data1 & data2;
			break;
		case ALUOP::XOR:
			result = data1 ^ data2;
			break;
		case ALUOP::ADD:
			result = bitset<32>(data1.to_ulong() + data2.to_ulong());
			break;
		case ALUOP::SUB:
			result = bitset<32>(data1.to_ulong() - data2.to_ulong());
			lessThanFlag = data1.to_ulong() < data2.to_ulong() ? 1 : 0;
			break;
		case ALUOP::SHIFT:
			result = bitset<32>(data1.to_ulong() >> data2.to_ulong()); 
			break;
		case ALUOP::IDLE:
			result = bitset<32>(0);
			break;
		default:
			perror("Invalid ALU operation");
			exit(1);
	}
	return result;
}