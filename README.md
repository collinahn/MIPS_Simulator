# MIPS_Simulator

### Description
* 32 bit RISC architecture simulator in C
* 32비트 RISC아키텍쳐인 MIPS CPU의 시뮬레이터. 
* 레지스터, PC카운터, ALU 등을 구성하고 이진 파일(프로그램)을 분석하여 MIPS 시스템의 시뮬레이션을 구현한 프로그램.
* LittleEndian으로 구성된 이진 파일을 MIPS 시스템에 맞게 BigEndian으로 변경한 뒤 Disassemble을 진행한다. 
* 명령어set으로 구성된 이진 파일을 입력받아 특정 기능을 수행하고 한 단계씩 실행하거나 전체를 실행하여 결과를 보여준다. 


### Environment
* Windows
* Visual Studio 2019

### Prerequisite
* None

### Files
* In Simulator.c,
```c
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
```
* 공용체의 비트필드를 사용하여 32-bit로 구성된 명령어들을 하나의 변수에 저장하였고, 사용할 땐 명령어의 opcode를 확인하여 각기 다른 포맷으로 처리함으로써 메모리 낭비와 불필요한 연산을 줄였다.


### Usage
* 시뮬레이터 실행 후 명령어
  * L, l: 이진 파일(프로그램)을 로드한다.
  * J, j: 특정 address로 점프한다.
  * G, g: 남은 명령어 수 만큼 동작한다.
  * S, s: 한 단계씩 실행한다.
  * M, m: 지정한 주소의 메모리 상태를 확인한다, 0을 입력하면 요약해서 보여준다.
  * R, r: 레지스터의 상태를 보여준다
  * X, x: 시뮬레이터 종료
  * W, w: 레지스터, 메모리, PC카운터를 초기화한다.
  * sr: 특정 레지스터의 값을 변경한다.
  * sm: 특정 메모리의 값을 변경한다.
