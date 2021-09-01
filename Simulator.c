#include<stdio.h>
#include<stdlib.h>
#include<string.h>

unsigned int PC = 0x00400000;
int reg[32];
int HI, LO;

int reg_old[32];  // 명령어 실행 전.후 레지스터 값 비교를 위해서
unsigned int nInst, nData;
unsigned char progMEM[0x100000], dataMEM[0x100000], stackMEM[0x100000];
int Z;
int syscall_flag;
union InstructionRegister {
	unsigned int I;
	struct Rformat {
		unsigned int funct : 6;
		unsigned int sh : 5;
		unsigned int rd : 5;
		unsigned int rt : 5;
		unsigned int rs : 5;
		unsigned int opcode : 6;
	} RI;
	struct Iformat {
		int offset : 16;
		unsigned int rt : 5;
		unsigned int rs : 5;
		unsigned int opcode : 6;
	} II;
	struct Jformat {
		unsigned int address : 26;
		unsigned int opcode : 6;
	} JI;
} IR;

unsigned int increasePC(unsigned int PC, unsigned int opcode, unsigned int address, int offset);
void viewMemory();
void loadMemory(unsigned char* fname);
unsigned int saveBigEndian(unsigned char* arr);
void runProgram(const unsigned int x);
void showReg();
void updateReg();
void disassembleInst(const unsigned int x);
unsigned int REG(unsigned int RegNum, unsigned int V, unsigned int nRW); //nRW = 0 read, nRW = 1 write
int MEM(unsigned int A, int V, int nRW, int S);
int ALU(int X, int Y, int S, int* Z);
int logicOperation(int X, int Y, int S);	//c = 0~3
int shiftOperation(int V, int Y, int C);	//v는 value, y는 시프트할 비트의 수
int addSubtract(int X, int Y, int C);
int checkZero(int S);
int checkSetLess(int X, int Y);

void main()
{
	char command[10] = { 0, };
	unsigned char fname[20] = { 0, };
	unsigned int g_rest, j_target, sm_memloc, sr_regnum;
	int sr_regval, m_start, m_end, sm_memval;
	reg[29] = 0x80000000;  //reg[29]=$sp
	reg_old[29] = 0x80000000;

	while (1) {
		printf("Input command: ");
		fflush(stdin);//반복해서 입력받을 시 버퍼 비우기
		scanf_s("%s", command, sizeof(command[0] * 20));

		if (command[1] == 0) {
			switch (command[0]) {
			case 'L':
			case 'l'://load program
				printf("Enter filename: ");
				scanf_s("%s", fname, sizeof(fname));
				loadMemory(fname);
				printf("\nLoad complete\n\n");
				break;

			case 'J':
			case 'j'://jump
				while (1) {
					printf("Enter initial address: ");
					fflush(stdin);
					scanf_s("%x", &j_target);
					if (j_target >= 0x00400000 && j_target <= 0x004fffff && j_target%4 == 0) {
						PC = j_target;
						printf("\nJump success\n\n");
						break;
					}
					else
						printf("jump target error. \n\n");
				}break;

			case 'G':
			case 'g'://go program
				g_rest = nInst - (PC - 0x400000) / 4;
				for (unsigned int i = 0; i < g_rest; i++) {   //남은 명령어 수 만큼 동작
					IR.I = MEM(PC, 0, 0, 2);
					printf("PC value = %#X\n", PC);
					printf("Instruction Code: 0x%08X\n", IR.I);

					disassembleInst(IR.I); //32비트 명령어를 하나씩 어셈블리어로 표현
					runProgram(IR.I);
					if (syscall_flag == 1) {
						syscall_flag = 0;
						break;
					}else {
						PC = increasePC(PC, IR.RI.opcode, IR.JI.address, IR.II.offset);
						printf("Renewed PC = 0x%x\n", PC);
						printf("-----------------------\n");
					}
				}break;

			case 'S':
			case 's'://step
				IR.I = MEM(PC, 0, 0, 2);
				printf("PC value = %#X\n", PC);
				printf("Instruction Code: 0X%08X\n", IR.I);

				disassembleInst(IR.I);
				runProgram(IR.I);
				if (syscall_flag == 1) {
					syscall_flag = 0;
					break;
				}else {
					PC = increasePC(PC, IR.RI.opcode, IR.JI.address, IR.II.offset);
					printf("Renewed PC = 0x%x\n", PC);
					printf("-----------------------\n");
				}
				showReg();
				updateReg();
				break;

			case 'M':
			case 'm': //view memory
				printf("시작 주소를 16진수로 입력하세요(전체 메모리를 간략히 보려면 0을 입력하세요): ");
				scanf_s("%x", &m_start);

				if (m_start == 0)
					viewMemory();
				else {
					printf("끝 주소를 16진수로 입력하세요: ");
					scanf_s("%x", &m_end);
					if (m_start > m_end) printf("시작 주소가 끝 주소보다 큽니다.\n\n");
					else {
						printf("\n");
						for (int i = m_start; i <= m_end; i = i + 4) {
							PC = i;
							IR.I = MEM(PC, 0, 0, 2);
							printf("[%x]   ", PC);
							printf("0x%x\n", IR.I);
						}
					}
				}break;

			case 'R':
			case 'r':
				showReg();
				updateReg();
				break;

			case 'X':
			case 'x':
				printf("Exit Simulator\n");
				exit(0);

			case 'W':
			case 'w': //PC, 레지스터, 메모리를 초기 상태로 되돌림
				PC = 0x400000;
				for (int i = 0; i < 32; i++) {
					if (i == 29) {
						reg[i] = 0x80000000;
						reg_old[i] = 0x80000000;
					}else {
						reg[i] = 0;
						reg_old[i] = 0;
					}
				}
				memset(progMEM, 0, sizeof(progMEM));
				memset(stackMEM, 0, sizeof(stackMEM));
				memset(dataMEM, 0, sizeof(dataMEM));

				printf("\nPC, reg, memory wipe complete\n\n");
				break;

			default:
				printf("command error_main_switch\n\n");
				break;
			}
		}
		else if (strcmp(command, "sr") == 0) {

			printf("Input register number: ");
			scanf_s("%d", &sr_regnum);
			printf("Input register value: ");
			scanf_s("%x", &sr_regval);
			REG(sr_regnum, sr_regval, 1);

			printf("\nRegister value changed\n\n");
			continue;
		}
		else if (strcmp(command, "sm") == 0) {

			printf("Input memory location: ");
			scanf_s("%x", &sm_memloc);
			printf("Input memory value: ");
			scanf_s("%x", &sm_memval);
			MEM(sm_memloc, sm_memval, 1, 2);

			printf("\nMemory value changed\n\n");
			continue;
		}else
			printf("command error_main_if\n");
	}
}


unsigned int increasePC(unsigned int PC, unsigned int opcode, unsigned int address, int offset)
{
	PC += 4;
	switch (opcode) {
	case 1:														 //bltz
		if (reg[IR.II.rs] < reg[IR.II.rt]) PC += offset << 2;
		return PC;
	case 2:
	case 3:
		PC = ((PC >> 28) << 28) + (address << 2);				 //j, jal
		return PC;
	case 4:														//beq
		if (Z == 1) PC += offset << 2;
		return PC;
	case 5:														//bne
		if (Z == 0) PC += offset << 2;
		return PC;
	default:
		return PC;
	}
};


void viewMemory()
{
	printf("\nProgram 0x00400000~\n");
	for (int i = 0; i < 0x60; i++) {
		printf("%02x ", progMEM[i]);
		if (i % 4 == 3) printf("  ");
		if (i % 32 == 31) printf("\n");
	}printf("...\n");

	printf("Data    0x10000000~\n");
	for (int i = 0; i < 0x60; i++) {
		printf("%02x ", dataMEM[i]);
		if (i % 4 == 3) printf("  ");
		if (i % 32 == 31) printf("\n");
	}printf("...\n");

	printf("Stack   ~0x7FFFFFFF\n");
	for (int i = 0xFFFA0; i < 0x100000; i++) {
		printf("%02x ", stackMEM[i]);
		if (i % 4 == 3) printf("  ");
		if (i % 32 == 31) printf("\n");
	}printf("...\n\n");
	return;
};


void loadMemory(unsigned char* fname)
{
	unsigned char buf[4] = { 0, };
	FILE* pFile = NULL;
	errno_t err;

	err = fopen_s(&pFile, fname, "rb");
	if (err) {
		printf("Cannot open file\n");
		exit(1);
	}

	fread(buf, sizeof(buf[0]), 4, pFile);
	nInst = saveBigEndian(buf);
	fread(buf, sizeof(buf[0]), 4, pFile);
	nData = saveBigEndian(buf);

	fread(progMEM, sizeof(progMEM[0]), nInst * 4, pFile);
	for (unsigned int i = 0; i < nInst * 4; i += 4) {
		IR.I = saveBigEndian(&progMEM[i]);
	}
	fread(dataMEM, sizeof(dataMEM[0]), nData * 4, pFile);
	for (unsigned int i = 0; i < nData * 4; i += 4) {
		IR.I = saveBigEndian(&dataMEM[i]);
	}

	fclose(pFile);
};


unsigned int saveBigEndian(unsigned char* arr)
{
	unsigned int n = 0;
	for (int j = 0; j < 4; j++) {
		n += (unsigned int)arr[j] << (24 - 8 * j);
	} return n;
};


void runProgram(const unsigned int x)
{
	long long  d = 0;
	int a1 = 0, a2 = 0;
	unsigned int offset16 = 0;

	if (IR.RI.opcode == 0) {
		switch (IR.RI.funct) {
		case 0: //sll
			reg[IR.RI.rd] = ALU(reg[IR.RI.rt], IR.RI.sh, 0b0001, &Z);
			break;
		case 2: //srl
			reg[IR.RI.rd] = ALU(reg[IR.RI.rt], IR.RI.sh, 0b0010, &Z);
			break;
		case 3: //sra
			reg[IR.RI.rd] = ALU(reg[IR.RI.rt], IR.RI.sh, 0b0011, &Z);
			break;
		case 32: //add
			reg[IR.RI.rd] = ALU(reg[IR.RI.rs], reg[IR.RI.rt], 0b1000, &Z);
			break;
		case 34: //sub
			reg[IR.RI.rd] = ALU(reg[IR.RI.rs], reg[IR.RI.rt], 0b1001, &Z);
			break;
		case 36: //and
			reg[IR.RI.rd] = ALU(reg[IR.RI.rs], reg[IR.RI.rt], 0b1100, &Z);
			break;
		case 37: //or
			reg[IR.RI.rd] = ALU(reg[IR.RI.rs], reg[IR.RI.rt], 0b1101, &Z);
			break;
		case 38: //xor
			reg[IR.RI.rd] = ALU(reg[IR.RI.rs], reg[IR.RI.rt], 0b1110, &Z);
			break;
		case 39: //nor
			reg[IR.RI.rd] = ALU(reg[IR.RI.rs], reg[IR.RI.rt], 0b1111, &Z);
			break;
		case 42: //slt
			reg[IR.RI.rd] = ALU(reg[IR.RI.rs], reg[IR.RI.rt], 0b0100, &Z);
			break;
		case 16: //mfhi
			reg[IR.RI.rd] = HI;
			break;
		case 18: //mflo
			reg[IR.RI.rd] = LO;
			break;
		case 24: //mult
			a1 = reg[IR.RI.rs]; a2 = reg[IR.RI.rt];
			while (a2 != 0) {
				if ((a2 & 1) != 0) d += a1;
				a1 = a1 << 1;
				a2 = a2 >> 1;
			}
			LO = d & 0x00000000ffffffff;
			HI = (d & 0xffffffff00000000) >> 32;
			break;
		case 8: //jr
			PC = reg[31] - 4; //바로 다음에 실행되는 increasePC()가 +4 해줌으로써 레지스터에 저장된 PC주소로 점프하게 됨.
			break;
		case 12: //syscall
			if (IR.RI.opcode == 0 && IR.RI.funct == 12 && reg[2] == 10) {	//syscall 10
				char ans = 0;

				syscall_flag = 1;
				printf("\nEnd of command\n\n");
				printf("Program exit?(y/n) ");
				scanf_s(" %c", &ans, sizeof(ans)); printf("\n");

				if (ans == 'y' || ans == 'Y') {
					printf("Goodbye.\n");
					exit(0);
				}break;
			}
			break;
		default:
			printf("function error_runProgram\n");
			break;
		}
	}
	else {
		switch (IR.RI.opcode) {
		case 1: //bltz  
		case 2: //j j-format
			break; //case 1, 2 모두 increasePC()에서 동작
		case 3: //jal j-format  
			reg[31] = PC + 4;
			break;
		case 4: //beq
		case 5: //bne
			ALU(reg[IR.II.rs], reg[IR.II.rt], 0b1001, &Z); //increasePC()에서 Z값을 가져다 사용
			break;
		case 8: //addi
			reg[IR.II.rt] = ALU(reg[IR.II.rs], IR.II.offset, 0b1000, &Z);
			break;
		case 10: //slti
			reg[IR.II.rt] = ALU(reg[IR.II.rs], IR.II.offset, 0b0100, &Z);
			break;
		case 12: //andi
			reg[IR.II.rt] = ALU(reg[IR.II.rs], IR.II.offset, 0b1100, &Z);
			break;
		case 13: //ori
			offset16 = IR.II.offset & 0x0000FFFF;	// 16비트 offset 값만 전달되도록 마스킹
			reg[IR.II.rt] = ALU(reg[IR.II.rs], offset16, 0b1101, &Z);
			break;
		case 14: //xori
			reg[IR.II.rt] = ALU(reg[IR.II.rs], IR.II.offset, 0b1110, &Z);
			break;
		case 15: //lui
			reg[IR.II.rt] = IR.II.offset << 16;
			break;
		case 32: //lb 나머지 24비트 부호확장
			reg[IR.II.rt] = MEM(reg[IR.II.rs] + IR.II.offset, 0, 0, 0);
			break;
		case 35: //lw
			reg[IR.II.rt] = MEM(reg[IR.II.rs] + IR.II.offset, 0, 0, 2);
			break;
		case 36: //lbu 실제 1바이트만 가져옴
			reg[IR.II.rt] = MEM(reg[IR.II.rs] + IR.II.offset, 0, 0, 0);
			reg[IR.II.rt] &= 0x000000FF;
			break;
		case 40: //sb
			MEM(reg[IR.II.rs] + IR.II.offset, reg[IR.II.rt], 1, 0);
			break;
		case 43: //sw
			MEM(reg[IR.II.rs] + IR.II.offset, reg[IR.II.rt], 1, 2);
			break;
		default:
			printf("opcode error_runProgram\n");
			break;
		}
	}
	return;
};


void showReg()
{
	unsigned char pseudoreg[32][5] = 
	{ "zero", "at", "v0", "v1",
		"a0", "a1", "a2", "a3",
		"t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
		"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
		"t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra", };

	printf("Regno.      Regvalue  Regval_renewed\n");
	for (int i = 0; i < 32; i++) {
		printf("$%2d", i);
		printf(" [$%s] ", pseudoreg[i]); if (i != 0) printf("  ");
		printf("%8x        ", reg_old[i]);
		printf("%8x", reg[i]);
		if (reg[i] != reg_old[i]) printf("  <--"); //달라진 레지스터 값 표현
		printf("\n");
	}printf("\n");
	return;
};


void updateReg()
{
	for (int i = 0; i < 32; i++)
		reg_old[i] = reg[i];
};


void disassembleInst(const unsigned int x)
{
	printf("Disassemble     : ");
	if (IR.RI.opcode == 0) { // instruction encoding
		switch (IR.RI.funct) {
		case 0:
			printf("sll");
			break;
		case 2:
			printf("srl");
			break;
		case 3:
			printf("sra");
			break;
		case 8:
			printf("jr");
			break;
		case 12:
			printf("syscall");
			break;
		case 16:
			printf("mfhi");
			break;
		case 18:
			printf("mflo");
			break;
		case 24:
			printf("mul");
			break;
		case 32:
			printf("add");
			break;
		case 34:
			printf("sub");
			break;
		case 36:
			printf("and");
			break;
		case 37:
			printf("or");
			break;
		case 38:
			printf("xor");
			break;
		case 39:
			printf("nor");
			break;
		case 42:
			printf("slt");
			break;
		default:
			printf("RI_err");
			break;
		}
	}
	else {
		switch (IR.RI.opcode) {
		case 1:
			printf("bltz");
			break;
		case 2:
			printf("j");            //j-format
			break;
		case 3:
			printf("jal");          //j-format
			break;
		case 4:
			printf("beq");
			break;
		case 5:
			printf("bne");
			break;
		case 8:
			printf("addi");
			break;
		case 10:
			printf("slti");
			break;
		case 12:
			printf("andi");
			break;
		case 13:
			printf("ori");
			break;
		case 14:
			printf("xori");
			break;
		case 15:
			printf("lui");
			break;
		case 32:
			printf("lb");
			break;
		case 35:
			printf("lw");
			break;
		case 36:
			printf("lbu");
			break;
		case 40:
			printf("sb");
			break;
		case 43:
			printf("sw");
			break;
		default:
			printf("nRI_err");
			break;
		}
	}printf(" ");

	if (IR.RI.opcode == 0) { //R-type instruction disassemble
		switch (IR.RI.funct) {
		case 0:
		case 2:
		case 3:
			printf("$%d, $%d, %d", IR.RI.rd, IR.RI.rt, IR.RI.sh);         //case of sll, srl, sra
			break;
		case 8:
			printf("$%d", IR.RI.rs);                                      //case of jr
			break;
		case 12:
			printf("%d", reg[2]);
			break;                                                        //case of syscall
		case 16:
		case 18:														  //case of mfhi, mflo
			printf("$%d", IR.RI.rd);
			break;
		case 24:														  //case of mult
			printf("$%d, $%d", IR.RI.rs, IR.RI.rt);
			break;
		default:
			printf("$%d, $%d, $%d", IR.RI.rd, IR.RI.rs, IR.RI.rt);        //Rformat disassemble
			break;
		}
	}

	else if (IR.RI.opcode == 2 || IR.RI.opcode == 3) //J-type instruction disassemble
		printf("0x%08x", (((PC + 4) >> 28) << 28) + ((unsigned int)IR.JI.address << 2));  

	else {	//I-type instruction disassemble
		switch (IR.RI.opcode) {
		case 1:
			printf("$%d, $%d", IR.II.rs, IR.II.offset * 4);					//case of bltz
			break;
		case 4:
		case 5:
			printf("$%d, $%d, %d", IR.II.rt, IR.II.rs, IR.II.offset * 4);   //case of bne, beq
			break;
		case 15:
			printf("$%d, %d", IR.II.rt, IR.II.offset);						//case of lui
			break;
		case 32:
		case 35:
		case 36:
		case 40:
		case 43:
			printf("$%d, %d($%d)", IR.II.rt, IR.II.offset, IR.II.rs);     //case of lb, lw, lbu, sb, sw
			break;
		default:
			printf("$%d, $%d, %d", IR.II.rt, IR.II.rs, IR.II.offset);     
			break;
		}
	} printf("\n");
	return;
};


unsigned int REG(unsigned int RegNum, unsigned int V, unsigned int nRW)
{
	if (nRW != 0) {
		reg[RegNum] = V;
	}
	return reg[RegNum];
};


int MEM(unsigned int A, int V, int nRW, int S) {
	unsigned int SEL, OFFSET;
	unsigned char* pM = NULL;
	short int halfword = 0;
	int word = 0;
	char v3, v2, v1, v0;

	SEL = A >> 20;
	OFFSET = A & 0x000FFFFF; //입력받은 32bit 주소를 각각 select 12bit와 OFFSET 20bit로 나눔

	if (SEL == 0x004) pM = progMEM;
	else if (SEL == 0x100) pM = dataMEM;
	else if (SEL == 0x7FF) pM = stackMEM;
	else {
		printf("No memory\n");
		exit(1);
	}

	switch (S) {
	case 0: //byte access
		if (nRW == 0) { //read 1 byte
			return pM[OFFSET];
		}else if (nRW == 1) { //write 1 byte
			*(pM + OFFSET) = (unsigned char)V;
			return 0;
		}else {
			printf("nRW error_MEM_byte\n");
			break;
		}
	case 1:  //access half word
		if (nRW == 0) { //read halfword big endian
			halfword = pM[OFFSET++] << 8;
			halfword += pM[OFFSET];
			return halfword;
		}else if (nRW == 1) { //write halfword big endian
			v0 = (V << 16) >> 24;
			v1 = (V << 24) >> 24;
			*(pM + OFFSET++) = v0;
			*(pM + OFFSET) = v1;
			return 0;
		}else {
			printf("nRW error_MEM_halfword\n");
			break;
		}
	case 2: //access word
		if (nRW == 0) { // read word big endian
			word = pM[OFFSET++] << 24;
			word += pM[OFFSET++] << 16;
			word += pM[OFFSET++] << 8;
			word += pM[OFFSET];
			return word;
		}else if (nRW == 1) { //write word big endian
			v0 = V >> 24;
			v1 = (V << 8) >> 24;
			v2 = (V << 16) >> 24;
			v3 = (V << 24) >> 24;
			*(pM + OFFSET++) = v0;
			*(pM + OFFSET++) = v1;
			*(pM + OFFSET++) = v2;
			*(pM + OFFSET) = v3;
			return 0;
		}else {
			printf("nRW error_MEM_word\n");
			break;
		}
	default:
		printf("Size error_MEM\n");
		break;
	}
	return 0;
};


int ALU(int X, int Y, int S, int* Z)
{
	int c32, c10, c0;
	int ret;

	c32 = (S >> 2) & 3; // C의 값을 0x00000003과 bitwise and시켜서 C3과 C2의 값을 C32에 저장
	c10 = S & 3;
	c0 = S & 1; //한 비트만 사용하는 addSubtract용

	if (c32 == 0) {
		ret = shiftOperation(X, Y, c10);
	}else if (c32 == 1) {
		ret = checkSetLess(X, Y);
	}else if (c32 == 2) {
		ret = addSubtract(X, Y, c0);
		*Z = checkZero(ret);
	}else {
		ret = logicOperation(X, Y, c10);
	}
	return ret;
};


int shiftOperation(int V, int Y, int C)
{
	int ret = 0;
	int mask = 0xffffffff;

	if (C < 0 || C > 3) {
		printf("error in shift operation\n");
		exit(1);
	}

	if (C == 0) {				// no shift
		ret = V;
	}else if (C == 1) {	    	// logical left
		V = V << Y;
		ret = V;
	}else if (C == 2) {	    	// logical right
		V = (unsigned int)V >> Y;
		ret = V;
	}else {				    	// arithmetic right(V >> Y)
		if (V >= 0) {
			V = (unsigned int)V >> Y;
			ret = V;
		}else {					// 컴파일러가 자동으로 unsigned int형은 logical shift, signed int형은 arith shift를 해주긴 함
			V = V >> Y;
			V = V | (mask << (32 - Y));
			ret = V;
		}
	}
	return ret;
};


int checkSetLess(int X, int Y)
{
	int ret;

	if (X < Y)
		ret = 1;
	else
		ret = 0;

	return ret;
};


int addSubtract(int X, int Y, int C)
{
	int ret = 0;

	if (C < 0 || C > 1) {
		printf("error in add/subtract operation\n");
		exit(1);
	}

	if (C == 0)
		ret = X + Y;
	else
		ret = X - Y;

	return ret;
};


int checkZero(int S)
{
	int ret;

	if (S == 0)
		ret = 1;
	else
		ret = 0;

	return ret;
};



int logicOperation(int X, int Y, int S)
{
	if (S < 0 || S > 3) {
		printf("error in logic operation\n");
		exit(1);
	}

	if (S == 0)
		return X & Y;
	else if (S == 1)
		return X | Y;
	else if (S == 2)
		return X ^ Y;
	else
		return ~(X | Y);
};