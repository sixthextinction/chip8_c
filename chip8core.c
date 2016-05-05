#include <stdio.h>
/*#ifdef WINDOWS*/
#include <direct.h>
#define GetCurrentDir _getcwd
//#else
//#include <unistd.h>
//#define GetCurrentDir getcwd

#include <stdlib.h>
#include <GL/glut.h>
#include <string.h>
#include <math.h>

/* Global variables */
int drawFlag = 0;
int windowID = 0;
/* Forward declarations */
void loadProgram(char *filePath);
void cycle();
void loadFontSet();

//####################################################################################################################################################################
struct chip8
{
	//4096 memory locations, each 8 bits long
	//So, 8 bits allow us to address a byte of data ("CHIP8 is an 8 bit computer.")
	//So we might as well use a char array for this
	unsigned char memory[4096];
	//16 8 bit data registers
	unsigned char V[16];
	//1 16-bit address register (of which only 12 bits are ever used, really)
	unsigned short I;
	//16 levels of stack (4 byte values)
	unsigned int stack[16];//this needed to be int instead of char(why was it char again?)
	//stackPointer
	unsigned int stackPointer;
	//program counter
	unsigned short pc;
	//timers which count down at 60hz
	unsigned char delayTimer;
	unsigned char soundTimer;
	//input, hex keyboard of 16 keys
	unsigned char keypad[16];
	//display, resolution:64x32, all graphics are 8-bit long sprites. CHIP8 draws in XOR mode.
	unsigned char display[64 * 32];
};
//####################################################################################################################################################################
//static init of struct
struct chip8 chip8core = { .pc = 0x200 };
//####################################################################################################################################################################
void createChip8()
{
	int i = 0;
	struct chip8 chip8core = { .pc = 0x200,.stackPointer = 0x0,.I = 0 };
	//clear display
	for (i = 0; i < (64 * 32); i++)
	{
		chip8core.display[i] = 0;
	}
	//clear memory
	for(i = 0 ; i< 4096 ; i++)
	{
		chip8core.memory[i] = 0;
	}
	//clear stack
	for (i = 0; i < 16; i++)
	{
		chip8core.stack[i] = 0;
	}
	//reset timers
	chip8core.delayTimer = 0;
	chip8core.soundTimer = 0;
}
//####################################################################################################################################################################
/* load the fontset into memory starting at 0x50 */
void loadFontSet()
{
	int i;
	//fontset, as sprites:
	unsigned char fontSet[80] = {
		0xF0, 0x90, 0x90, 0x90, 0xF0, //0
		0x20, 0x60, 0x20, 0x20, 0x70, //1
		0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
		0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
		0x90, 0x90, 0xF0, 0x10, 0x10, //4
		0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
		0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
		0xF0, 0x10, 0x20, 0x40, 0x40, //7
		0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
		0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
		0xF0, 0x90, 0xF0, 0x90, 0x90, //A
		0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
		0xF0, 0x80, 0x80, 0x80, 0xF0, //C
		0xE0, 0x90, 0x90, 0x90, 0xE0, //D
		0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
		0xF0, 0x80, 0xF0, 0x80, 0x80  //F
	};

	for (i = 0; i < 80; ++i) //fontSet.length = 80
	{
		/* CHIP8's fontset (sprites for characters 0-F, basically) starts at addr 0x50*/
		chip8core.memory[i + 0x50] = fontSet[i];
	}
}
//####################################################################################################################################################################
/* load ROM into chip8 memory */
void loadProgram(char *filePath)
{
	FILE *fileStream = fopen(filePath, "rb"); //open file from filepath in binary, read only mode
	if (!fileStream)
	{
		//Very basic error handling for a failed open
		printf("Failed to load ROM \"%s\"\n", filePath);
		exit(0);
	}


	//Read file in binary mode into Chip8 memory, starting at 0x200
	fread((chip8core.memory + 0x200),
		//We read 1 byte (sizeof unsigned char = 1) at a time...
		sizeof(unsigned char),
		//We read (4096 - 0x200) data elements in all. Again, this is because the CHIP8 interpreter has 0x0-0x1ff reserved.
		(4096 - 0x200),
		//and we read from the specified stream fileStream.
		fileStream);

	free(fileStream);
}
//####################################################################################################################################################################
/* a cycle of the emulator */
void cycle()
{
	//sizeof(short) = 2 bytes. So are our opcodes. How convenient.
	unsigned short opcode = (chip8core.memory[chip8core.pc] << 8) | chip8core.memory[chip8core.pc + 1];

	/* debug */
	//printf("I = %d, memory[I] = %d, PC = %d\n", chip8core.I, chip8core.memory[chip8core.I],chip8core.pc);
	
	/* Local Variables*/
	//register indices, memory addresses, and data, present in opcodes
	unsigned int x = (opcode & 0x0f00) >> 8;
	unsigned int y;
	unsigned int nnn = (opcode & 0xfff);
	unsigned int nn = (opcode & 0xff);
	char addr;
	//standard loop counter
	int i;
	// vars required for opcode 0xCXNN
	int rndnum;
	// vars required for opcode 0xDXYN 
	unsigned short xcoord;
	unsigned short ycoord;
	unsigned short n;
	int finalX;
	int finalY;
	int finalIndex = 0;
	int xcounter;
	int ycounter;
	char pixelData;
	//vars required for opcode 0xFX33
	int hundreds;
	int tens;
	int ones;

	/*print opcode for easier debugging */
	printf("0x%X : ", opcode);//format this to a hex

	/* switchcase first nibble to find out which kind of opcode it is */
	switch (opcode & 0xf000)
	{
	case 0x0000:		//instructions beginning with a hex 0
		/*switchcase last two nibbles to find out which */
		switch (opcode & 0xff)
		{
		case 0xE0:	//00E0 clear screen
			printf("CLR\n");
			for (i = 0; i < (64 * 32); i++)
			{
				chip8core.display[i] = 0x0;
			}
			chip8core.pc += 2;//increment by two because with each opcode we've actually seen 2 instructions
			break;

		case 0xEE: //return from subroutine
			printf("Return from subroutine\n");
			--chip8core.stackPointer;
			chip8core.pc = (int)chip8core.stack[chip8core.stackPointer];
			chip8core.pc += 2;
			break;
		}

		break; //end of case 0x0000

	case 0x1000:	//1NNN Jump to address nnn
		printf("JMP %d\n", nnn);
		//set pc to nnn
		chip8core.pc = nnn;
		break;

	case 0x2000:	//2NNN call subroutine at NNN
		printf("CALL %d\n", nnn);
		//save current pc in stack
		chip8core.stack[chip8core.stackPointer++] = (int)chip8core.pc;
		//set pc to nnn
		chip8core.pc = nnn;
		break;

	case 0x3000:	//3XNN : Skip next ins. if VX == nn
		printf("SE V%d, %d\n", x, nn);
		if (chip8core.V[x] == nn)
			chip8core.pc += 4;
		else
			chip8core.pc += 2;
		break;

	case 0x4000:	//4XNN : Skip next ins. if VX != nn
		printf("SNE V%d, %d\n", x, nn);
		if (chip8core.V[x] != nn)
			chip8core.pc += 4;
		else
			chip8core.pc += 2;
		break;

	case 0x5000:	//5XY0 : SE Vx, Vy
		y = (opcode & 0x00f0) >> 4;
		printf("SE V%d, V%d\n", x, y);
		if (chip8core.V[x] == chip8core.V[y])
			chip8core.pc += 4;
		else
			chip8core.pc += 2;
		break;

	case 0x6000:	//6XNN : LD Vx, nn
		printf("LD V%d, %d\n", x, nn);
		chip8core.V[x] = nn;
		chip8core.pc += 2;
		break;

	case 0x7000:	//7XNN : ADD Vx, nn
		printf("ADD V%d, %d\n", x, nn);
		chip8core.V[x] += nn;
		chip8core.V[x] &= 0xff;	//account for overflow
		chip8core.pc += 2;
		break;

	case 0x8000:	//Instructions beghinning with hex 8
		y = (opcode & 0x00f0) >> 4;
		switch (opcode & 0xf)
		{
		case 0x0:	//8XY0 : LD Vx, Vy
			printf("LD V%d, V%d\n", x, y);
			chip8core.V[x] = chip8core.V[y];
			chip8core.pc += 2;
			break;

		case 0x1:	//8XY1	:	VX = VX | VY
			printf("OR V%d, V%d\n", x, y);
			chip8core.V[x] = chip8core.V[x] | chip8core.V[y];
			chip8core.pc += 2;
			break;

		case 0x2:	//8XY2	:	VX = VX % VY
			printf("AND V%d, V%d\n", x, y);
			chip8core.V[x] &= chip8core.V[y];
			chip8core.pc += 2;
			break;

		case 0x3:	//8xy3	:	vx = vx ^ vy
			printf("XOR V%d, V%d\n", x, y);
			chip8core.V[x] ^= chip8core.V[y];
			chip8core.pc += 2;
			break;

		case 0x4:	//8xy4	:	vx = vx + vy, set vf if carry
			printf("ADD V%d, V%d\n", x, y);
			if (chip8core.V[x] + chip8core.V[y] > 0xff)
				chip8core.V[0xf] = 1;
			else
				chip8core.V[0xf] = 0;
			chip8core.V[x] += chip8core.V[y];
			chip8core.V[x] &= 0xff;	//account for overflow
			chip8core.pc += 2;
			break;

		case 0x5:	//8xy5	:	Vx = vx - vy, set NOT vf if borrow
			printf("SUB V%d, V%d\n", x, y);
			if (chip8core.V[y] > chip8core.V[x])
				chip8core.V[0xf] = 0;
			else
				chip8core.V[0xf] = 1;
			chip8core.V[x] -= chip8core.V[y];
			chip8core.pc += 2;
			break;

		case 0x6:	//8xy6	:	vx = vx >> 1, vf = MSB before shift
			printf("SHR V%d\n", x);
			chip8core.V[0xf] = chip8core.V[x] & 0x80; // AND with 10000000 to get MSB
			chip8core.V[x] = chip8core.V[x] >> 1;
			chip8core.pc += 2;
			break;

		case 0x7:	//8xy7	:	vx = vy - vx, set NOT vf if borrow
			printf("SUBN V%d, V%d\n", x, y);
			if (chip8core.V[x] > chip8core.V[y])
				chip8core.V[0xf] = 0;
			else
				chip8core.V[0xf] = 1;
			chip8core.V[x] = chip8core.V[y] - chip8core.V[x];
			chip8core.pc += 2;
			break;

		case 0xe:	//8XYE :	VX = VX << 1, VF = LSB before shift
			printf("SHL V%d\n", x);
			chip8core.V[0xf] = chip8core.V[x] & 0x1; // AND with 00000001 to get lsb
			chip8core.V[x] = chip8core.V[x] << 1;

			chip8core.pc += 2;
			break;

		default:
			printf("Unsupported opcode\n");
			break;
		}
		break;	//End of 0x8000

	case 0x9000:	// 9XY0	:	SNE Vx, Vy
		y = (opcode & 0x00f0) >> 4;
		printf("SNE V%d, V%d\n", x, y);
		if (chip8core.V[x] != chip8core.V[y])
			chip8core.pc += 4;
		else
			chip8core.pc += 2;
		break;

	case 0xA000:	//ANNN : LD I, nnn
		printf("LD I, %d\n", nnn);
		chip8core.I = nnn;
		chip8core.pc += 2;

		break;

	case 0xB000:	//BNNN	:	JMP nnn (v0+nnn)
		printf("JMP V0, %d\n", nnn);
		addr = (nnn + (chip8core.V[0x0] & 0xff));
		chip8core.pc = addr;
		break;

	case 0xC000:	//CXNN	:	VX = Rand() & nn
		printf("JMP V0, rand\n");
		rndnum = rand() & 0xff;	//truncate to 8 bits.
		chip8core.V[x] = rndnum & nn;
		chip8core.pc += 2;
		break;

	case 0xD000:	//DXYN	:	Draw a pixel at coord(Vx,Vy) pixel of height N
		y = (opcode & 0x00f0) >> 4;
		xcoord = (unsigned short)chip8core.V[x];	//xcoord
		ycoord = (unsigned short)chip8core.V[y];	//ycoord
		n = opcode & 0xf;							//pixel height N


		printf("DRAW VX, VY, %d\n", n);

		chip8core.V[0xf] = 0;	//initialize collision flag to 0

		for (ycounter = 0; ycounter < n; ycounter++)
		{
			pixelData = chip8core.memory[chip8core.I + ycounter];
			for (xcounter = 0; xcounter < 8; xcounter++)	//pixels are 8 bits in width
			{
				/*grab each bit of pixelData, one at a time, left to right*/
				if (pixelData &(0x80 >> xcounter))	//Isolate each bit of pixelData, one at a time. If this is hard to understand, bear in mind 0x80 = 1000 0000, and that solitary 1 shifts right by one each time.
				{
					finalX = (xcoord + xcounter);
					finalY = (ycoord + ycounter);
					finalIndex = ((finalY * 64) + finalX);
					//draw a pixel at finalindex, set vf = 1 if collision
					if (chip8core.display[finalIndex] == 1) //collision detected
					{
						chip8core.V[0xf] = 1;
					}

					chip8core.display[finalIndex] ^= 1;	//xor mode drawing
				}
			}//end xcounter (column) loop
		}//end ycounter (row)
		drawFlag = 1;
		chip8core.pc += 2;
		break;

	case 0xE000:	//instructions beginning with hex E
		switch (opcode & 0xf)
		{

		case 0xe:	//EX9E	:	Skip next instruction if key stored in VX pressed
			printf("SKip next instruction if keypad[V[%d]] pressed\n", x);
			if (chip8core.keypad[chip8core.V[x]] != 0)
				chip8core.pc += 4;
			else
				chip8core.pc += 2;
			break;

		case 0x1:	//EXA1	: Skip next instruction if key stored in VX isnt pressed
			printf("SKip next instruction if keypad[V[%d]] IS NOT pressed\n", x);
			if (chip8core.keypad[chip8core.V[x]] != 0)
				chip8core.pc += 2;
			else
				chip8core.pc += 4;
			break;

		default:
			printf("Unsupported opcode\n");
			break;
		}
		break;

	case 0xF000:	//instructions beginning with hex F
		switch (opcode & 0xff)//switch last 2 nibbles even though we need just 1, personal preference.
		{
		case 0x07:	//FX07 : LD Vx, DT
			printf("LD V%d, dT\n", x);
			chip8core.V[x] = chip8core.delayTimer;
			chip8core.pc += 2;
			break;

		case 0x0a:	//FX0A : Keypress awaited, then stored in vx
			printf("LD V%d : Awaiting keypress to be stored in V%d\n", x, x);
			for (i = 0; i < 16; i++)
			{
				if (chip8core.keypad[i] == 1)
				{
					//chip8core.V[x] = (char)chip8core.keypad[i];	//<--found the problem. This line of code will only store a 1 or 0, not the key itself.
					chip8core.V[x] = (char)i;						//<--Solution. This gets the actual key (hex value)
					chip8core.pc += 2;
					break;
				}
			}
			break;

		case 0x15:	//FX15	:	LD dT, VX
			printf("LD dT, V%d\n", x);
			chip8core.delayTimer = chip8core.V[x];
			chip8core.pc += 2;
			break;

		case 0x18:	//FX18	:	LD sT, VX
			printf("LD sT, V%d\n", x);
			chip8core.soundTimer = chip8core.V[x];
			chip8core.pc += 2;
			break;

		case 0x1e:	//FX1E	:	I = I + VX
			printf("ADD I, V%d\n", x);
			if ((chip8core.I + chip8core.V[x]) > 0xFFF)//range overflow for I
			{
				chip8core.V[0xF] = 1;
			}
			else
			{
				chip8core.V[0xF] = 0;
			}
			chip8core.I += chip8core.V[x];

			chip8core.pc += 2;
			break;

		case 0x29:	// FX29	:	I = location of sprite for char in Vx
			printf("LD F, V%d : Set I to location of sprite for char in V%d\n", (int)x, (int)x);
			chip8core.I = (0x50 + (chip8core.V[x]) * 5);//font sprites are 5 bits in width
			chip8core.pc += 2;
			break;

		case 0x33:	//FX33	:	store BCD representation of VX in mem loc. i, i+1, i+2
			printf("LD B, V%d : Store BCD representation of V%d in mem loc starting at I\n", (int)x, (int)x);
			hundreds = (chip8core.V[x]) / 100;
			tens = (chip8core.V[x] % 100) / 10;
			ones = (chip8core.V[x] % 100) % 10;

			chip8core.memory[chip8core.I] = hundreds;
			chip8core.memory[chip8core.I + 1] = tens;
			chip8core.memory[chip8core.I + 2] = ones;

			chip8core.pc += 2;
			break;

		case 0x55:	// FX55	:	store VO to VX in mem[I] onwards
			printf("LD [I], V%d : Store V0 to V%d in mem[I] onwards\n", x, x);
			for (i = 0; i <= x; i++)
			{
				//printf("___DEBUG. Storing V[%d] ( = %d) in memory[%d].\n", i, chip8core.V[i], chip8core.I + i);
				chip8core.memory[chip8core.I + i] = chip8core.V[i];
				//printf("__DEBUG.  memory[%d] now holds %d\n", chip8core.I + i, chip8core.memory[chip8core.I + i]);
			}
			//chip8core.I += (int)((opcode & 0x0F00) >> 8) + 1;
			chip8core.pc += 2;
			break;

		case 0x65:	//FX65	:	load V0 to VX with contents of memory, from memory[I] onwards.
			printf("LD V%d, [I] : Load VO to V%d With mem[I] onwards\n", x, x);
			for (i = 0; i <= x; i++)
			{
				chip8core.V[i] = chip8core.memory[chip8core.I + i];
			}

			chip8core.I += ((opcode & 0x0F00) >> 8) + 1;
			chip8core.pc += 2;
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
	if (chip8core.delayTimer > 0)
		chip8core.delayTimer--;
	if (chip8core.soundTimer > 0)
		chip8core.soundTimer--;

}
//########################################################################################################################################################
void renderScene(void)
{
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(0.0, 0.0, 0.0, 0.0); //unnecessary for our use case

	glMatrixMode(GL_PROJECTION);
	/* Reset view, clear prior projections */
	glLoadIdentity();
	/*Set up an orthographic projection (Fancy way to say "don't render pixels outside these bounds", basically.)
	Params are in the order : (left, right, bottom, top, zNear, zFar);
	*/
	glOrtho(0, 1.0, 1.0, 0, 1, -1);

	glMatrixMode(GL_MODELVIEW);

	/* upscale CHIP8's 64x32 display to 640x320 for better visibility on modern displays*/
	glPointSize(10.0);


	/* call a cycle of the cpu, for internal data updation*/
	cycle();

	/* vars required for rendering*/
	int i = 0;
	int x, y;
	float xfloat, yfloat;

	/* render loop */
	for (i = 0; i < (64 * 32); i++)
	{
		if (chip8core.display[i] != 0)
		{
			/* set drawing color = white */
			glColor3f(1.0, 1.0, 1.0);
		}
		else
		{
			/* set drawing color = black */
			glColor3f(0.0, 0.0, 0.0);
		}

		/* handle wraparounds*/
		x = (int)i % 64;
		y = (int)floor(i / 64);

		/* multiply by 10 for upscaling, divide by width (for x) or height (for y) to convert final coords to a range from 0.0 to 1.0 */
		xfloat = (float)(x * 10) / 640;
		yfloat = (float)(y * 10) / 320;

		/* start gl rendering stuff*/
		glBegin(GL_POINTS);
			glVertex3f(xfloat, yfloat, 0.0);
		glEnd();
		/* end gl rendering stuff*/

	}//end render loop

	 /* swap back and front buffers, as our glWindow is double buffered*/
	glutSwapBuffers();

	/* this is for single buffered displays, and needs opengl32.lib
	glFLush();
	*/

	/* ask for the window to be updated. Alternatively, have this function as glutIdleFunc()*/
	//glutPostRedisplay();
}
//####################################################################################################################################################################
void handleKeyPress(unsigned char key, int x, int y)
{
	/*
	* Implement keypad mapping
	* If corresponding key pressed, set keypad[key] to 1
	* else, if corresponding key released, set keypad[key] to 0
	*
	* Mapping
	* --------
	* ORIGINAL:
	* 1 2 3 C
	* 4 5 6 D
	* 7 8 9 E
	* A 0 B F
	*
	* MAPPED:
	* 1 2 3 4
	* Q W E R
	* A S D F
	* Z X C V
	*/

	switch (key)
	{
	case 27://ESC key. Free up GLUT window and exit.
		printf("Escape pressed, exiting emulator.\n");
		glutDestroyWindow(windowID);
		exit(0);
		break;
	case '1':
		chip8core.keypad[0x1] = 1;
		break;
	case '2':
		chip8core.keypad[0x2] = 1;
		break;
	case '3':
		chip8core.keypad[0x3] = 1;
		break;
	case '4':
		chip8core.keypad[0xC] = 1;
		break;
	case 'Q':
	case 'q':
		chip8core.keypad[0x4] = 1;
		break;
	case 'W':
	case 'w':
		chip8core.keypad[0x5] = 1;
		break;
	case 'E':
	case 'e':
		chip8core.keypad[0x6] = 1;
		break;
	case 'R':
	case 'r':
		chip8core.keypad[0xD] = 1;
		break;
	case 'A':
	case 'a':
		chip8core.keypad[0x7] = 1;
		break;
	case 'S':
	case 's':
		chip8core.keypad[0x8] = 1;
		break;
	case 'D':
	case 'd':
		chip8core.keypad[0x9] = 1;
		break;
	case 'F':
	case 'f':
		chip8core.keypad[0xE] = 1;
		break;
	case 'Z':
	case 'z':
		chip8core.keypad[0xA] = 1;
		break;
	case 'X':
	case 'x':
		chip8core.keypad[0x0] = 1;
		break;
	case 'C':
	case 'c':
		chip8core.keypad[0xB] = 1;
		break;
	case 'V':
	case 'v':
		chip8core.keypad[0xF] = 1;
		break;
	default:
		break;

	}
}
void handleKeyRelease(unsigned char key, int x, int y)
{

	switch (key)
	{
	case '1':
		chip8core.keypad[0x1] = 0;
		break;
	case '2':
		chip8core.keypad[0x2] = 0;
		break;
	case '3':
		chip8core.keypad[0x3] = 0;
		break;
	case '4':
		chip8core.keypad[0xC] = 0;
		break;
	case 'Q':
	case 'q':
		chip8core.keypad[0x4] = 0;
		break;
	case 'W':
	case 'w':
		chip8core.keypad[0x5] = 0;
		break;
	case 'E':
	case 'e':
		chip8core.keypad[0x6] = 0;
		break;
	case 'R':
	case 'r':
		chip8core.keypad[0xD] = 0;
		break;
	case 'A':
	case 'a':
		chip8core.keypad[0x7] = 0;
		break;
	case 'S':
	case 's':
		chip8core.keypad[0x8] = 0;
		break;
	case 'D':
	case 'd':
		chip8core.keypad[0x9] = 0;
		break;
	case 'F':
	case 'f':
		chip8core.keypad[0xE] = 0;
		break;
	case 'Z':
	case 'z':
		chip8core.keypad[0xA] = 0;
		break;
	case 'X':
	case 'x':
		chip8core.keypad[0x0] = 0;
		break;
	case 'C':
	case 'c':
		chip8core.keypad[0xB] = 0;
		break;
	case 'V':
	case 'v':
		chip8core.keypad[0xF] = 0;
		break;
	default:
		break;

	}
}

//####################################################################################################################################################################
/* main function*/
int main(int argc, char *argv[])
{
	/*GLUT init/window creation stuff*/
	glutInit(&argc, argv);					//init GLUT itself
	glutInitDisplayMode(GLUT_DOUBLE);		//Init display mode with bitwise ORing of GLUT display mode masks (GLUT_DOUBLE = Double buffered display)
	glutInitWindowPosition(100, 100);		//set window position on screen (-1,-1 is auto management mode)
	glutInitWindowSize(640, 320);			//set window dimensions
	windowID = glutCreateWindow("chip8 Emulator");		//now create said window
	/* end init/window stuff */

	/* register callbacks*/
	glutDisplayFunc(renderScene);			//assign the render function
	glutIdleFunc(renderScene);				//assign the idle function
	glutKeyboardFunc(handleKeyPress);		//assign keyboard function
	glutKeyboardUpFunc(handleKeyRelease);	//assign keyboardUp function
	/* first off, create the chip8 core */
	createChip8();

	/* Then get path to ROM specified in command line argument */
	char cCurrentPath[FILENAME_MAX];
	if (!GetCurrentDir(cCurrentPath, sizeof(cCurrentPath)))
	{
		return errno;
	}
	char filePath[FILENAME_MAX];
	/* If we're going to specify ROM name as a command line argument, use this instead:
	sprintf(filePath, "%s\\%s", cCurrentPath, argv[1]);
	*/

	/* Path to ROM. Uncomment/comment as necessary */
	/* Demos/Test Programs */
		//sprintf(filePath, "%s\\%s", cCurrentPath, "logo.ch8");
		//sprintf(filePath, "%s\\%s", cCurrentPath, "zerodemo.ch8");
		//sprintf(filePath, "%s\\%s", cCurrentPath, "keypad.ch8");
	/* Games */
		//sprintf(filePath, "%s\\%s", cCurrentPath, "pong2.ch8");
		sprintf(filePath, "%s\\%s", cCurrentPath, "brix.ch8");
		//sprintf(filePath, "%s\\%s", cCurrentPath, "invaders.ch8");

	/* load fontset*/
	loadFontSet();

	/*load the specified ROM into CHIP8 memory */
	loadProgram(filePath);

	/* infinite core game loop*/
	glutMainLoop();//meaning, all updation of game logic will take place inside the function defined in glDisplayFunc() (renderScene, in our case)

	return 0;
}
//####################################################################################################################################################################

