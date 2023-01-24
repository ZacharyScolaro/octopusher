#include "vcsLib.h"
#include "text.h"
#include "title_screen_2600.h"
#include "title_screen_7800.h"

// Temporary workaround to allow running in Gopher
// These functions aren't implemented yet and are only needed by the 7800 mode at the moment
void vcsWaitForAddress(uint16_t address) {}
void vcsJmpToRam3(uint16_t addr) {}
void injectDmaData(int address, int count, const uint8_t* pBuffer) {}
void vcsWrite6(uint16_t address, uint8_t data) {}

#define BOARD_HEIGHT 14
#define BOARD_WIDTH 17

const int8_t Kernel78[] = { 0xA5, 0x28, 0x30, 0xFC, 0xA5, 0x28, 0x10, 0xFC, 0xA9, 0x40, 0x85, 0x3C, 0xA5, 0x28, 0x10, 0xFC, 0xAD, 0xAA, 0xFF, 0xC9, 0x25, 0xD0, 0x12, 0xAD, 0xAB, 0xFF, 0xC9, 0xCC, 0xD0, 0x0B, 0xAE, 0xAC, 0xFF, 0xAD, 0xAD, 0xFF, 0x95, 0x00, 0x4C, 0x50, 0x00, 0xA5, 0x28, 0x30, 0xFC, 0x4C, 0x4C, 0x00 };

__attribute__((section(".noinit")))
static uint8_t screenBufer[192*80];

enum GameModeEnum
{
	solo,
	vs,
	co_op
};


const char* SelectionHeader[4] =
{
	"                  ",
	"   OCTOPUSHER     ",
	"                  ",
	"  Select a game   "
};
const char* SelectionName[3] =
{
	"<     Solo      > ",
	"<      VS.      > ",
	"<     Co-op     > ",
};
const char* SelectionHelp[7 * 4] =
{
	"Play alone on a full width board.   ",
	"                                    ",
	"                                    ",
	"                                    ",

	"Play on a single board. Once the    ",
	"board has been filled, whoever      ",
	"has the highest score wins.         ",
	"                                    ",

	"Work together towards a shared high ",
	"score.                              ",
	"                                    ",
	"                                    ",
};

const char Points[] =
{
	0,0,0,0,0,0, 
	0,0,0,0,0,0, 
	0,0,0,0,0,0, 
	0,0,0,0,0,1, // 3
	0,0,0,0,0,4, 
	0,0,0,0,1,6, 
	0,0,0,0,3,2,
	0,0,0,0,6,4,
	0,0,0,1,2,8,
	0,0,0,2,5,6,
	0,0,0,5,1,2,
	0,0,1,0,2,4,
	0,0,2,0,4,8,
	0,0,4,0,9,6,
	0,0,8,1,9,2,
	0,1,6,0,0,0,
	0,3,2,0,0,0,
	0,6,4,0,0,0,
	1,2,8,0,0,0,
	2,5,6,0,0,0,
	5,1,2,0,0,0,
	9,9,9,9,9,9,
};

// Bitwise AND of TileColorA & TileColorX will give 4th color which is used as background (no tile present)
#define TileColorA 0x2a
#define TileColorX 0x95
// use 0x38 for PAL?
#define TileColorY 0xc8
#define BorderColor 0x7d

#define P0Color 0x4c
#define P1Color 0x1e

typedef struct _GameModeState
{
	int is_7800;
	int mode_enum;
	int player_count;
	int score_type; // 0 - Single, 1 - Double
	int match_size;
} GameModeState;

static GameModeState game_mode_state = { 0 };

typedef struct _Player
{
	int x;
	int y;
	int col;
	int row;
	int moves_remaining;
	int x_vel;
	int y_vel;
	int block_color; // set to 0 when no block moved
	int block_x; // Track the last block player moved
	int block_y;
	int powered_up;
	int is_pressing_button;
	char score[16];
} Player;

static int8_t sprite[11] = { 0x1c, 0x3e, 0x2a, 0x3e, 0x3e, 0x1c, 0x1c, 0x3e, 0x2a, 0xcb, 0x00 };
static const int8_t title_screen_2600_colubk[16] = { 0x96, 0x96, 0x98, 0x98, 0x9a, 0x9a, 0x9c, 0x9c, 0x9c, 0x9c, 0x9a, 0x9a, 0x98, 0x98, 0x96, 0x96 };

__attribute__((section(".noinit")))
static int8_t blocks[BOARD_HEIGHT * BOARD_WIDTH + 4]; // Must keep multiple of 4 for quick zeroing

void Init7800();
void TitleScreen2600();
void TitleScreen7800();
int SelectionScreen();
void DisplayFullBoard(int p0y, int p1y, int m0y, int m1y);
void DisplaySplitBoard(int p0y, int p1y, int m0y, int m1y, int8_t* blocks0, int8_t* blocks1);
int LookForMatches(Player* player, int8_t* blocks0, int width, int min_length);
void PlaceRandomBlock(int8_t* blocks, int count);
void MovePlayer(Player* player, int joy, int8_t* blocks, int width);
void AddPoints(char* score, int total);
int ScoreToInt(char* score);
void DrawBitmap();
void InitBoard7800();
void PlaceCoral(int x, int y, int color);

int elf_main(uint32_t* args)
{
	int frameCount = 0;
	int tenthCount = 0;

	Player p0 = { 0 };
	Player p1 = { 0 };

	uint8_t m0y = 13;
	uint8_t m1y = 13;

	uint8_t joy0 = 0;
	uint8_t but0 = 0;
	uint8_t but1 = 0;


	game_mode_state.is_7800 = args[MP_SYSTEM_TYPE] == ST_NTSC_7800;

	if (game_mode_state.is_7800)
	{
		Init7800();
	}
	else
	{
		// Always reset PC first, cause it's going to be close to the end of the 6507 address space
		vcsJmp3();

		// Init TIA and RIOT RAM
		vcsLda2(0);
		for (int i = 0; i < 256; i++) {
			vcsSta3((unsigned char)i);
		}
		vcsCopyOverblankToRiotRam();

		vcsStartOverblank();
	}

	for (int i = 0; i < sizeof(blocks)/sizeof(int); i++)
	{
		((int*)blocks)[i] = 0;
	}

	for (int i = 0; i < 16; i++)
	{
		p0.score[i] = p1.score[i] = '0';
	}
	p0.score[15] = ' ';
	p1.score[0] = ' ';

	if (game_mode_state.is_7800)
	{
		TitleScreen7800();
	}
	else
	{
		TitleScreen2600();
	}

	game_mode_state.match_size = 3;
	game_mode_state.mode_enum = SelectionScreen();

	p0.powered_up = 0;
	p1.powered_up = 0;

	switch (game_mode_state.mode_enum) {
	case solo:
	default:
		game_mode_state.player_count = 1;
		game_mode_state.score_type = 0;
		break;
	case vs:
	case co_op:
		game_mode_state.player_count = 2;
		game_mode_state.score_type = 1;
		break;
	}

	if (game_mode_state.is_7800)
	{
		InitBoard7800();
	}

	while (1)
	{
		frameCount++;
		if (frameCount == 6)
		{
			frameCount = 0;
			tenthCount++;
		}
		if ((tenthCount & 0xf) + frameCount  == 0)
		{
			PlaceRandomBlock(blocks, BOARD_WIDTH * BOARD_HEIGHT);
		}
		PrintSmall(0, 0, 16, p0.score);
		if (game_mode_state.score_type == 0)
		{
			PrintSmall(0, 18, 16, SelectionHeader[0]);
		}
		else
		{
			PrintSmall(0, 18, 16, p1.score);
		}
		char mc = (char)('0' + game_mode_state.match_size);
		PrintLarge(0, 8, 1, &mc);
		PrintLarge(0, 17, 1, " ");

		if(game_mode_state.is_7800)
		{
			DrawBitmap();
		}
		else
		{
			vcsEndOverblank();

			DisplayText(1);

			vcsSta3(WSYNC);
			vcsWrite5(PF0, 0xc0);
			vcsWrite5(PF1, 0xff);
			vcsWrite5(PF2, 0xff);
			vcsWrite5(COLUPF, BorderColor);
			vcsWrite5(CTRLPF, 0x01);
			vcsJmp3();
			vcsLdx2(TileColorX);
			vcsLdy2(TileColorY);
			vcsNop2n(14);
			vcsSta3(RESM1);
			vcsNop2();
			vcsWrite5(HMBL, 0xf0);
			vcsSta3(RESBL);

			vcsSta3(WSYNC);
			int x = p0.x + 1;
			vcsNop2n(7);
			while (x > -1)
			{
				vcsNop2();
				x -= 6;
			}
			vcsWrite5(HMP0, (uint8_t)((0 - x) << 4));
			vcsSta3(RESP0);

			vcsSta3(WSYNC);
			x = p1.x + 1;
			vcsNop2n(7);
			while (x > -1)
			{
				vcsNop2();
				x -= 6;
			}
			vcsWrite5(HMP1, (uint8_t)((0 - x) << 4));
			vcsSta3(RESPONE);

			vcsSta3(WSYNC);
			vcsSta3(HMOVE);
			vcsWrite5(COLUP0, P0Color);
			vcsWrite5(COLUP1, P1Color);
			vcsLda2(0);
			vcsSta3(NUSIZ0);
			vcsSta3(NUSIZ1);
			vcsSta3(VDELP0);
			vcsSta3(VDELP1);

			vcsLdx2(0xef);
			vcsTxs2();
			vcsWrite5(0xf0, BorderColor);
			vcsPlp4();

			vcsLdx2(COLUPF - 1);
			vcsTxs2();
			vcsLdx2(TileColorX);


			for (int i = 0; i < 2; i++)
			{
				vcsSta3(WSYNC);
				vcsWrite5(VBLANK, 0x80); // Clears D7 when reading from INPT1 (PLA, SP=8
				vcsWrite5(COLUBK, BorderColor);
				vcsWrite5(COLUPF, BorderColor);
				vcsWrite5(PF0, 0xe0);
				vcsWrite5(GRP0, 0);
				vcsWrite5(GRP1, 0);
				vcsWrite5(ENABL, 0x02);
				vcsJmp3();
			}

			vcsNop2n((76 / 2) - 21);

			DisplayFullBoard(p0.y, p1.y, m0y, m1y);

			for (int i = 0; i < 5; i++)
			{
				vcsWrite5(VBLANK, 0);
				vcsWrite5(COLUPF, BorderColor);
				if (i < 2)
					vcsWrite5(COLUBK, BorderColor);
				else
					vcsWrite5(COLUBK, 0);
				vcsWrite5(PF0, 0xc0);
				vcsWrite5(ENABL, 0);
				vcsSta3(WSYNC);

			}

			vcsWrite5(VBLANK, 2);
			joy0 = vcsRead4(SWCHA);
			but0 = vcsRead4(INPT4);
			but1 = vcsRead4(INPT5);
			vcsSta3(WSYNC);
			p0.is_pressing_button = (but0 & 0x80) == 0;
			p1.is_pressing_button = (but1 & 0x80) == 0;
			vcsStartOverblank();
		}

		p0.block_color = p1.block_color = 0;
		MovePlayer(&p0, joy0, blocks, BOARD_WIDTH);
		if (game_mode_state.player_count > 1) {
			MovePlayer(&p1, joy0 << 4, blocks, BOARD_WIDTH);
		}
		else {
			MovePlayer(&p1, joy0, blocks, BOARD_WIDTH);
		}

		int p0_total = 0;
		int p1_total = 0;
		p0_total = LookForMatches(&p0, blocks, BOARD_WIDTH, game_mode_state.match_size);
		if (game_mode_state.player_count > 1) {
			p1_total = LookForMatches(&p1, blocks, BOARD_WIDTH, game_mode_state.match_size);
		}
		AddPoints(p0.score, p0_total);
		int highestScore = ScoreToInt(p0.score);
		if (game_mode_state.score_type == 0)
		{
			AddPoints(p0.score, p1_total);
		}
		else
		{
			AddPoints(&p1.score[1], p1_total);
			int p1score = ScoreToInt(&p1.score[1]);
			if (p1score> highestScore)
			{
				highestScore = p1score;
			}
		}
		// increase level at certain scores

//		for (int i = 0; i < 8; i++)
//		{
//			char v = ((p0score >> (4*(7 - i))) & 0xf);
//			p1.score[i + 1] = v < 10 ? '0' + v : 'A' + v - 10;
//		}
//		p1.score[9] = ' ';

		if (highestScore >= 100000000)
		{
			game_mode_state.match_size = 8;
		}
		else if (highestScore >= 1000000)
		{
			game_mode_state.match_size = 7;
		}
		else if (highestScore >= 10000)
		{
			game_mode_state.match_size = 6;
		}
		else if (highestScore >= 500)
		{
			game_mode_state.match_size = 5;
		}
		else if (highestScore >= 50)
		{
			game_mode_state.match_size = 4;
		}
	}
}

void AddPoints(char* score, int total)
{
	if (total >= sizeof(Points) / 6)
	{
		total = (sizeof(Points) / 6) - 1;
	}
	int carry = 0;
	for (int i = 0; i < 15; i++)
	{
		if (i < 6)
		{
			score[14 - i] += Points[(total*6) + 5 - i];
		}
		score[14 - i] += carry;
		carry = 0;
		while (score[14 - i] > '9')
		{
			score[14 - i] -= 10;
			carry++;
		}
	}
}

void DisplayFullBoard(int p0y, int p1y, int m0y, int m1y)
{
	// 168 visible lines
	int blockY = 0;
	int k = 0;
	int m0height = 0;
	int m1height = 0;
	int p0si = 0;
	int p1si = 0;

	for (int i = 0; i < BOARD_HEIGHT*12; i++)
	{
		// TODO AUDIO
		vcsNop2n(2); // vcsPla4(); // vcsPla4Ex(0x10);
		if (k > 11)
		{
			k = 0;
			blockY++;
		}
		k++;
		vcsSta3(AUDV0);
		if (i >= p0y && p0si < 11)
		{
			vcsWrite5(GRP0, sprite[p0si++]);
		}
		else
		{
			vcsJmp3();
			vcsNop2();
		}
		if (i >= p1y && p1si < 11)
		{
			vcsWrite5(GRP1, sprite[p1si++]);
		}
		else
		{
			vcsJmp3();
			vcsNop2();
		}
		vcsLda2(TileColorA);
		if (i >= m0y && m0height > 0)
		{
			vcsSta3(ENAM0);
			m0height--;
		}
		else
		{
			vcsStx3(ENAM0);
		}
		if (i >= m1y && m1height > 0)
		{
			vcsSta3(ENAM1);
			m1height--;
		}
		else
		{
			vcsStx3(ENAM1);
		}
		for (int j = 0; j < BOARD_WIDTH; j++)
		{
			switch (blocks[(blockY * BOARD_WIDTH) + j])
			{
			default:
				vcsSax3(COLUPF);
				break;
			case 1:
				vcsSta3(COLUPF);
				break;
			case 2:
				vcsStx3(COLUPF);
				break;
			case 3:
				vcsSty3(COLUPF);
				break;
			}
		}
		//vcsPhp3();
	}
}

int LookForMatches(Player* player, int8_t* blocks0, int width, int min_length)
{
	if (player->block_color == 0)
	{
		return 0;
	}
	int total = 1;
	int yb = player->block_y;
	int yt = player->block_y;
	int xl = player->block_x;
	int xr = player->block_x;
	// Check each direction
	// Up
	for (int y = player->block_y - 1; y >= 0 && (blocks0[y * width + player->block_x] == player->block_color); y--)
	{
		total++;
		yt = y;
	}
	// Down
	for (int y = player->block_y + 1; y < BOARD_HEIGHT && (blocks0[y * width + player->block_x] == player->block_color); y++)
	{
		total++;
		yb = y;
	}
	// Left
	for (int x = player->block_x - 1; x >= 0 && (blocks0[player->block_y * width + x] == player->block_color); x--)
	{
		total++;
		xl = x;
	}
	// Right
	for (int x = player->block_x + 1; x < width && (blocks0[player->block_y * width + x] == player->block_color); x++)
	{
		total++;
		xr = x;
	}
	// Clear blocks if min met
	if (total >= min_length)
	{
		for (int x = xl; x <= xr; x++)
		//for (int x = 0; x < width; x++)
		{
			blocks0[x + (player->block_y * width)] = 0;
		}
		for (int y = yt; y <= yb; y++)
		//for (int y = 0; y < BOARD_HEIGHT; y++)
		{
			blocks0[player->block_x + (y * width)] = 0;
		}
		return total;
	}
	return 0;
}

void PlaceRandomBlock(int8_t* blocks, int count)
{
	int width = BOARD_WIDTH;
	for (int j = 0; j < (20-game_mode_state.match_size); j++)
	{
		int i = randint() % count;
		if (blocks[i] == 0)
		{
			blocks[i] = (randint() % 3) + 1;

			PlaceCoral(i % width, i / width, blocks[i]);
			break;
		}
	}
}

void MovePlayer(Player* p, int joy, int8_t* blocks, int width)
{
	if (p->moves_remaining > 0)
	{
		p->moves_remaining--;
		p->x += p->x_vel;
		p->y += p->y_vel;
		if (p->moves_remaining == 6 || p->moves_remaining == 3 || p->moves_remaining == 0)
		{
			p->y += p->y_vel;
		}
		if (p->moves_remaining == 0)
		{
			p->x_vel = p->y_vel = 0;
		}
	}
	else if (((joy & 0x10) == 0) && p->row > 0)
	{
		// Up
		if (blocks[((p->row - 1) * width) + p->col] == 0)
		{
			if (p->is_pressing_button &&
				p->row < BOARD_HEIGHT-1 && blocks[((p->row + 1) * width) + p->col] != 0)
			{
				// Remember which block is moved for later
				p->block_x = p->col;
				p->block_y = p->row;
				p->block_color = blocks[((p->row + 1) * width) + p->col];
				blocks[((p->row) * width) + p->col] = p->block_color;
				blocks[((p->row + 1) * width) + p->col] = 0;
			}
			p->row--;
			p->y_vel = -1;
			p->moves_remaining = 9;
		}
		else if (p->row > 1 && blocks[((p->row - 2) * width) + p->col] == 0)
		{
			// Remember which block is moved for later
			p->block_x = p->col;
			p->block_y = p->row - 2;
			p->block_color = blocks[((p->row - 1) * width) + p->col];
			// Move the block
			blocks[((p->row - 2) * width) + p->col] = blocks[((p->row - 1) * width) + p->col];
			blocks[((p->row - 1) * width) + p->col] = 0;
			p->row--;
			p->y_vel = -1;
			p->moves_remaining = 9;
		}
	}
	else if (((joy & 0x20) == 0) && p->row < BOARD_HEIGHT -1)
	{
		// Down
		if (blocks[((p->row + 1) * width) + p->col] == 0)
		{
			if (p->is_pressing_button &&
				p->row > 0 && blocks[((p->row - 1) * width) + p->col] != 0)
			{
				// Remember which block is moved for later
				p->block_x = p->col;
				p->block_y = p->row;
				p->block_color = blocks[((p->row - 1) * width) + p->col];
				blocks[((p->row) * width) + p->col] = p->block_color;
				blocks[((p->row - 1) * width) + p->col] = 0;
			}
			p->row++;
			p->y_vel = 1;
			p->moves_remaining = 9;
		}
		else if (p->row < BOARD_HEIGHT - 2 && blocks[((p->row + 2) * width) + p->col] == 0)
		{
			// Remember which block is moved for later
			p->block_x = p->col;
			p->block_y = p->row + 2;
			p->block_color = blocks[((p->row + 1) * width) + p->col];
			// Move the block
			blocks[((p->row + 2) * width) + p->col] = blocks[((p->row + 1) * width) + p->col];
			blocks[((p->row + 1) * width) + p->col] = 0;
			p->row++;
			p->y_vel = 1;
			p->moves_remaining = 9;
		}
	}
	else if (((joy & 0x40) == 0) && p->col > 0)
	{
		// Left
		if (blocks[(p->row * width) + (p->col - 1)] == 0)
		{
			if( p->is_pressing_button &&
				p->col < width - 1 && blocks[((p->row) * width) + p->col + 1] != 0)
			{
				// Remember which block is moved for later
				p->block_x = p->col;
				p->block_y = p->row;
				p->block_color = blocks[((p->row) * width) + p->col + 1];
				blocks[((p->row) * width) + p->col] = p->block_color;
				blocks[((p->row) * width + 1) + p->col] = 0;
			}
			p->col--;
			p->x_vel = -1;
			p->moves_remaining = 9;
		}
		else if (p->col > 1 && blocks[(p->row * width) + (p->col - 2)] == 0)
		{
			// Remember which block is moved for later
			p->block_x = p->col - 2;
			p->block_y = p->row;
			p->block_color = blocks[(p->row * width) + (p->col - 1)];
			// Move the block
			blocks[(p->row * width) + (p->col - 2)] = blocks[(p->row * width) + (p->col - 1)];
			blocks[(p->row * width) + (p->col - 1)] = 0;
			p->col--;
			p->x_vel = -1;
			p->moves_remaining = 9;
		}
	}
	else if (((joy & 0x80) == 0) && p->col < width-1)
	{
		// Right
		if (blocks[(p->row * width) + (p->col + 1)] == 0)
		{
			if (p->is_pressing_button &&
				p->col > 0 && blocks[((p->row) * width) + p->col - 1] != 0)
			{
				// Remember which block is moved for later
				p->block_x = p->col;
				p->block_y = p->row;
				p->block_color = blocks[((p->row) * width) + p->col - 1];
				blocks[((p->row) * width) + p->col] = p->block_color;
				blocks[((p->row) * width - 1) + p->col] = 0;
			}
			p->col++;
			p->x_vel = 1;
			p->moves_remaining = 9;
		}
		else if (p->col < width - 2 && blocks[(p->row * width) + (p->col + 2)] == 0)
		{
			// Remember which block is moved for later
			p->block_x = p->col + 2;
			p->block_y = p->row;
			p->block_color = blocks[(p->row * width) + (p->col + 1)];
			// Move the block
			blocks[(p->row * width) + (p->col + 2)] = blocks[(p->row * width) + (p->col + 1)];
			blocks[(p->row * width) + (p->col + 1)] = 0;
			p->col++;
			p->x_vel = 1;
			p->moves_remaining = 9;
		}
	}
}

void TitleScreen2600()
{
	int frameCount = 0;
	int tenthCount = 0;
	uint8_t p0ButtonPrev = 0xff;
	uint8_t p0Button = 0xff;

	// Title Screen
	while (tenthCount < 10 || !((p0Button & 0x80) && ((p0ButtonPrev & 0x80) == 0))) {
		frameCount++;
		if (frameCount == 6)
		{
			frameCount = 0;
			tenthCount++;
		}

		vcsEndOverblank();

		vcsSta3(WSYNC);
		vcsSta3(WSYNC);
		vcsSta3(WSYNC);
		vcsWrite5(CTRLPF, 0x01);
		vcsWrite5(COLUPF, 0x80);
		vcsWrite5(GRP0, 0xff);
		vcsWrite5(GRP1, 0xff);

		vcsSta3(WSYNC);
		vcsWrite5(NUSIZ0, 0x03);
		vcsWrite5(NUSIZ1, 0x03);
		vcsWrite5(COLUP0, 0x0f);
		vcsWrite5(COLUP1, 0x0f);
		vcsNop2n(8);
		vcsSta3(RESP0);
		vcsSta3(RESPONE);
		vcsWrite5(HMP0, 0xe0);
		vcsWrite5(HMP1, 0xf0);
		vcsWrite5(VDELP0, 0x01);
		vcsWrite5(VDELP1, 0x01);

		vcsSta3(WSYNC);
		vcsSta3(HMOVE);
		vcsNop2n(31);
		vcsJmp3();
		vcsSta3(HMCLR);
		vcsWrite5(VBLANK, 0);

		// Render
		int y = 0;
		for (int i = 0; i < 192; i++)
		{
			vcsWrite5(COLUBK, title_screen_2600_colubk[0xf & (i + tenthCount)]);
			vcsWrite5(PF0, TitleScreen2600Graphics[y]);
			vcsWrite5(PF1, TitleScreen2600Graphics[y + 1]);
			vcsWrite5(PF2, TitleScreen2600Graphics[y + 2]);
			vcsLdx2(TitleScreen2600Graphics[y + 7]);
			vcsLdy2(TitleScreen2600Graphics[y + 8]);
			vcsWrite5(GRP0, TitleScreen2600Graphics[y + 3]);
			vcsWrite5(GRP1, TitleScreen2600Graphics[y + 4]);
			vcsWrite5(GRP0, TitleScreen2600Graphics[y + 5]);
			vcsWrite5(GRP1, TitleScreen2600Graphics[y + 6]);
			vcsStx3(GRP0);
			vcsSty3(GRP1);
			vcsStx3(GRP0);
			vcsJmp3();
			y += 9;
			vcsSta3(WSYNC);
		}

		vcsWrite5(VBLANK, 2);

		p0ButtonPrev = p0Button;
		p0Button = vcsRead4(INPT4);

		vcsSta3(WSYNC);
		vcsStartOverblank();
	}
}

int SelectionScreen()
{
	int timeout = 0;
	int selection = 0;
	int debounce = 0;
	uint8_t joy0 = 0xff;
	uint8_t p0Button = 0xff;

	for (int i = 0; i < 4; i++)
	{
		PrintLarge(i, 0, 18, SelectionHeader[i]);
	}
	PrintLarge(4, 0, 18, SelectionHeader[0]);
	PrintLarge(6, 0, 18, SelectionHeader[0]);
	while ((p0Button & 0x80) && timeout < 60)
	{
		PrintLarge(5, 0, 18, SelectionName[selection]);
		for (int i = 0; i < 4; i++)
		{
			PrintSmall(7+i, 0, 36, SelectionHelp[selection * 4 + i]);
		}
		
		if(game_mode_state.is_7800)
		{ 
			timeout++;
			DrawBitmap();
		}
		else
		{
			vcsEndOverblank();
			DisplayText(11);
			vcsWrite5(VBLANK, 2);
			joy0 = vcsRead4(SWCHA);
			p0Button = vcsRead4(INPT4);
			for (int i = 0; i < 51; i++)
			{
				vcsSta3(WSYNC);
			}
			vcsStartOverblank();
		}

		if (debounce == 0)
		{
			if ((joy0 & 0x80) == 0)
			{
				// Right
				debounce++;
				selection++;
				if (selection > 6)
				{
					selection = 0;
				}
			}
			else if ((joy0 & 0x40) == 0)
			{
				// Left
				debounce++;
				selection--;
				if (selection < 0)
				{
					selection = 6;
				}
			}
		}
		else if ((joy0 & 0xf0) == 0xf0)
		{
			debounce = 0;
		}
	}
	return selection;
}

int ScoreToInt(char* score)
{
	int total = 0;
	int multiplier = 1;
	for (int i = 0; i < 15; i++)
	{
		total += (score[14 - i] - '0') * multiplier;
		if (multiplier <= 100000000)
		{
			multiplier *= 10;
		}
	}
	return total;
}

void Init7800()
{
	Enable7800Mode(screenBufer);

	// Must lock into 7800 mode before doing any TIA accesses
	vcsWrite5(INPTCTRL, 0x7);
	vcsNop2n(2);
	// Pallette


	vcsWrite5(BACKGRND, TitleScreen7800Palette[0]);

	vcsWrite5(P0C1, TitleScreen7800Palette[1]);
	vcsWrite5(P0C2, TitleScreen7800Palette[2]);
	vcsWrite5(P0C3, TitleScreen7800Palette[3]);
	vcsWrite5(P1C1, TitleScreen7800Palette[5]);
	vcsWrite5(P1C2, TitleScreen7800Palette[6]);
	vcsWrite5(P1C3, TitleScreen7800Palette[7]);
	vcsWrite5(P2C1, TitleScreen7800Palette[9]);
	vcsWrite5(P2C2, TitleScreen7800Palette[10]);
	vcsWrite5(P2C3, TitleScreen7800Palette[11]);
	vcsWrite5(P3C1, TitleScreen7800Palette[13]);
	vcsWrite5(P3C2, TitleScreen7800Palette[14]);
	vcsWrite5(P3C3, TitleScreen7800Palette[15]);

	vcsWrite5(P4C1, 0x2f);
	vcsWrite5(P4C2, 0x7f);
	vcsWrite5(P4C3, 0xCf);

	vcsWrite5(DPPL, 0x00);
	vcsWrite5(DPPH, 0x22);
	// Address RAM via 0x2000-0x2fff to avoid driving A12 high

	// DLL starts at 0x2200-0x222f	(16 DLL entries * 3 = 48 bytes for DLL)
	// Leave top 25 empty
	vcsWrite6(0x2200, 0x08);
	vcsWrite6(0x2201, 0x22);
	vcsWrite6(0x2202, 0x46);
	vcsWrite6(0x2203, 0x0f);
	vcsWrite6(0x2204, 0x22);
	vcsWrite6(0x2205, 0x46);
	vcsNop2n(2);

	// 12 zones each 16 tall
	for (uint16_t i = 0x2206; i < 0x222a; i += 3)
	{
		vcsWrite6(i, 0x0f);
		vcsWrite6(i + 1, 0x22);
		vcsWrite6(i + 2, 0x30);
		vcsNop2n(2);
	}

	// Leave bottom 26 empty
	vcsWrite6(0x222a, 0x0f);
	vcsWrite6(0x222b, 0x22);
	vcsWrite6(0x222c, 0x46);
	vcsWrite6(0x222d, 0x09);
	vcsWrite6(0x222e, 0x22);
	vcsWrite6(0x222f, 0x46);


	// DL for 160x224 with 16 total colors
	// 32 4bpp
	vcsWrite6(0x2230, 0xe0);
	vcsWrite6(0x2231, 0xc0); // 160b
	vcsWrite6(0x2232, 0xf0);
	vcsWrite6(0x2233, 0x00);
	vcsWrite6(0x2234, 0x00);

	vcsWrite6(0x2235, 0xe0);
	vcsWrite6(0x2236, 0x08);
	vcsWrite6(0x2237, 0xf0);
	vcsWrite6(0x2238, 0x40);

	vcsWrite6(0x2239, 0xe0);
	vcsWrite6(0x223a, 0x08);
	vcsWrite6(0x223b, 0xf0);
	vcsWrite6(0x223c, 0x70);

	vcsWrite6(0x223d, 0x0); //e0
	vcsWrite6(0x223e, 0x0); //40  160a
	vcsWrite6(0x223f, 0x0); //f0
	vcsWrite6(0x2240, 0x8c);
	vcsWrite6(0x2241, 0x00);

	vcsWrite6(0x2242, 0xe0);
	vcsWrite6(0x2243, 0x8c);
	vcsWrite6(0x2244, 0xf0);
	vcsWrite6(0x2245, 0x50);

	// 0x2230 - end of DL
	vcsWrite6(0x2246, 0);
	vcsWrite6(0x2247, 0);
	vcsNop2n(2);

	// simple program to wait for vblank, enable DMA, and then endless loop CPU
//			$0040
//			wfvb: lda $28		a5 28
//			  bpl				10 fc
//
//			  lda #$40			a9 40
//			  sta $3c			85 3c
//
//			loop: jmp loop		4c 48 00


	for (int i = 0; i < sizeof(Kernel78); i++)
	{
		vcsWrite5((uint8_t)(0x40 + i), (uint8_t)Kernel78[i]);
	}


	//		while(1)
	//			;
	// TEST PATTERN
	vcsNop2n(2);
	while (1)
	{
		for (int i = 0; i < sizeof(Kernel78); i++)
		{
			vcsJmp3();
			vcsWrite5(BACKGRND, (uint8_t)Kernel78[i]);
			vcsSta3(MARIA_WSYNC);
			vcsSta3(MARIA_WSYNC);
		}
	}

	int clr = 0;
	while (0)
	{
		clr++;
		if (clr > 262)
			clr = 0;
		vcsWrite6(BACKGRND, clr);
		vcsJmp3();
		vcsSta3(MARIA_WSYNC);
	}


	//		while(1)
	//		{
	//			vcsJmp3();
	//			vcsWrite5(BACKGRND, 0x4c);
	//			vcsWrite6(BACKGRND, 0x8c);
	//		}

	vcsJmpToRam3(0x0040);
	vcsWaitForAddress(0x004c);
}

void TitleScreen7800()
{
	int frameCount = 0;
	int tenthCount = 0;
	uint8_t p0ButtonPrev = 0xff;
	uint8_t p0Button = 0xff;

	if (game_mode_state.is_7800)
	{
		int* buffer = (int*)screenBufer;
		for (int i = 0; i < 192 * 20; i++)
		{
			buffer[i] = 0;
		}
	}

	// Title Screen
	while (tenthCount < 10){//} || !((p0Button & 0x80) && ((p0ButtonPrev & 0x80) == 0))) {
		frameCount++;
		if (frameCount == 6)
		{
			frameCount = 0;
			tenthCount++;
		}

		vcsWaitForAddress(0x0069);

		for (int row = 0; row < 192; )
		{
			for (int i = 0x1fe0; i > 0x1000; i -= 0x100)
			{
				const uint8_t* pRow = TitleScreen7800Graphics + (row * 80);
				row++;
				injectDmaData(i, 32, pRow);
				injectDmaData(i, 24, &(pRow[32]));
				injectDmaData(i, 24, &(pRow[56]));
			}
		}
	}
}

void DrawBitmap()
{
	vcsWaitForAddress(0x0069);

	for (int row = 0; row < 192; )
	{
		for (int i = 0x1fe0; i > 0x1000; i -= 0x100)
		{
			const uint8_t* pRow = screenBufer + (row * 80);
			row++;
			injectDmaData(i, 32, pRow);
			injectDmaData(i, 24, &(pRow[32]));
			injectDmaData(i, 24, &(pRow[56]));
		}
	}
}


void InitBoard7800()
{
	int* buffer = (int*)screenBufer;
	for (int i = 0; i < 192*20; i++)
	{
		buffer[i] = 0;
	}

	int index = 0;
	for (int y = 0; y < 6; y++)
	{
		for (int x = 0; x < 20; x++)
		{
			buffer[12 * 20 + index] = buffer[186 * 20 + index] = 0x50505050;
			index++;
		}
	}

	for (int y = 18*20; y < 186*20; y+=20)
	{
		buffer[y] = 0x00005050;
		buffer[y + 19] = 0x50100000;
	}
}

static uint8_t coral1[] =
{
	0x0, 0x3, 0xf, 0x0, 0x0,
	0x3, 0xe, 0xe, 0xf, 0x0,
	0xd, 0xa, 0xa, 0xa, 0xc,
	0xe, 0xb, 0xb, 0xa, 0xc,
	0xb, 0xf, 0xf, 0xf, 0x8,
	0xb, 0xd, 0xd, 0xf, 0x8,
	0xd, 0xa, 0xa, 0xa, 0xc,
	0xe, 0xb, 0xb, 0xa, 0xc,
	0xb, 0xf, 0xf, 0xf, 0x8,
	0xb, 0xd, 0xd, 0xf, 0x8,
	0x3, 0xe, 0xe, 0xf, 0x0,
	0x0, 0x3, 0xf, 0x0, 0x0,
};
/*
00 00       0
10 00       8
11 00       c
00 10       2
10 10       a
11 10       d
00 11       3
10 11       b
11 11       f
*/

static uint8_t coralColorLookup[16*3] =
{
	0x0,
	0x0,
	0x20,
	0x30,
	0x0,
	0x0,
	0x0,
	0x0,
	0x80,
	0x0,
	0xa0,
	0xb0,
	0xc0,
	0xd0,
	0x0,
	0xf0,
	// 2 -> 5, 3-> 6
	0x0,
	0x0,
	0x11,
	0x21,
	0x0,
	0x0,
	0x0,
	0x0,
	0x44,
	0x0,
	0x55,
	0x65,
	0x84,
	0x95,
	0x0,
	0xa5,
	// 2->f 3->e
	0x0,
	0x0,
	0x33,
	0x23,
	0x0,
	0x0,
	0x0,
	0x0,
	0xcc,
	0x0,
	0xff,
	0xef,
	0x8c,
	0xbf,
	0x0,
	0xaf,
};

void PlaceCoral(int x, int y, int color)
{
	uint8_t* lu = &coralColorLookup[(color - 1) * 16];
	int index = (18 + y * 12) * 80 + 2 + (x * 9 / 2);
	int coralIndex = 0;
	for (int y = 0; y < 12; y++)
	{
		for (int x = 0; x < 4; x++)
		{
			screenBufer[index++] = lu[coral1[coralIndex++]];
		}
		coralIndex++;
		index += 76;
	}
}