#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_events.h>
#include <stdint.h>
#include <stdio.h>

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;

bool extended_mem = false;
bool extended_stack = false;
bool timer_int = false;

bool cpudebug = false;
bool iodebug = false;

uint8_t mem[65536];
uint8_t p_input[8];
uint8_t p_output[24];
uint16_t stack[16];
uint8_t r_a, r_b, r_c, r_d, r_e, r_h, r_l, r_sp;
bool f_carry, f_zero, f_sign, f_parity, f_halt;

int rom_start = 0x3800;
int rom_end = 0x3FFF;

void ram_init()
{
	mem[0x0000] = 0x00;
	//CILOC: JMP 0x3CA0 - redirect back to built-in "TTYIN"
	//mem[0x3700] = 0x44;
	//mem[0x3701] = 0xA0;
	//mem[0x3702] = 0x3C;
	//COLOC: JMP 0x3C5C - redirect back to built-in "TTYOUT"
	//mem[0x3703] = 0x44;
	//mem[0x3704] = 0x5C;
	//mem[0x3705] = 0x3C;
	//R1LOC:
	//R2LOC:
	//P1LOC:
	//P2LOC:
	//L1LOC:
	//L2LOC:
	//CSLOC:
}

void ResetCpu()
{
	r_sp = 0;
	stack[r_sp] = rom_start; //Intellec 8/Mod 8 ROM start
	f_carry = f_zero = f_sign = f_parity = false;
	f_halt = false;

	ram_init();
}

//hardware
//in	0	UART data in
//in	1	UART status
//in	2
//in	3	tape reader in
//in	4
//in	5
//in	6
//in	7
//out	8	UART data out
//out	9	UART/TTY control
//out	A	
//out	B	tape reader out

bool Uart_Rx_Rdy = true;
bool Uart_Tx_Rdy = false;
uint8_t Uart_Rx = 0xFF;
uint8_t Uart_Tx = 0xFF;
bool crlf = false;

void GetNextKey()
{
	SDL_Event kbd;
	bool shift;
	bool ctrl;
	bool caps;

	//LF after sending CR when hitting enter
	if (crlf)
	{
		Uart_Rx_Rdy = false;
		Uart_Rx = '\012';
		crlf = false;
		return;
	}

	if(SDL_PollEvent(&kbd) && kbd.type == SDL_EVENT_KEY_DOWN)
	{
		shift = !!(kbd.key.mod & SDL_KMOD_SHIFT);
		ctrl = !!((kbd.key.mod & SDL_KMOD_CTRL) | (kbd.key.mod & SDL_KMOD_ALT));
		caps = kbd.key.mod & SDL_KMOD_CAPS;

		switch (kbd.key.key)
		{
			case SDLK_TILDE:
			case SDLK_GRAVE:
			case SDLK_EXCLAIM:
			case SDLK_1:
			case SDLK_AT:
			case SDLK_2:
			case SDLK_HASH:
			case SDLK_3:
			case SDLK_DOLLAR:
			case SDLK_4:
			case SDLK_PERCENT:
			case SDLK_5:
			case SDLK_CARET:
			case SDLK_6:
			case SDLK_AMPERSAND:
			case SDLK_7:
			case SDLK_ASTERISK:
			case SDLK_8:
			case SDLK_LEFTPAREN:
			case SDLK_9:
			case SDLK_RIGHTPAREN:
			case SDLK_0:
			case SDLK_UNDERSCORE:
			case SDLK_MINUS:
			case SDLK_PLUS:
			case SDLK_EQUALS:
			case SDLK_BACKSPACE:
			case SDLK_TAB:
			case SDLK_Q:
			case SDLK_W:
			case SDLK_E:
			case SDLK_R:
			case SDLK_T:
			case SDLK_Y:
			case SDLK_U:
			case SDLK_I:
			case SDLK_O:
			case SDLK_P:
			case SDLK_LEFTBRACKET:
			case SDLK_LEFTBRACE:
			case SDLK_RIGHTBRACKET:
			case SDLK_RIGHTBRACE:
			case SDLK_PIPE:
			case SDLK_BACKSLASH:
			case SDLK_A:
			case SDLK_S:
			case SDLK_D:
			case SDLK_F:
			case SDLK_G:
			case SDLK_H:
			case SDLK_J:
			case SDLK_K:
			case SDLK_L:
			case SDLK_SEMICOLON:
			case SDLK_COLON:
			case SDLK_APOSTROPHE:
			case SDLK_DBLAPOSTROPHE:
			case SDLK_RETURN:
			case SDLK_Z:
			case SDLK_X:
			case SDLK_C:
			case SDLK_V:
			case SDLK_B:
			case SDLK_N:
			case SDLK_M:
			case SDLK_LESS:
			case SDLK_COMMA:
			case SDLK_GREATER:
			case SDLK_PERIOD:
			case SDLK_QUESTION:
			case SDLK_SLASH:
			case SDLK_UP:
			case SDLK_LEFT:
			case SDLK_RIGHT:
			case SDLK_DOWN:
			case SDLK_ESCAPE:
			case SDLK_SPACE:
				Uart_Rx_Rdy = false;
				break;

			default:
				break;
		}

		switch (kbd.key.key)
		{
			case SDLK_TILDE:
			case SDLK_GRAVE:
				Uart_Rx = shift ? '~' : '`';
				break;

			case SDLK_EXCLAIM:
			case SDLK_1:
				Uart_Rx = shift ? '!' : '1';
				break;

			case SDLK_AT:
			case SDLK_2:
				Uart_Rx = (ctrl && shift) ? '\000' : shift ? '@' : '2';
				break;

			case SDLK_HASH:
			case SDLK_3:
				Uart_Rx = shift ? '#' : '3';
				break;

			case SDLK_DOLLAR:
			case SDLK_4:
				Uart_Rx = shift ? '$' : '4';
				break;

			case SDLK_PERCENT:
			case SDLK_5:
				Uart_Rx = shift ? '%' : '5';
				break;

			case SDLK_CARET:
			case SDLK_6:
				Uart_Rx = (ctrl && shift) ? '\036' : shift ? '^' : '6';
				break;

			case SDLK_AMPERSAND:
			case SDLK_7:
				Uart_Rx = shift ? '&' : '7';
				break;

			case SDLK_ASTERISK:
			case SDLK_8:
				Uart_Rx = shift ? '*' : '8';
				break;

			case SDLK_LEFTPAREN:
			case SDLK_9:
				Uart_Rx = shift ? '(' : '9';
				break;

			case SDLK_RIGHTPAREN:
			case SDLK_0:
				Uart_Rx = shift ? ')' : '0';
				break;

			case SDLK_UNDERSCORE:
			case SDLK_MINUS:
				Uart_Rx = shift ? '_' : '-';
				break;

			case SDLK_PLUS:
			case SDLK_EQUALS:
				Uart_Rx = shift ? '+' : '=';
				break;

			case SDLK_BACKSPACE:
				Uart_Rx = '\010';
				break;

			case SDLK_TAB:
				Uart_Rx = '\011';
				break;

			case SDLK_Q:
				Uart_Rx = ctrl ? '\021' : (shift ^ caps) ? 'Q' : 'q';
				break;

			case SDLK_W:
				Uart_Rx = ctrl ? '\027' : (shift ^ caps) ? 'W' : 'w';
				break;

			case SDLK_E:
				Uart_Rx = ctrl ? '\005' : (shift ^ caps) ? 'E' : 'e';
				break;

			case SDLK_R:
				Uart_Rx = ctrl ? '\022' : (shift ^ caps) ? 'R' : 'r';
				break;

			case SDLK_T:
				Uart_Rx = ctrl ? '\024' : (shift ^ caps) ? 'T' : 't';
				break;

			case SDLK_Y:
				Uart_Rx = ctrl ? '\031' : (shift ^ caps) ? 'Y' : 'y';
				break;

			case SDLK_U:
				Uart_Rx = ctrl ? '\025' : (shift ^ caps) ? 'U' : 'u';
				break;

			case SDLK_I:
				Uart_Rx = ctrl ? '\011' : (shift ^ caps) ? 'I' : 'i';
				break;

			case SDLK_O:
				Uart_Rx = ctrl ? '\017' : (shift ^ caps) ? 'O' : 'o';
				break;

			case SDLK_P:
				Uart_Rx = ctrl ? '\020' : (shift ^ caps) ? 'P' : 'p';
				break;

			case SDLK_LEFTBRACKET:
			case SDLK_LEFTBRACE:
				Uart_Rx = ctrl ? '\033' : shift ? '{' : '[';
				break;

			case SDLK_RIGHTBRACKET:
			case SDLK_RIGHTBRACE:
				Uart_Rx = ctrl ? '\035' : shift ? '}' : ']';
				break;

			case SDLK_PIPE:
			case SDLK_BACKSLASH:
				Uart_Rx = shift ? '|' : '\\';
				break;

			case SDLK_A:
				Uart_Rx = ctrl ? '\001' : shift ^ caps ? 'A' : 'a';
				break;

			case SDLK_S:
				Uart_Rx = ctrl ? '\023' : shift ^ caps ? 'S' : 's';
				break;

			case SDLK_D:
				Uart_Rx = ctrl ? '\004' : shift ^ caps ? 'D' : 'd';
				break;

			case SDLK_F:
				Uart_Rx = ctrl ? '\006' : shift ^ caps ? 'F' : 'f';
				break;

			case SDLK_G:
				Uart_Rx = ctrl ? '\007' : shift ^ caps ? 'G' : 'g';
				break;

			case SDLK_H:
				Uart_Rx = ctrl ? '\010' : shift ^ caps ? 'H' : 'h';
				break;

			case SDLK_J:
				Uart_Rx = ctrl ? '\012' : shift ^ caps ? 'J' : 'j';
				break;

			case SDLK_K:
				Uart_Rx = ctrl ? '\013' : shift ^ caps ? 'K' : 'k';
				break;

			case SDLK_L:
				Uart_Rx = ctrl ? '\014' : shift ^ caps ? 'L' : 'l';
				break;

			case SDLK_SEMICOLON:
			case SDLK_COLON:
				Uart_Rx = shift ? ':' : ';';
				break;

			case SDLK_APOSTROPHE:
			case SDLK_DBLAPOSTROPHE:
				Uart_Rx = shift ? '"' : '\'';
				break;

			case SDLK_RETURN:
				Uart_Rx = '\015';
				crlf = true;
				break;

			case SDLK_Z:
				Uart_Rx = ctrl ? '\032' : (shift ^ caps) ? 'Z' : 'z';
				break;

			case SDLK_X:
				Uart_Rx = (ctrl && shift) ? '\030' : ctrl ? '\034' : (shift ^ caps) ? 'X' : 'x';
				break;

			case SDLK_C:
				Uart_Rx = ctrl ? '\003' : (shift ^ caps) ? 'C' : 'c';
				break;

			case SDLK_V:
				Uart_Rx = ctrl ? '\026' : (shift ^ caps) ? 'V' : 'v';
				break;

			case SDLK_B:
				Uart_Rx = ctrl ? '\002' : (shift ^ caps) ? 'B' : 'b';
				break;

			case SDLK_N:
				Uart_Rx = ctrl ? '\016' : (shift ^ caps) ? 'N' : 'n';
				break;

			case SDLK_M:
				Uart_Rx = ctrl ? '\015' : (shift ^ caps) ? 'M' : 'm';
				break;

			case SDLK_LESS:
			case SDLK_COMMA:
				Uart_Rx = shift ? '<' : ',';
				break;

			case SDLK_GREATER:
			case SDLK_PERIOD:
				Uart_Rx = shift ? '>' : '.';
				break;

			case SDLK_QUESTION:
			case SDLK_SLASH:
				Uart_Rx = shift ? '?' : '/';
				break;

			case SDLK_UP:
				Uart_Rx = '\013';
				break;

			case SDLK_LEFT:
				Uart_Rx = '\010';
				break;

			case SDLK_RIGHT:
				Uart_Rx = '\014';
				break;

			case SDLK_DOWN:
				Uart_Rx = '\012';
				break;

			case SDLK_ESCAPE:
				Uart_Rx = '\033';
				break;

			case SDLK_SPACE:
				Uart_Rx = ' ';
				break;

			default:
				break;
		}
	}
	else
	{
		;
	}
}

void RunIo(int port)
{
	switch (port)
	{
		//input ports

		//UART - data in (keyboard input)
		case 0:
			r_a = ~Uart_Rx;
			Uart_Rx_Rdy = true;
			break;

		//status - UART, programmer, and tape punch status
		case 1:
			r_a =
				(Uart_Rx_Rdy ? 1 : 0) +
				(Uart_Tx_Rdy ? 4 : 0)
				;
			break;

		//PROM programmer - data in from EPROM
		case 2:
			//TODO: read EPROM data from file
			r_a = 0xFF;
			break;
			
		//tape punch - data in from tape
		case 3:
			//TODO: read tape data from file
			r_a = 0xFF;
			break;

		//data stack - data in (pop)
		case 4:
			//TODO: implement 256-level stack
			r_a = 0xFF;
			break;

		//spare
		case 5:
			r_a = 0xFF;
			break;

		//spare
		case 6:
			r_a = 0xFF;
			break;

		//spare
		case 7:
			r_a = 0xFF;
			break;

		//output ports

		//UART - data out (emulated terminal screen output)
		case 8:
			//Uart_Tx = r_a;
			//Uart_Tx_Rdy = false;
			//printf("%c", (char)r_a);
			putchar(~r_a);
			Uart_Tx_Rdy = false;
			break;

		//control - tape punch, PROM programmer
		case 9:
			putchar(r_a);
			break;

		//PROM programmer - address
		case 10:
			//TODO: implement fake PROM programmer
			break;

		//PROM programmer, tape punch - data out
		case 11:
			//TODO: implement fake PROM programmer and tape punch output to file
			break;

		//data stack - data output (push)
		case 12:
			//TODO: implement 256-level stack
			break;

		case 13:
			break;

		case 14:
			break;

		case 15:
			break;

		case 16:
			break;

		case 17:
			break;

		case 18:
			break;

		case 19:
			break;

		case 20:
			break;

		case 21:
			break;

		case 22:
			break;

		case 23:
			break;

		case 24:
			break;

		case 25:
			break;

		case 26:
			break;

		case 27:
			break;

		case 28:
			break;

		case 29:
			break;

		case 30:
			break;

		case 31:
			break;

		default:
			break;
	}

	if (cpudebug || iodebug) printf(port < 8 ? "INP" : "OUTP");
	if (cpudebug || iodebug) printf(" %i: %02X\r\n", port, r_a);
}

void LoadPC(int value)
{
	stack[r_sp] = value & (extended_mem ? 0xFFFF : 0x3FFF);
}

void IncPC()
{
	LoadPC(stack[r_sp] + 1);
}

void IncSP()
{
	r_sp = (r_sp + 1) & (extended_stack ? 0x0F : 0x07);
}

void DecSP()
{
	r_sp = (r_sp - 1) & (extended_stack ? 0x0F : 0x07);
}

void Jump()
{
	uint8_t imm1, imm2;
	IncPC();
	imm1 = mem[stack[r_sp]];
	IncPC();
	imm2 = mem[stack[r_sp]];
	LoadPC((imm2 << 8) + imm1);
	if (cpudebug) printf("%04X", (imm2 << 8) + imm1);
}

void Call()
{
	uint8_t imm1, imm2;
	IncPC();
	imm1 = mem[stack[r_sp]];
	IncPC();
	imm2 = mem[stack[r_sp]];
	IncPC();
	IncSP();
	LoadPC((imm2 << 8) + imm1);
	if (cpudebug) printf("%04X", (imm2 << 8) + imm1);
}

uint8_t GetHL()
{
	return mem[((r_h << 8) + r_l) & (extended_mem ? 0xFFFF : 0x3FFF)];
}

void SetHL(uint8_t val)
{
	int addr = ((r_h << 8) + r_l) & (extended_mem ? 0xFFFF : 0x3FFF);
	if (addr < rom_start || addr > rom_end)
	{
		mem[addr] = val;
	}
}

bool Parity(uint8_t bits)
{
	bool even = true;

	for (int i = 0; i < 7; i++)
	{
		if (bits & (1 << i))
		{
			even = !even;
		}
	}

	return even;
}

uint8_t GetRegVal(int reg)
{
	switch (reg)
	{
		case 0:
			return r_a;
		case 1:
			return r_b;
		case 2:
			return r_c;
		case 3:
			return r_d;
		case 4:
			return r_e;
		case 5:
			return r_h;
		case 6:
			return r_l;
		case 7:
			return GetHL();
		default:
			printf("GetRegVal - invalid register source specified.\r\n");
			return 0;
	}
}

void SetRegVal(int reg, uint8_t val)
{
	switch (reg)
	{
		case 0:
			r_a = val;
			break;

		case 1:
			r_b = val;
			break;

		case 2:
			r_c = val;
			break;

		case 3:
			r_d = val;
			break;

		case 4:
			r_e = val;
			break;

		case 5:
			r_h = val;
			break;

		case 6:
			r_l = val;
			break;

		case 7:
			SetHL(val);
			break;

		default:
			printf("SetRegVal - invalid register destination specified.\r\n");
			break;
	}
}

char regletter(int reg)
{
	switch (reg)
	{
		case 0:
			return 'A';
		case 1:
			return 'B';
		case 2:
			return 'C';
		case 3:
			return 'D';
		case 4:
			return 'E';
		case 5:
			return 'H';
		case 6:
			return 'L';
		case 7:
			return 'M';
		default:
			return 'Z';
	}
}

//emulate as if running at 800khz when timing emulated I/O
void RunCpu()
{
	uint8_t instruction;
	uint8_t imm;
	uint16_t dest;
	uint16_t tmp_acc;
	bool ctmp;
	int sreg;
	int dreg;

	instruction = mem[stack[r_sp]];

	sreg = instruction & 0b00000111;
	dreg = (instruction & 0b00111000) >> 3;

	if (f_halt)
	{
		return;
	}

	if (Uart_Rx_Rdy)
	{
		GetNextKey();
	}

	if (cpudebug) printf("REGS: A:%02X B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X M:%02X\r\n", r_a, r_b, r_c, r_d, r_e, r_h, r_l, mem[(r_h << 8) + r_l]);
	if (cpudebug) printf("FLAG: C:%c Z:%c S:%c P:%c\r\n", f_carry ? 'T' : 'F', f_zero ? 'T' : 'F', f_sign ? 'T' : 'F', f_parity ? 'T' : 'F');
	if (cpudebug) printf("INST: %04X ", stack[r_sp]);

	switch (instruction)
	{
		//LrM1rm2
		case 0b11000000:
		case 0b11000001:
		case 0b11000010:
		case 0b11000011:
		case 0b11000100:
		case 0b11000101:
		case 0b11000110:
		case 0b11000111:
		case 0b11001000:
		case 0b11001001:
		case 0b11001010:
		case 0b11001011:
		case 0b11001100:
		case 0b11001101:
		case 0b11001110:
		case 0b11001111:
		case 0b11010000:
		case 0b11010001:
		case 0b11010010:
		case 0b11010011:
		case 0b11010100:
		case 0b11010101:
		case 0b11010110:
		case 0b11010111:
		case 0b11011000:
		case 0b11011001:
		case 0b11011010:
		case 0b11011011:
		case 0b11011100:
		case 0b11011101:
		case 0b11011110:
		case 0b11011111:
		case 0b11100000:
		case 0b11100001:
		case 0b11100010:
		case 0b11100011:
		case 0b11100100:
		case 0b11100101:
		case 0b11100110:
		case 0b11100111:
		case 0b11101000:
		case 0b11101001:
		case 0b11101010:
		case 0b11101011:
		case 0b11101100:
		case 0b11101101:
		case 0b11101110:
		case 0b11101111:
		case 0b11110000:
		case 0b11110001:
		case 0b11110010:
		case 0b11110011:
		case 0b11110100:
		case 0b11110101:
		case 0b11110110:
		case 0b11110111:
		case 0b11111000:
		case 0b11111001:
		case 0b11111010:
		case 0b11111011:
		case 0b11111100:
		case 0b11111101:
		case 0b11111110:
			SetRegVal(dreg, GetRegVal(sreg));
			IncPC();
			if (cpudebug) printf("L%c%c", regletter(dreg), regletter(sreg));
			break;

		//LrI/LMI
		case 0b00000110:
		case 0b00001110:
		case 0b00010110:
		case 0b00011110:
		case 0b00100110:
		case 0b00101110:
		case 0b00110110:
		case 0b00111110:
			IncPC();
			imm = mem[stack[r_sp]];
			SetRegVal(dreg, imm);
			IncPC();
			if (cpudebug) printf("L%cI %02X", regletter(dreg), imm);
			break;

		//INr
		case 0b00001000:
		case 0b00010000:
		case 0b00011000:
		case 0b00100000:
		case 0b00101000:
		case 0b00110000:
		case 0b00111000:
			tmp_acc = GetRegVal(dreg) + 1;
			f_sign = tmp_acc & 0x80;
			f_zero = (tmp_acc & 0xFF) == 0x00;
			f_parity = Parity(tmp_acc & 0xFF);
			SetRegVal(dreg, tmp_acc & 0xFF);
			IncPC();
			if (cpudebug) printf("IN%c", regletter(dreg));
			break;
		
		//DCr
		case 0b00001001:
		case 0b00010001:
		case 0b00011001:
		case 0b00100001:
		case 0b00101001:
		case 0b00110001:
		case 0b00111001:
			tmp_acc = GetRegVal(dreg) - 1;
			f_sign = tmp_acc & 0x80;
			f_zero = (tmp_acc & 0xFF) == 0x00;
			f_parity = Parity(tmp_acc & 0xFF);
			SetRegVal(dreg, tmp_acc & 0xFF);
			IncPC();
			if (cpudebug) printf("DC%c", regletter(dreg));
			break;

		//ADr/ADM
		case 0b10000000:
		case 0b10000001:
		case 0b10000010:
		case 0b10000011:
		case 0b10000100:
		case 0b10000101:
		case 0b10000110:
		case 0b10000111:
			tmp_acc = r_a + GetRegVal(sreg);
			f_carry = tmp_acc > 0xFF;
			f_sign = tmp_acc & 0x80;
			f_zero = (tmp_acc & 0xFF) == 0x00;
			f_parity = Parity(tmp_acc & 0xFF);
			r_a = tmp_acc & 0xFF;
			IncPC();
			if (cpudebug) printf("AD%c", regletter(sreg));
			break;

			//ADI
		case 0b00000100:
			IncPC();
			imm = mem[stack[r_sp]];
			tmp_acc = r_a + imm;
			f_carry = tmp_acc > 0xFF;
			f_sign = tmp_acc & 0x80;
			f_zero = (tmp_acc & 0xFF) == 0x00;
			f_parity = Parity(tmp_acc & 0xFF);
			r_a = tmp_acc & 0xFF;
			IncPC();
			if (cpudebug) printf("ADI %02X", imm);
			break;

		//ACr/ACM
		case 0b10001000:
		case 0b10001001:
		case 0b10001010:
		case 0b10001011:
		case 0b10001100:
		case 0b10001101:
		case 0b10001110:
		case 0b10001111:
			tmp_acc = (r_a + GetRegVal(sreg)) + (f_carry ? 1 : 0);
			f_carry = tmp_acc > 0xFF;
			f_sign = tmp_acc & 0x80;
			f_zero = (tmp_acc & 0xFF) == 0x00;
			f_parity = Parity(tmp_acc & 0xFF);
			r_a = tmp_acc & 0xFF;
			IncPC();
			if (cpudebug) printf("AC%c", regletter(sreg));
			break;

			//ACI
		case 0b00001100:
			IncPC();
			imm = mem[stack[r_sp]];
			tmp_acc = (r_a + imm) + (f_carry ? 1 : 0);
			f_carry = tmp_acc > 0xFF;
			f_sign = tmp_acc & 0x80;
			f_zero = (tmp_acc & 0xFF) == 0x00;
			f_parity = Parity(tmp_acc & 0xFF);
			r_a = tmp_acc & 0xFF;
			IncPC();
			if (cpudebug) printf("ACI %02X", imm);
			break;

		//SUr/SUM
		case 0b10010000:
		case 0b10010001:
		case 0b10010010:
		case 0b10010011:
		case 0b10010100:
		case 0b10010101:
		case 0b10010110:
		case 0b10010111:
			tmp_acc = r_a - GetRegVal(sreg);
			f_carry = tmp_acc > 0xFF;
			f_sign = tmp_acc & 0x80;
			f_zero = (tmp_acc & 0xFF) == 0x00;
			f_parity = Parity(tmp_acc & 0xFF);
			r_a = tmp_acc & 0xFF;
			IncPC();
			if (cpudebug) printf("SU%c", regletter(sreg));
			break;

			//SUI
		case 0b00010100:
			IncPC();
			imm = mem[stack[r_sp]];
			tmp_acc = r_a - imm;
			f_carry = tmp_acc > 0xFF;
			f_sign = tmp_acc & 0x80;
			f_zero = (tmp_acc & 0xFF) == 0x00;
			f_parity = Parity(tmp_acc & 0xFF);
			r_a = tmp_acc & 0xFF;
			IncPC();
			if (cpudebug) printf("SUI %02X", imm);
			break;

		//SBr/SBM
		case 0b10011000:
		case 0b10011001:
		case 0b10011010:
		case 0b10011011:
		case 0b10011100:
		case 0b10011101:
		case 0b10011110:
		case 0b10011111:
			tmp_acc = (r_a - GetRegVal(sreg)) - (f_carry ? 1 : 0);
			f_carry = tmp_acc > 0xFF;
			f_sign = tmp_acc & 0x80;
			f_zero = (tmp_acc & 0xFF) == 0x00;
			f_parity = Parity(tmp_acc & 0xFF);
			r_a = tmp_acc & 0xFF;
			IncPC();
			if (cpudebug) printf("SB%c", regletter(sreg));
			break;

			//SBI
		case 0b00011100:
			IncPC();
			imm = mem[stack[r_sp]];
			tmp_acc = (r_a - imm) - (f_carry ? 1 : 0);
			f_carry = tmp_acc > 0xFF;
			f_sign = tmp_acc & 0x80;
			f_zero = (tmp_acc & 0xFF) == 0x00;
			f_parity = Parity(tmp_acc & 0xFF);
			r_a = tmp_acc & 0xFF;
			IncPC();
			if (cpudebug) printf("SBI %02X", imm);
			break;

		//NDr/NDM
		case 0b10100000:
		case 0b10100001:
		case 0b10100010:
		case 0b10100011:
		case 0b10100100:
		case 0b10100101:
		case 0b10100110:
		case 0b10100111:
			r_a &= GetRegVal(sreg);
			f_carry = false;
			f_sign = r_a & 0x80;
			f_zero = (r_a == 0x00);
			f_parity = Parity(r_a);
			IncPC();
			if (cpudebug) printf("ND%c", regletter(sreg));
			break;

			//NDI
		case 0b00100100:
			IncPC();
			imm = mem[stack[r_sp]];
			r_a &= imm;
			f_carry = false;
			f_sign = r_a & 0x80;
			f_zero = (r_a == 0x00);
			f_parity = Parity(r_a);
			IncPC();
			if (cpudebug) printf("NDI %02X", imm);
			break;

		//XRr/XRM
		case 0b10101000:
		case 0b10101001:
		case 0b10101010:
		case 0b10101011:
		case 0b10101100:
		case 0b10101101:
		case 0b10101110:
		case 0b10101111:
			r_a ^= GetRegVal(sreg);
			f_carry = false;
			f_sign = r_a & 0x80;
			f_zero = (r_a == 0x00);
			f_parity = Parity(r_a);
			IncPC();
			if (cpudebug) printf("XR%c", regletter(sreg));
			break;

			//XRI
		case 0b00101100:
			IncPC();
			imm = mem[stack[r_sp]];
			r_a ^= imm;
			f_carry = false;
			f_sign = r_a & 0x80;
			f_zero = (r_a == 0x00);
			f_parity = Parity(r_a);
			IncPC();
			if (cpudebug) printf("XRI %02X", imm);
			break;

		//ORr/ORM
		case 0b10110000:
		case 0b10110001:
		case 0b10110010:
		case 0b10110011:
		case 0b10110100:
		case 0b10110101:
		case 0b10110110:
		case 0b10110111:
			r_a |= GetRegVal(sreg);
			f_carry = false;
			f_sign = r_a & 0x80;
			f_zero = (r_a == 0x00);
			f_parity = Parity(r_a);
			IncPC();
			if (cpudebug) printf("OR%c", regletter(sreg));
			break;

		//ORI
		case 0b00110100:
			IncPC();
			imm = mem[stack[r_sp]];
			r_a |= imm;
			f_carry = false;
			f_sign = r_a & 0x80;
			f_zero = (r_a == 0x00);
			f_parity = Parity(r_a);
			IncPC();
			if (cpudebug) printf("ORI %02X", imm);
			break;

		//CPr/CPM
		case 0b10111000:
		case 0b10111001:
		case 0b10111010:
		case 0b10111011:
		case 0b10111100:
		case 0b10111101:
		case 0b10111110:
		case 0b10111111:
			tmp_acc = r_a - GetRegVal(sreg);
			f_carry = tmp_acc > 0xFF;
			f_sign = tmp_acc & 0x80;
			f_zero = (tmp_acc & 0xFF) == 0x00;
			f_parity = Parity(tmp_acc & 0xFF);
			IncPC();
			if (cpudebug) printf("CP%c", regletter(sreg));
			break;

		//CPI
		case 0b00111100:
			IncPC();
			imm = mem[stack[r_sp]];
			tmp_acc = r_a - imm;
			f_carry = tmp_acc > 0xFF;
			f_sign = tmp_acc & 0x80;
			f_zero = (tmp_acc & 0xFF) == 0x00;
			f_parity = Parity(tmp_acc & 0xFF);
			IncPC();
			if (cpudebug) printf("CPI %02X", imm);
			break;

		//RLC
		case 0b00000010:
			ctmp = (r_a & 0x80);
			r_a = (r_a << 1) | (ctmp ? 0x01 : 0x00);
			IncPC();
			if (cpudebug) printf("RLC");
			break;

		//RRC
		case 0b00001010:
			ctmp = (r_a & 0x01);
			r_a = (r_a >> 1) | (ctmp ? 0x80 : 0x00);
			IncPC();
			if (cpudebug) printf("RRC");
			break;

		//RAL
		case 0b00010010:
			ctmp = f_carry;
			f_carry = (r_a & 0x80);
			r_a = (r_a << 1) | (ctmp ? 0x01 : 0x00);
			IncPC();
			if (cpudebug) printf("RAL");
			break;
		
		//RAR
		case 0b00011010:
			ctmp = f_carry;
			f_carry = (r_a & 0x01);
			r_a = (r_a >> 1) | (ctmp ? 0x80 : 0x00);
			IncPC();
			if (cpudebug) printf("RAR");
			break;

		//JMP
		case 0b01000100:
		case 0b01001100:
		case 0b01010100:
		case 0b01011100:
		case 0b01100100:
		case 0b01101100:
		case 0b01110100:
		case 0b01111100:
			if (cpudebug) printf("JMP ");
			Jump();
			break;

		//JFc
		case 0b01000000:
			if (cpudebug) printf("JFC ");
			if (!f_carry)
			{
				Jump();
			}
			else
			{
				IncPC();
				imm = mem[stack[r_sp]];
				IncPC();
				dest = (mem[stack[r_sp]] << 8) + imm;
				IncPC();
				if (cpudebug) printf("%04X", dest);
			}
			break;
		case 0b01001000:
			if (cpudebug) printf("JFZ ");
			if (!f_zero)
			{
				Jump();
			}
			else
			{
				IncPC();
				imm = mem[stack[r_sp]];
				IncPC();
				dest = (mem[stack[r_sp]] << 8) + imm;
				IncPC();
				if (cpudebug) printf("%04X", dest);
			}
			break;
		case 0b01010000:
			if (cpudebug) printf("JFS ");
			if (!f_sign)
			{
				Jump();
			}
			else
			{
				IncPC();
				imm = mem[stack[r_sp]];
				IncPC();
				dest = (mem[stack[r_sp]] << 8) + imm;
				IncPC();
				if (cpudebug) printf("%03X", dest);
			}
			break;
		case 0b01011000:
			if (cpudebug) printf("JFP ");
			if (!f_parity)
			{
				Jump();
			}
			else
			{
				IncPC();
				imm = mem[stack[r_sp]];
				IncPC();
				dest = (mem[stack[r_sp]] << 8) + imm;
				IncPC();
				if (cpudebug) printf("%03X", dest);
			}
			break;
		//JTc
		case 0b01100000:
			if (cpudebug) printf("JTC ");
			if (f_carry)
			{
				Jump();
			}
			else
			{
				IncPC();
				imm = mem[stack[r_sp]];
				IncPC();
				dest = (mem[stack[r_sp]] << 8) + imm;
				IncPC();
				if (cpudebug) printf("%03X", dest);
			}
			break;
		case 0b01101000:
			if (cpudebug) printf("JTZ ");
			if (f_zero)
			{
				Jump();
			}
			else
			{
				IncPC();
				imm = mem[stack[r_sp]];
				IncPC();
				dest = (mem[stack[r_sp]] << 8) + imm;
				IncPC();
				if (cpudebug) printf("%03X", dest);
			}
			break;
		case 0b01110000:
			if (cpudebug) printf("JTS ");
			if (f_sign)
			{
				Jump();
			}
			else
			{
				IncPC();
				imm = mem[stack[r_sp]];
				IncPC();
				dest = (mem[stack[r_sp]] << 8) + imm;
				IncPC();
				if (cpudebug) printf("%03X", dest);
			}
			break;
		case 0b01111000:
			if (cpudebug) printf("JTP ");
			if (f_parity)
			{
				Jump();
			}
			else
			{
				IncPC();
				imm = mem[stack[r_sp]];
				IncPC();
				dest = (mem[stack[r_sp]] << 8) + imm;
				IncPC();
				if (cpudebug) printf("%03X", dest);
			}
			break;

		//CAL
		case 0b01000110:
		case 0b01001110:
		case 0b01010110:
		case 0b01011110:
		case 0b01100110:
		case 0b01101110:
		case 0b01110110:
		case 0b01111110:
			if (cpudebug) printf("CAL ");
			Call();
			break;

		//CFc
		case 0b01000010:
			if (cpudebug) printf("CFC ");
			if (!f_carry)
			{
				Call();
			}
			else
			{
				IncPC();
				imm = mem[stack[r_sp]];
				IncPC();
				dest = (mem[stack[r_sp]] << 8) + imm;
				IncPC();
				if (cpudebug) printf("%03X", dest);
			}
			break;
		case 0b01001010:
			if (cpudebug) printf("CFZ ");
			if (!f_zero)
			{
				Call();
			}
			else
			{
				IncPC();
				imm = mem[stack[r_sp]];
				IncPC();
				dest = (mem[stack[r_sp]] << 8) + imm;
				IncPC();
				if (cpudebug) printf("%03X", dest);
			}
			break;
		case 0b01010010:
			if (cpudebug) printf("CFS ");
			if (!f_sign)
			{
				Call();
			}
			else
			{
				IncPC();
				imm = mem[stack[r_sp]];
				IncPC();
				dest = (mem[stack[r_sp]] << 8) + imm;
				IncPC();
				if (cpudebug) printf("%03X", dest);
			}
			break;
		case 0b01011010:
			if (cpudebug) printf("CFP ");
			if (!f_parity)
			{
				Call();
			}
			else
			{
				IncPC();
				imm = mem[stack[r_sp]];
				IncPC();
				dest = (mem[stack[r_sp]] << 8) + imm;
				IncPC();
				if (cpudebug) printf("%03X", dest);
			}
			break;

		//CTc
		case 0b01100010:
			if (cpudebug) printf("CTC ");
			if (f_carry)
			{
				Call();
			}
			else
			{
				IncPC();
				imm = mem[stack[r_sp]];
				IncPC();
				dest = (mem[stack[r_sp]] << 8) + imm;
				IncPC();
				if (cpudebug) printf("%03X", dest);
			}
			break;
		case 0b01101010:
			if (cpudebug) printf("CTZ ");
			if (f_zero)
			{
				Call();
			}
			else
			{
				IncPC();
				imm = mem[stack[r_sp]];
				IncPC();
				dest = (mem[stack[r_sp]] << 8) + imm;
				IncPC();
				if (cpudebug) printf("%03X", dest);
			}
			break;
		case 0b01110010:
			if (cpudebug) printf("CTS ");
			if (f_sign)
			{
				Call();
			}
			else
			{
				IncPC();
				imm = mem[stack[r_sp]];
				IncPC();
				dest = (mem[stack[r_sp]] << 8) + imm;
				IncPC();
				if (cpudebug) printf("%03X", dest);
			}
			break;
		case 0b01111010:
			if (cpudebug) printf("CTP ");
			if (f_parity)
			{
				Call();
			}
			else
			{
				IncPC();
				imm = mem[stack[r_sp]];
				IncPC();
				dest = (mem[stack[r_sp]] << 8) + imm;
				IncPC();
				if (cpudebug) printf("%03X", dest);
			}
			break;

		//RET
		case 0b00000111:
		case 0b00001111:
		case 0b00010111:
		case 0b00011111:
		case 0b00100111:
		case 0b00101111:
		case 0b00110111:
		case 0b00111111:
			DecSP();
			if (cpudebug) printf("RET");
			break;

		//RFc
		case 0b00000011:
			if (!f_carry)
			{
				DecSP();
			}
			else
			{
				IncPC();
			}
			if (cpudebug) printf("RFC");
			break;
		case 0b00001011:
			if (!f_zero)
			{
				DecSP();
			}
			else
			{
				IncPC();
			}
			if (cpudebug) printf("RFZ");
			break;
		case 0b00010011:
			if (!f_sign) DecSP();
			if (cpudebug) printf("RFS");
			break;
		case 0b00011011:
			if (!f_parity) DecSP();
			if (cpudebug) printf("RFP");
			break;

		//RTc
		case 0b00100011:
			if (f_carry)
			{
				DecSP();
			}
			else
			{
				IncPC();
			}
			if(cpudebug) printf("RTC");
			break;
		case 0b00101011:
			if (f_zero)
			{
				DecSP();
			}
			else
			{
				IncPC();
			}
			if (cpudebug) printf("RTZ");
			break;
		case 0b00110011:
			if (f_sign)
			{
				DecSP();
			}
			else
			{
				IncPC();
			}
			if (cpudebug) printf("RTS");
			break;
		case 0b00111011:
			if (f_parity)
			{
				DecSP();
			}
			else
			{
				IncPC();
			}
			if (cpudebug) printf("RTP");
			break;

		//RST
		case 0b00000101:
		case 0b00001101:
		case 0b00010101:
		case 0b00011101:
		case 0b00100101:
		case 0b00101101:
		case 0b00110101:
		case 0b00111101:
			IncPC();
			IncSP();
			stack[r_sp] = instruction & 0b00111000;
			if (cpudebug) printf("RST %03X", (instruction & 0b00111000));
			break;

		//INP/OUTP
		case 0b01000001:
		case 0b01000011:
		case 0b01000101:
		case 0b01000111:
		case 0b01001001:
		case 0b01001011:
		case 0b01001101:
		case 0b01001111:
		case 0b01010001:
		case 0b01010011:
		case 0b01010101:
		case 0b01010111:
		case 0b01011001:
		case 0b01011011:
		case 0b01011101:
		case 0b01011111:
		case 0b01100001:
		case 0b01100011:
		case 0b01100101:
		case 0b01100111:
		case 0b01101001:
		case 0b01101011:
		case 0b01101101:
		case 0b01101111:
		case 0b01110001:
		case 0b01110011:
		case 0b01110101:
		case 0b01110111:
		case 0b01111001:
		case 0b01111011:
		case 0b01111101:
		case 0b01111111:
			RunIo((instruction & 0b00111110) >> 1);
			IncPC();
			break;

		//HLT
		case 0b00000000:
		case 0b00000001:
		case 0b11111111:
			printf("\r\n---HLT AT %03X---\r\n", stack[r_sp]);
			f_halt = true;
			IncPC();
			break;

		//invalid/unknown, skip
		default:
			printf("invalid instruction: %03o\r\n", instruction);
			IncPC();
			break;
	}

	if (cpudebug) printf("\r\n\r\n");
}

int statecount = 0;
bool singlestep = false;
bool step = false;

SDL_Mutex* cpulock;

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
	FILE* romfile;
	romfile = fopen("rom.bin", "rb");

	if (!romfile)
	{
		SDL_Log("Couldn't load ROM file.");
		return SDL_APP_FAILURE;
	}

	int result = fread((void*)(&(mem[rom_start])), 1, 2048, romfile);

	printf("Bytes read: %i\r\n", result);

	fclose(romfile);

	ResetCpu();

	/* Create the window */
	if (!SDL_CreateWindowAndRenderer("Hello World", 800, 600, SDL_WINDOW_RESIZABLE /*SDL_WINDOW_FULLSCREEN*/, &window, &renderer))
	{
		SDL_Log("Couldn't create window and renderer: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	cpulock = SDL_CreateMutex();

	if (cpulock == NULL)
	{
		SDL_Log("Failed to create mutex: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
	if (event->type == SDL_EVENT_QUIT)
	{
		return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
	}
	return SDL_APP_CONTINUE;
}

bool reentry = false;
int64_t lasttime = 0;
int64_t newtime = 0;
int timediff = 0;

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void* appstate)
{
	const char* message = "Hello World!";
	int w = 0, h = 0;
	float x, y;
	const float scale = 4.0f;

	/* Center the message and scale it up */
	SDL_GetCurrentRenderOutputSize(renderer, &w, &h);
	SDL_SetRenderScale(renderer, scale, scale);
	x = ((w / scale) - SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE * SDL_strlen(message)) / 2;
	y = ((h / scale) - SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE) / 2;

	/* Draw the message */
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_RenderDebugText(renderer, x, y, message);
	SDL_RenderPresent(renderer);

	if (!reentry)
	{
		reentry = true;

		newtime = SDL_GetTicks();
		timediff = (newtime - lasttime) * 700;
		lasttime = newtime;
		
		for (int i = 0; i < timediff; i++)
		{
			RunCpu();
		}
		reentry = false;
	}

	return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
	SDL_DestroyMutex(cpulock);
}