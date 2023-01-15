#include "vcsLib.h"

#define GlyphHeight 12
#define MaxRowCount 13

static void kernelA(uint8_t textBuffer[18]);
static void kernelB(uint8_t textBuffer[18]);

static int frameCount = 0;
static uint16_t font[128 * 3] =
{
	0,0,0, // Null char
	0,0,0, // Start of Heading
	0,0,0, // Start of Text
	0,0,0, // End of Text
	0,0,0, // End of Transmission
	0,0,0, // Enquiry
	0,0,0, // Acknowledgment
	0,0,0, // Bell
	0,0,0, // Back Space
	0,0,0, // Horizontal Tab
	0,0,0, // Line Feed
	0,0,0, // Vertical Tab
	0,0,0, // Form Feed
	0,0,0, // Carriage Return
	0,0,0, // Shift Out / X - On
	0,0,0, // Shift In / X - Off
	0,0,0, // Data Line Escape
	0,0,0, // Device Control 1 (oft.XON)
	0,0,0, // Device Control 2
	0,0,0, // Device Control 3 (oft.XOFF)
	0,0,0, // Device Control 4
	0,0,0, // Negative Acknowledgement
	0,0,0, // Synchronous Idle
	0,0,0, // End of Transmit Block
	0,0,0, // Cancel
	0,0,0, // End of Medium
	0,0,0, // Substitute
	0,0,0, // Escape
	0,0,0, // File Separator
	0,0,0, // Group Separator
	0,0,0, // Record Separator
	0,0,0, // Unit Separator
	// *** BEGIN PRINTABLE CHARACTERS ***
	0,0,0, // Space
	0b010010010010, 0b010010000000, 0b010010000000, // !
	0b101101101000, 0b000000000000, 0b000000000000, // "
	0b001001101111, 0b111101101111, 0b111101100100, // #
	0b010010111101, 0b100110011001, 0b101111010010, // $
	0b100101001011, 0b010010110100, 0b101001000000, // %
	0b010111101011, 0b110101101101, 0b111011001000, // &
	0b010010010010, 0b010000000000, 0b000000000000, // '
	0b001011010110, 0b100100100100, 0b110010011001, // (
	0b100110010011, 0b001001001001, 0b011010110100, // )
	0b000000101101, 0b010111111010, 0b101101000000, // *
	0b000000010010, 0b010111111010, 0b010010000000, // +
	0b000000000000, 0b000000000010, 0b010010100100, // ,
	0b000000000000, 0b000111111000, 0b000000000000, // -
	0b000000000000, 0b000000000010, 0b010010000000, // .
	0b001001001001, 0b010010010010, 0b100100100100, // /
	0b010111101101, 0b101101101101, 0b111110000000, // 0
	0b010110110010, 0b010010010010, 0b010010000000, // 1
	0b110111001001, 0b001011110100, 0b111111000000, // 2
	0b110111001001, 0b010011001001, 0b111110000000, // 3
	0b001001101101, 0b101111111001, 0b001001000000, // 4
	0b111111100110, 0b111001001001, 0b111110000000, // 5
	0b011111100110, 0b111101101101, 0b111010000000, // 6
	0b111111001001, 0b001010010010, 0b010010000000, // 7
	0b010111101101, 0b110011101101, 0b111110000000, // 8
	0b011111101101, 0b101111011001, 0b001001000000, // 9
	0b000000000010, 0b010010000010, 0b010010000000, // :
	0b000000000010, 0b010010000010, 0b010010100100, // ;
	0b000001001010, 0b010100100010, 0b010001001000, // <
	0b000000000111, 0b111000000111, 0b111000000000, // =
	0b000100100010, 0b010001001010, 0b010100100000, // >
	0b110111001001, 0b011010010000, 0b010010000000, // ?
	0b010010101101, 0b000010010000, 0b101101010010, // @
	0b011111101101, 0b101111111101, 0b101101000000, // A
	0b110111101101, 0b110111101101, 0b111110000000, // B
	0b011111100100, 0b100100100100, 0b111011000000, // C
	0b110111101101, 0b101101101101, 0b111110000000, // D
	0b111111100100, 0b110110100100, 0b111111000000, // E
	0b111111100100, 0b110110100100, 0b100100000000, // F
	0b011111100100, 0b100101101101, 0b111011000000, // G
	0b101101101101, 0b111111101101, 0b101101000000, // H
	0b111111010010, 0b010010010010, 0b111111000000, // I
	0b001001001001, 0b001001001001, 0b111110000000, // J
	0b101101101111, 0b110110101101, 0b101101000000, // K
	0b100100100100, 0b100100100100, 0b111111000000, // L
	0b101101111111, 0b111101101101, 0b101101000000, // M
	0b001001101101, 0b111111111101, 0b101100000000, // N
	0b010111101101, 0b101101101101, 0b111010000000, // O
	0b110111101101, 0b101111110100, 0b100100000000, // P
	0b010111101101, 0b101101101101, 0b110011001000, // Q
	0b110111101101, 0b101110111101, 0b101101000000, // R
	0b011111100100, 0b110011001001, 0b111110000000, // S
	0b111111010010, 0b010010010010, 0b010010000000, // T
	0b101101101101, 0b101101101101, 0b111010000000, // U
	0b101101101101, 0b101101111111, 0b010010000000, // V
	0b101101101101, 0b101111111111, 0b101101000000, // W
	0b101101101111, 0b010010111101, 0b101101000000, // X
	0b101101101101, 0b111111010010, 0b010010000000, // Y
	0b111111001001, 0b011010110100, 0b111111000000, // Z
	0b011011010010, 0b010010010010, 0b010010011011, // [
	0b100100100100, 0b010010010010, 0b001001001001, // \ (BackSlash)
	0b110110010010, 0b010010010010, 0b010010110110, // ]
	0b010010111101, 0b101000000000, 0b000000000000, // ^
	0b000000000000, 0b000000000000, 0b000000111111, // _
	0b100100110010, 0b010000000000, 0b000000000000, // `
	0b000000110111, 0b001011111101, 0b111011000000, // a
	0b100100110111, 0b101101101101, 0b111110000000, // b
	0b000000011111, 0b100100100100, 0b111011000000, // c
	0b001001011111, 0b101101101101, 0b111011000000, // d
	0b000000010111, 0b101111110100, 0b111011000000, // e
	0b001011010010, 0b111111010010, 0b010010000000, // f
	0b000000011111, 0b101101101111, 0b011001111110, // g
	0b100100110111, 0b101101101101, 0b101101000000, // h
	0b010010000010, 0b010010010010, 0b010010000000, // i
	0b010010000010, 0b010010010010, 0b010010110100, // j
	0b100100101101, 0b111110110101, 0b101101000000, // k
	0b110110010010, 0b010010010010, 0b010010000000, // l
	0b000000101111, 0b111111101101, 0b101101000000, // m
	0b000000110111, 0b101101101101, 0b101101000000, // n
	0b000000010111, 0b101101101101, 0b111010000000, // o
	0b000000110111, 0b101101101101, 0b111110100100, // p
	0b000000011111, 0b101101101101, 0b111011001001, // q
	0b000000101111, 0b110100100100, 0b100100000000, // r
	0b000000011111, 0b100110011001, 0b111110000000, // s
	0b010010111111, 0b010010010010, 0b011001000000, // t
	0b000000101101, 0b101101101101, 0b111011000000, // u
	0b000000101101, 0b101101101111, 0b010010000000, // v
	0b000000101101, 0b101101111111, 0b111101000000, // w
	0b000000101101, 0b111010010111, 0b101101000000, // x
	0b000000101101, 0b101101101111, 0b011001111110, // y
	0b000000111111, 0b001010010100, 0b111111000000, // z
	0b001011010010, 0b010100100010, 0b010010011001, // {
	0b010010010010, 0b010010010010, 0b010010010010, // |
	0b100110010010, 0b010001001010, 0b010010110100, // }
	0b000000000000, 0b001111111100, 0b000000000000, // ~
	// *** END PRINTABLE CHARACTERS ***
	0,0,0, // Delete
};

__attribute__((section(".noinit")))
static uint8_t textBuffer[MaxRowCount * 18 * GlyphHeight];

static uint16_t* screenBufer7800 = 0;
void Enable7800Mode(uint8_t* screenBufer)
{
	screenBufer7800 = (uint16_t * )screenBufer;
}

const uint16_t SmallLookup160b[16] =
{
	0x0000,
	0x2000,
	0x8000,
	0xa000,
	0x0020,
	0x2020,
	0x8020,
	0xa020,
	0x0080,
	0x2080,
	0x8080,
	0xa080,
	0x00a0,
	0x20a0,
	0x80a0,
	0xa0a0,
};


void PrintSmall(int row, int col, int len, const char* ptext) 
{
	int index;
	int inc;
	if (screenBufer7800)
	{
		index = row * 40 * GlyphHeight + col + 3;
		inc = 40 - len;
	}
	else
	{
		index = row * 18 * GlyphHeight + col / 2;
		inc = 18 - len/2;
	}
	for (int offset = 0; offset < 3; offset++)
	{
		for (int y = 3; y >= 0; y--)
		{
			for (int x = 0; x < len;)
			{
				if (screenBufer7800)
				{
					screenBufer7800[index++] = SmallLookup160b[(font[(int)ptext[x++] * 3 + offset] >> (y * 3)) & 7];
				}
				else
				{
					uint8_t b = ((font[(int)ptext[x++] * 3 + offset] >> (y * 3)) & 7) << 4;
					b |= (font[(int)ptext[x++] * 3 + offset] >> (y * 3)) & 7;
					textBuffer[index++] = b;
				}
			}
			index += inc;
		}
	}
}

const uint8_t WideLookup[8] =
{
	0x00,
	0x03,
	0x0c,
	0x0f,
	0x30,
	0x33,
	0x3c,
	0x3f
};

const uint16_t LargeLookup160b[4] =
{
	0x0000,
	0xa000,
	0x00a0,
	0xa0a0,
};

void PrintLarge(int row, int col, int len, const char* ptext)
{
	int index;
	int inc;
	if (screenBufer7800)
	{
		index = row * 40 * GlyphHeight + 2 * col + 3;
		inc = 40 - len * 2;
	}
	else
	{
		index = row * 18 * GlyphHeight + col;
		inc = 18 - len;
	}
	for (int offset = 0; offset < 3; offset++)
	{
		for (int y = 3; y >= 0; y--)
		{
			for (int x = 0; x < len;)
			{
				uint8_t b = ((font[(int)ptext[x++] * 3 + offset] >> (y * 3)) & 7);
				if (screenBufer7800) {
					screenBufer7800[index++] = LargeLookup160b[b >> 2];
					screenBufer7800[index++] = LargeLookup160b[b & 0x3];
				}
				else {
					textBuffer[index++] = WideLookup[b];
				}
			}
			index += inc;
		}
	}
}

void DisplayText(int RowCount)
{
	frameCount++;
	int frameToggle = frameCount & 1;

	vcsLda2(0);//	lda #$0
	vcsSta3(PF0);//	sta PF0
	vcsSta3(PF1);//	sta PF1
	vcsSta3(PF2);//	sta PF2
	vcsSta3(ENAM0);
	vcsSta3(ENAM1);
	vcsSta3(ENABL);
	vcsSta3(VDELP0);
	vcsSta3(VDELP1);
	vcsWrite5(COLUBK, 0x00);//	lda #$04//	sta COLUBK
	vcsWrite5(COLUP0, 0x0c);//	lda #$88//	sta COLUP0
	vcsWrite5(COLUP1, 0x0c);//	sta COLUP1
	vcsWrite5(NUSIZ0, 3);
	vcsWrite5(NUSIZ1, 3);

	vcsSta3(WSYNC);//	sta WSYNC
	if (frameToggle)
	{
		vcsNop2n(17);
		vcsWrite5(HMP1, 0xe0);
	}
	else
	{
		vcsNop2n(16);
		vcsWrite5(HMP1, 0xf0);
	}
	vcsSta4(RESPONE);//	sta RESP1
	vcsSta3(WSYNC);//	sta WSYNC
	vcsSta3(HMOVE);//	sta HMOVE
	vcsSta3(WSYNC);//	sta WSYNC

	vcsSta3(HMCLR);//	sta HMCLR
	vcsJmp3();
	vcsNop2n(32);
	// Need to position P0 just right in blank lines to avoid it showing up on the next line in the left margin
	if (frameToggle)
	{
		vcsJmp3();
		vcsSta3(RESP0);
		// 192 lines
		for (int i = 0; ;)
		{
			for (int j = 0; j < GlyphHeight; )
			{
				kernelB(&textBuffer[i * 18 * GlyphHeight + j * 18]);
				j++;
				kernelA(&textBuffer[i * 18 * GlyphHeight + j * 18]);
				j++;
			}

			vcsSta3(HMOVE);
			vcsWrite5(GRP1, 0);
			vcsWrite5(GRP0, 0);
			vcsNop2n(15);
			vcsWrite5(HMP1, 0x70);
			vcsSta3(RESP0);
			vcsJmp3();
			vcsSta3(RESP0);
			vcsNop2n(5);
			vcsJmp3();
			vcsSta3(RESP0); // Simulates Kernel B
			vcsJmp3();
			i++;
			if (i == RowCount) { break; }

			for (int j = 0; j < GlyphHeight;)
			{
				kernelA(&textBuffer[i * 18 * GlyphHeight + j * 18]);
				j++;
				kernelB(&textBuffer[i * 18 * GlyphHeight + j * 18]);
				j++;
			}

			vcsSta3(HMOVE);
			vcsWrite5(GRP0, 0);
			vcsWrite5(GRP1, 0);
			vcsNop2n(18);
			vcsSta3(RESP0);
			vcsJmp3();
			vcsSta3(RESP0);
			vcsNop2n(5);
			vcsWrite5(HMP1, 0x90);
			vcsSta3(RESP0); // Simulates Kernel A
			i++;
			if (i == RowCount) { break; }

		}
	}
	else
	{
		vcsSta3(RESP0);
		vcsJmp3();
		// 192 lines
		for (int i = 0; ;)
		{
			for (int j = 0; j < GlyphHeight;)
			{
				kernelA(&textBuffer[i * 18 * GlyphHeight + j * 18]);
				j++;
				kernelB(&textBuffer[i * 18 * GlyphHeight + j * 18]);
				j++;
			}

			vcsSta3(HMOVE);
			vcsWrite5(GRP0, 0);
			vcsWrite5(GRP1, 0);
			vcsNop2n(18);
			vcsSta3(RESP0);
			vcsJmp3();
			vcsSta3(RESP0);
			vcsNop2n(5);
			vcsWrite5(HMP1, 0x90);
			vcsSta3(RESP0); // Simulates Kernel A
			i++;
			if (i == RowCount) { break; }

			for (int j = 0; j < GlyphHeight;)
			{
				kernelB(&textBuffer[i * 18 * GlyphHeight + j * 18]);
				j++;
				kernelA(&textBuffer[i * 18 * GlyphHeight + j * 18]);
				j++;
			}

			vcsSta3(HMOVE);
			vcsWrite5(GRP1, 0);
			vcsWrite5(GRP0, 0);
			vcsNop2n(15);
			vcsWrite5(HMP1, 0x70);
			vcsSta3(RESP0);
			vcsJmp3();
			vcsSta3(RESP0);
			vcsNop2n(5);
			vcsJmp3();
			vcsSta3(RESP0); // Simulates Kernel B
			vcsJmp3();

			i++;
			if (i == RowCount) { break; }
		}
	}
}

// 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ
// 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7
static void kernelA(uint8_t textBuffer[18])
{
	vcsSta3(HMOVE);
	vcsWrite5(VBLANK, 0);
	vcsNop2();
	vcsLdy2(textBuffer[13] << 1);				// qqq_rrr_
	vcsWrite5(GRP0, textBuffer[0]);			// _000_111
	vcsLda2(textBuffer[6] << 1);				// ccc_ddd_
	vcsSta3(GRP1);
	vcsSta3(RESP0);
	vcsWrite5(GRP0, textBuffer[2]);			// _444_555
	vcsWrite5(GRP0, textBuffer[4]);			// _888_999
	vcsWrite5(HMP1, 0x90);
	vcsLdx2(textBuffer[10] << 1);				// kkk_lll_
	vcsWrite5(GRP1, textBuffer[8] << 1);	// ggg_hhh_
	vcsStx4(GRP1);
	vcsSta3(RESP0);
	vcsSty3(GRP0);
	vcsSta3(RESP0);
	vcsWrite5(GRP0, textBuffer[15] << 1);	// uuu_vvv_
	vcsWrite5(GRP0, textBuffer[17] << 1);	// yyy_zzz_
	vcsJmp3();//	SLEEP 3
	vcsSta3(RESP0);//	sta RESP0 
}

static void kernelB(uint8_t textBuffer[18])
{
	vcsSta3(HMOVE);
	vcsWrite5(VBLANK, 0);
	vcsWrite5(GRP1, textBuffer[7]);			// _eee_fff
	vcsLdx2(textBuffer[11]);					// _mmm_nnn
	vcsWrite5(HMP1, 0x70);
	vcsWrite5(GRP0, textBuffer[1] << 1);	// 222_333_
	vcsSta3(RESP0);
	vcsWrite5(GRP0, textBuffer[3] << 1);	// 666_777_
	vcsWrite5(GRP0, textBuffer[5] << 1);	// aaa_bbb_
	vcsWrite5(GRP0, textBuffer[12]);			// _ooo_ppp
	vcsWrite5(GRP1, textBuffer[9]);			// _iii_jjj
	vcsSta3(RESP0);
	vcsStx3(GRP1);
	vcsSta3(RESP0);
	vcsWrite5(GRP0, textBuffer[14]);			// _sss_ttt
	vcsWrite5(GRP0, textBuffer[16]);			// _www_xxx
	vcsJmp3();
	vcsSta3(RESP0);
	vcsJmp3();
}