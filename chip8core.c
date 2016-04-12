#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int drawFlag = 0;
unsigned char *displayArray;
//####################################################################################################################################################################
struct chip8
{
	//4096 memory locations, each 8 bits long
	//So, 8 bits allow us to address a byte of data (CHIP8 is an 8 bit computer)
	//Might as well use char * for this
	unsigned char memory[4096];
	//16 8 bit data registers
	unsigned char V[16];
	//1 16-bit address register
	unsigned short I;
	//16 levels of stack (4 byte values)
	unsigned char stack[16];
	//stackPointer
	unsigned char stackPointer;
	//program counter
	unsigned short pc;
	//timers
	unsigned char delayTimer;
	unsigned char soundTimer;
	//input, hex keyboard of 16 keys
	unsigned char keypad[16];
	//display, resolution:64x32, all graphics are 8-bit long sprites. CHIP8 draws in XOR mode.
	unsigned char display[64*32];
};
//####################################################################################################################################################################
/*forward declarations */
struct chip8 *loadProgram	(char *filePath, struct chip8 *chip8core);
struct chip8 *cycle		(struct chip8 *chip8core);
struct chip8 *loadFontSet	(struct chip8 *chip8core);


//####################################################################################################################################################################
/* create the chip8 core*/
struct chip8 *createChip8()
{
	struct chip8 *chip8core = malloc(sizeof(struct chip8));
	/* Program counter effectively needs to start at addr 0x200,
	*  as 0x0 - 0x1FF is reserved for interpreter/fontset */
	chip8core->pc = 0x200;
	chip8core->stackPointer = 0;
	return chip8core;
}

//#######################################################################################
void destroyChip8(struct chip8 *chip8core)
{
	free(chip8core);
}
//####################################################################################################################################################################
/* load the fontset into memory starting at 0x50 */
struct chip8 *loadFontSet(struct chip8 *chip8core)
{
	int i;
	//fontset :
	unsigned char fontSet[80] = {
		0XF0, 0X90, 0X90, 0X90, 0xF0, 	//0
		0X20, 0X60, 0X20, 0X20, 0X70,	//1
		0XF0, 0X10, 0XF0, 0X80, 0XF0,	//2
		0XF0, 0X10, 0XF0, 0X10, 0XF0,	//3
		0X90, 0X90, 0XF0, 0X10, 0X10,	//4
		0XF0, 0X80, 0XF0, 0X10, 0XF0,	//5
		0XF0, 0X80, 0XF0, 0X90, 0XF0,	//6
		0XF0, 0X10, 0X20, 0X40, 0X40,	//7
		0XF0, 0X90, 0XF0, 0X90, 0XF0,	//8
		0XF0, 0X90, 0XF0, 0X10, 0XF0, 	//9
		0XF0, 0X90, 0XF0, 0X90, 0X90,	//A
		0XE0, 0X90, 0XE0, 0X90, 0XE0,	//B
		0XF0, 0X80, 0X80, 0X80, 0XF0, 	//C
		0XE0, 0X90, 0X90, 0X90, 0XE0,	//D
		0XF0, 0X80, 0XF0, 0X80, 0XF0,	//E
		0XF0, 0X80, 0XF0, 0X80, 0X80 	//F
	};

	for(i = 0 ; i < 80; i++) //fontSet.length = 80
	{
		/* CHIP8's fontset (sprites for characters 0-F, basically) starts at addr 0x50*/
		chip8core->memory[0x50 + i] = fontSet[i];
	}
	return chip8core;
}
//####################################################################################################################################################################
/* load ROM into chip8 memory */
struct chip8 *loadProgram(char *filePath, struct chip8 *chip8core)
{
	FILE *fileStream = fopen(filePath, "r"); //open file from filepath in binary, read only mode
	//char *buffer;
	if(!fileStream)
	{
		//Very basic error handling for a failed open
		printf("Failed to load ROM \"%s\"\n",filePath);
		exit(0);
	}


	/*
	fread usage : size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);

			- reads  "nmemb"  elements of data,
			- each "size" bytes long
       	 	- from the stream pointed to by "stream",
        	- storing them in memory beginning at the memaddr given by "ptr".
	*/
	//Read file in binary mode into Chip8 memory, starting at 0x200
	fread((chip8core->memory + 0x200),
		//We read 1 byte (sizeof unsigned char = 1) at a time...
		sizeof(unsigned char),
		//We read (4096 - 0x200) data elements in all. Again, this is because the CHIP8 interpreter has 0x0-0x1ff reserved.
		(4096 - 0x200),
		//and we read from the specified stream fileStream.
		fileStream);


	//Copy buffer over to CHIP8's memory.
	//CHIP 8 has memory locations 0x0 - 0x1FF reserved for interpreter and fontset
	//Thus, ROM data is stored in memory starting at 0x200.
	//if using char *buffer, uncomment this :
	/*for(i = 0 ; i < (int)strlen(buffer) ; i++)
	{
		chip8core->memory[0x200 + i] = buffer[i];
	}*/

	free(fileStream);

	return chip8core;
}
//####################################################################################################################################################################
/* a cycle of the emulator */
struct chip8 *cycle(struct chip8 *chip8core)
{
	//sizeof(short) = 2 bytes. So are our opcodes.
	unsigned short opcode = (chip8core->memory[chip8core->pc] << 8)|chip8core->memory[chip8core->pc + 1];
	/* register indices, memory addresses, and data, present in opcodes */
	unsigned int x = (opcode & 0x0f00) >> 8;
	unsigned int y;
	unsigned int nnn = (opcode & 0xfff);
	unsigned int nn = (opcode & 0xff);
	/* standard loop counter */
	int i;
	/* vars required for opcode 0xCXNN */
	int rndnum;
	/* vars required for opcode 0xDXYN */
	int xcoord;
	int ycoord;
	int n;
	int finalX;
	int finalY;
	int finalIndex = 0;
	int xcounter;
	int ycounter;
	char pixelData;
	/* vars required for opcode 0xFX33 */
	int hundreds;
	int tens;
	int ones;

	/*print opcode for easier debugging */
	printf("0x%X : ", opcode);//format this to a hex
	//printf("\nDEBUG:[current PC value : %d]",chip8core->pc);
	//exit(0);

	/* switchcase first nibble to find out which kind of opcode it is */
	switch (opcode & 0xf000)
	{
		case 0x0000:		//instructions beginning with a hex 0
			/*switchcase last two nibbles to find out which */
			switch(opcode & 0xff)
			{
				case 0xE0:	//00E0 clear screen
				printf("CLR\n");
				for(i = 0; i < (64*32) ; i++)
				{
					chip8core->display[i] = 0;
				}
				chip8core->pc+=2;//increment by two, as with each opcode we've seen 2 instructions
				break;

				case 0xee: //return from subroutine
				printf("Return from subroutine\n");
				chip8core->stackPointer--;
				chip8core->pc = chip8core->stack[chip8core->stackPointer] + 2;
				break;
			}

		break; //end of case 0x0000

		case 0x1000:	//1NNN Jump to address nnn
		printf("JMP %d\n", nnn);
		//set pc to nnn
		chip8core->pc = nnn;
		break;

		case 0x2000:	//2NNN call subroutine at NNN
		printf("call subroutine at %d\n", nnn);
		//save current pc in stack
		chip8core->stackPointer++;
		chip8core->stack[chip8core->stackPointer] = chip8core->pc;
		//set pc to nnn
		chip8core->pc = nnn;
		break;

		case 0x3000:	//3XNN : SKip next ins. if VX == nn
		printf("SE V%d, %d\n",x, nn);
		if(chip8core->V[x] == nn )
			chip8core->pc+=4;
		else
			chip8core->pc+=2;
		break;

		case 0x4000:	//4XNN : opposite of above
		printf("SNE V%d, %d\n", x, nn);
		if(chip8core->V[x] != nn)
			chip8core->pc+=4;
		else
			chip8core->pc+=2;
		break;

		case 0x5000:	//5XY0 : SE Vx, Vy
		y = (opcode & 0x00f0) >> 4;
		printf("SE V%d, V%d\n", x, y);
		if(chip8core->V[x] == chip8core->V[y])
			chip8core->pc+=4;
		else
			chip8core->pc+=2;
		break;

		case 0x6000:	//6XNN : LD Vx, nn
		printf("LD V%d, %d\n", x, nn);
		chip8core->V[x] = nn;
		chip8core->pc+=2;
		break;

		case 0x7000:	//7XNN : ADD Vx, nn
		printf("ADD V%d, %d\n", x, nn);
		chip8core->V[x] += nn;
		chip8core->V[x] &= 0xff;	//account for overflow
		chip8core->pc+=2;
		break;

		case 0x8000:	//Instructions beghinning with hex 8
			y = (opcode & 0x00f0) >> 4;
			switch(opcode & 0xf)
			{
				case 0x0:	//8XY0 : LD Vx, Vy
				printf("LD V%d, V%d\n", x, y);
				chip8core->V[x] = chip8core->V[y];
				chip8core->pc+=2;
				break;

				case 0x1:	//8XY1	:	VX = VX | VY
				printf("OR V%d, V%d\n", x, y);
				chip8core->V[x] = chip8core->V[x] | chip8core->V[y];
				chip8core->V[x] &= 0xff;	//account for overflow
				chip8core->pc+=2;
				break;

				case 0x2:	//8XY2	:	VX = VX % VY
				printf("AND V%d, V%d\n", x, y);
				chip8core->V[x] &= chip8core->V[y];
				chip8core->pc+=2;
				break;

				case 0x3:	//8xy3	:	vx = vx ^ vy
				printf("XOR V%d, V%d\n", x, y);
				chip8core->V[x] ^= chip8core->V[y];
				chip8core->V[x] &= 0xff;	//account for overflow
				chip8core->pc+=2;
				break;

				case 0x4:	//8xy4	:	vx = vx + vy, set vf if carry
				printf("ADD V%d, V%d\n", x , y);
				if(chip8core->V[x] + chip8core->V[y] > 0xff)
					chip8core->V[0xf] = 1;
				else
					chip8core->V[0xf] = 0;
				chip8core->V[x] += chip8core->V[y];
				chip8core->V[x] &= 0xff;	//account for overflow
				chip8core->pc+=2;
				break;

				case 0x5:	//8xy5	:	Vx = vx - vy, set NOT vf if borrow
				printf("SUB V%d, V%d\n", x, y);
				if(chip8core->V[y] > chip8core->V[x])
					chip8core->V[0xf] = 0;
				else
					chip8core->V[0xf] = 1;
				chip8core->V[x] -= chip8core->V[y];
				chip8core->pc+=2;
				break;

				case 0x6:	//8xy6	:	vx = vx >> 1, vf = MSB before shift
				printf("SHR V%d\n", x);
				chip8core->V[0xf] = chip8core->V[x] & 0x80; // AND with 10000000 to get MSB
				chip8core->V[x] = chip8core->V[x] >> 1;
				chip8core->pc+=2;
				break;

				case 0x7:	//8xy7	:	vx = vy - vx, set NOT vf if borrow
				printf("Vx = Vy - Vx\n"); //TODO: PROPER MNEMONIC
				if(chip8core->V[x] > chip8core->V[y])
					chip8core->V[0xf] = 0;
				else
					chip8core->V[0xf] = 1;
				chip8core->V[x] = chip8core->V[y] - chip8core->V[x];
				chip8core->pc+=2;
				break;

				case 0xe:	//8XYE :	VX = VX << 1, VF = LSB before shift
				printf("SHL V%d\n", x);
				chip8core->V[0xf] = chip8core->V[x] & 0x1; // AND with 00000001 to get lsb
				chip8core->V[x] = chip8core->V[x] << 1;
				chip8core->pc+=2;
				break;

				default:
				printf("Unsupported opcode\n");
				break;
			}
		break;	//End of 0x8000

		case 0x9000:	// 9XY0	:	SNE Vx, Vy
		y = (opcode & 0x00f0) >> 4;
		printf("SNE V%d, V%d\n", x,y);
		if(chip8core->V[x] != chip8core->V[y])
			chip8core->pc+=4;
		else
			chip8core->pc+=2;
		break;

		case 0xA000:	//ANNN : LD I, nnn
		printf("LD I, %d\n",nnn);
		chip8core->I = nnn;
		chip8core->pc+=2;

		break;

		case 0xB000:	//BNNN	:	JMP nnn (v0+nnn)
		printf("JMP V0, %d\n", nnn);
		char addr = (nnn + (chip8core->V[0x0] & 0xff));
		chip8core->pc = addr;
		break;

		case 0xC000:	//CXNN	:	VX = Rand() & nn
		printf("JMP V0, rand\n");
		rndnum = rand() & 0xff;	//truncate to 8 bits.
		chip8core->V[x] = rndnum & nn;
		chip8core->pc+=2;
		break;

		case 0xD000:	//DXYN	:	Draw a pixel at coord(Vx,Vy) pixel of height N
		y = (opcode & 0x00f0) >> 4;
		xcoord = chip8core->V[x];		//xcoord//
			//for debug
			//printf("xcoord calculated okay. value > V%d = %d\n", x, xcoord);
		ycoord = chip8core->V[y];		//ycoord
			//for debug
			//printf("ycoord calculated okay. value > V%d = %d\n", y, ycoord);
		n = opcode & 0xf;	//pixel height N


		printf("DRAW VX, VY, %d\n", n);

		chip8core->V[0xf] = 0;	//initialize collision flag to 0

		for(ycounter = 0; ycounter < n ; ycounter++)
		{
			pixelData = chip8core->memory[chip8core->I + ycounter];
			for(xcounter = 0; xcounter < 8 ; xcounter++)	//pixels are 8 bits in width
			{
				/*grab each bit of pixelData, one at a time, left to right*/
				if(pixelData &(0x80 >> xcounter) != 0)	//If this is hard to understand, bear in mind 0x80 = 1000 0000, and that solitary 1 shifts right by one each time
				{
					finalX = (xcoord + xcounter);
					finalY = (ycoord + ycounter);
					finalX %= 64;//wraparound
					finalY %= 32;//wraparound
					finalIndex = finalY*64 + finalX;
					//draw a pixel at finalindex, set vf = 1 if collision
					if(chip8core->display[finalIndex] == 1) //collision detected
						chip8core->V[0xf] = 1;
					chip8core->display[finalIndex] ^= 1;	//xor mode drawing
				}
			}//end xcounter (column) loop
		}//end ycounter (row)
		chip8core->pc+=2;
		drawFlag = 1;
		break;

		case 0xE000:	//instructions beginning with hex E
			switch(opcode & 0xf)
			{

				case 0xe:	//EX9E	:	Skip next instruction if key stored in VX pressed
				printf("SKip next instruction if keypad[V[%d]] pressed\n", x);
				if(chip8core->keypad[chip8core->V[x]] != 0)
					chip8core->pc+=4;
				else
					chip8core->pc+=2;
				break;

				case 0x1:	//EXA1	: Skip next instruction if key stored in VX isnt pressed
				printf("SKip next instruction if keypad[V[%d]] IS NOT pressed\n", x);
				if(chip8core->keypad[chip8core->V[x]] != 0)
					chip8core->pc+=2;
				else
					chip8core->pc+=4;
				break;

				default:
				printf("Unsupported opcode\n");
				break;
			}
		break;

		case 0xF000:	//instructions beginning with hex F
			switch(opcode & 0xff)//last 2 nibbles because i feel more comfortable with this
			{
				case 0x07:	//FX07 LD Vx, DT
				printf("LD V%d, dT\n", x);
				chip8core->V[x] = chip8core->delayTimer;
				chip8core->pc+=2;
				break;

				case 0x0a:	//keypress awaited, then stored in vx
				printf("await keypress to be stored in V%d\n", x);
				for(i = 0 ; i < 16 ; i++)
				{
					if(chip8core->keypad[i] != 0)
						chip8core->V[x] = chip8core->keypad[i];
				}
				chip8core->pc+=2;
				break;

				case 0x15:	//LD dT, VX
				printf("LD dT, V%d\n", x);
				chip8core->delayTimer = chip8core->V[x];
				chip8core->pc+=2;
				break;

				case 0x18:	//LD sT, VX
				printf("LD sT, V%d\n", x);
				chip8core->soundTimer = chip8core->V[x];
				chip8core->pc+=2;
				break;

				case 0x1e:	// I = I + VX
				printf("LD I, V%d\n", x);
				chip8core->I = chip8core->I + chip8core->V[x];
				chip8core->I &= 0xff;	//account for overflow
				break;

				case 0x29:	// I = location of sprite for char in Vx
				printf("Set I to location of sprite for char in V%d\n", (int)x);
				chip8core->I = (0x50 + (chip8core->V[x]) * 5);//fonts are 5 bits in width
				chip8core->pc+=2;
				break;

				case 0x33:	//store BCD representation of VX in mem loc. i, i+1, i+2
				printf("Store BCD representation of V%d in mem loc starting at I\n", (int)x);
				hundreds = (chip8core->V[x]) / 100;
				tens = (chip8core->V[x]%100) / 10;
				ones = (chip8core->V[x]%100) % 10;

				chip8core->memory[chip8core->I] = hundreds;
				chip8core->memory[chip8core->I + 1] = tens;
				chip8core->memory[chip8core->I + 2] = ones;

				chip8core->pc+=2;
				break;

				case 0x55:	// store VO to VX in mem[I] onwards
				printf("Store V0 to VX in mem[I] onwards\n");
				for(i = 0 ; i < x ; i++)
				{
					chip8core->memory[chip8core->I + i] = chip8core->V[x];
				}
				chip8core->pc+=2;
				break;

				case 0x65:	//load V0 to VX with contents of memory, from memory[I] onwards.
				printf("Load VO to VX With mem[I] onwards\n");
				for(i = 0; i < x ; i++)
				{
					chip8core->V[i] = chip8core->memory[chip8core->I + i];
				}
				chip8core->pc+=2;
				break;

				default:	//formality
				printf("Unsupported opcode");
				break;
			}
		break;	//end of 0xF000

		default:
			printf("Unsupported opcode.\n");
		break;
	}

	/* timers have to be updated at the end of every CPU cycle */
	if(chip8core->delayTimer > 0)
		chip8core->delayTimer--;
	if(chip8core->soundTimer > 0)
		chip8core->soundTimer--;

	return chip8core;
}

//####################################################################################################################################################################
/* main function*/
int main(int argc, char *argv[])
{
	/* first off, create the chip8 core */
	struct chip8 *chip8core = createChip8();
	/* Get path to ROM specified in command line argument */
	char filePath[40];
	sprintf(filePath, "C:\\Users\\PRITH\\workspace\\CHIP8C\\Default\\%s", argv[1]);
	//the two lines above ^ comprise a horrible way of going about it. No way of knowing buffer (filePath) size.
	//char *filePath = ("C:\\Users\\PRITH\\workspace\\CHIP8C\\Default\\zerodemo.ch8"); <--If hardcoded

	//printf("\nDEBUG_filePath = %s\n",filePath);
	/*load the specified program into CHIP8 memory */
	chip8core = loadProgram(filePath, chip8core);

	/*
		global variable *displayArray = &(chip8core->display)
		and then use *displayArray in the glDisplayFunc.
	displayArray = &(chip8core->display[0]);
	*/

	/* core game loop, infinite */
	for(;;)
		{
			/*invoke a cycle of the emulator*/
			chip8core = cycle(chip8core);
			/*insert openGL rendering loop here */
		}
	return 0;
}
//####################################################################################################################################################################

