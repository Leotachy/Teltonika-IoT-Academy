

#include <linux/init.h>
#include "lt_lcd_letter.h"
#include "util.h"

// Reference to ASCII characters
// https://github.com/idispatch/raster-fonts/blob/master/font-5x8.c

static const u8 a_nosine[] = {
	0x00, // 00000
	0x00, // 00000
	0x0E, // 01110
	0x01, // 00001
	0x0F, // 01111
	0x11, // 10001
	0x0F, // 01111
	0x04, // 00100
};

static const u8 ch[] = {
	0x0A, // 01010
	0x04, // 00100
	0x0E, // 01110
	0x11, // 10001
	0x10, // 10000
	0x11, // 10001
	0x0E, // 01110
	0x00, // 00000
};

static const u8 e_nosine[] = {
	0x00, // 00000
	0x00, // 00000
	0x0E, // 01110
	0x11, // 10001
	0x1F, // 11111
	0x10, // 10000
	0x0E, // 01110
	0x04, // 00100
};

static const u8 eee[] = {
	0x04, // 00100
	0x00, // 00000
	0x0E, // 01110
	0x11, // 10001
	0x1F, // 11111
	0x10, // 10000
	0x0E, // 01110
	0x00, // 00000
};

static const u8 i_nosine[] = {
	0x00, // 00000
	0x00, // 00000
	0x0C, // 01100
	0x04, // 00100
	0x04, // 00100
	0x04, // 00100
	0x0E, // 01110
	0x04, // 00100
};

static const u8 sh[] = {
	0x0A, // 01010
	0x04, // 00100
	0x0F, // 01111
	0x10, // 10000
	0x0E, // 01110
	0x01, // 00001
	0x1E, // 11110
	0x00, // 00000
};

static const u8 u_nosine[] = {
	0x00, // 00000
	0x00, // 00000
	0x11, // 10001
	0x11, // 10001
	0x11, // 10001
	0x11, // 10001
	0x0F, // 01111
	0x04, // 00100
};

static const u8 u_ilgoji[] = {
	0x1F, // 11111
	0x00, // 00000
	0x11, // 10001
	0x11, // 10001
	0x11, // 10001
	0x11, // 10001
	0x0E, // 01110
	0x00, // 00000
};

static const u8 zh[] = {
	0x0A, // 01010
	0x04, // 00100
	0x1F, // 11111
	0x02, // 00010
	0x04, // 00100
	0x08, // 01000
	0x1F, // 11111
	0x00, // 00000
};

static const u8 A_nosine[] = {
	0x00, // 00000
	0x0E, // 01110
	0x11, // 10001
	0x11, // 10001
	0x1F, // 11111
	0x11, // 10001
	0x11, // 10001
	0x04, // 00100
};

static const u8 CH[] = {
	0x0A, // 01010
	0x04, // 00100
	0x0E, // 01110
	0x11, // 10001
	0x10, // 10000
	0x11, // 10001
	0x0E, // 01110
	0x00, // 00000
};

static const u8 E_nosine[] = {
	0x1F, // 11111
	0x10, // 10000
	0x1E, // 11110
	0x10, // 10000
	0x1F, // 11111
	0x00, // 00000
	0x00, // 00000
	0x04, // 00100
};

static const u8 EEE[] = {
	0x04, // 00100
	0x00, // 00000
	0x1F, // 11111
	0x10, // 10000
	0x1E, // 11110
	0x10, // 10000
	0x1F, // 11111
	0x00, // 00000
};

static const u8 I_nosine[] = {
	0x1F, // 11111
	0x04, // 00100
	0x04, // 00100
	0x04, // 00100
	0x04, // 00100
	0x04, // 00100
	0x1F, // 11111
	0x04, // 00100
};

static const u8 SH[] = {
	0x0A, // 01010
	0x04, // 00100
	0x0F, // 01111
	0x10, // 10000
	0x0E, // 01110
	0x01, // 00001
	0x1E, // 11110
	0x00, // 00000
};

static const u8 U_nosine[] = {
	0x00, // 00000
	0x11, // 10001
	0x11, // 10001
	0x11, // 10001
	0x11, // 10001
	0x11, // 10001
	0x0E, // 01110
	0x04, // 00100
};

static const u8 U_ilgoji[] = {
	0x1F, // 11111
	0x00, // 00000
	0x11, // 10001
	0x11, // 10001
	0x11, // 10001
	0x11, // 10001
	0x0E, // 01110
	0x00, // 00000
};

static const u8 ZH[] = {
	0x0A, // 01010
	0x04, // 00100
	0x1F, // 11111
	0x02, // 00010
	0x04, // 00100
	0x08, // 01000
	0x1F, // 11111
	0x00, // 00000
};

const u8 *get_lithuanian_char_pixel_matrix(wchar_t wchar)
{
	const u8 *pixel_matrix = NULL;

	switch (wchar) {
	case L'ą':
		pixel_matrix = a_nosine;
		break;
	case L'č':
		pixel_matrix = ch;
		break;
	case L'ę':
		pixel_matrix = e_nosine;
		break;
	case L'ė':
		pixel_matrix = eee;
		break;
	case L'į':
		pixel_matrix = i_nosine;
		break;
	case L'š':
		pixel_matrix = sh;
		break;
	case L'ų':
		pixel_matrix = u_nosine;
		break;
	case L'ū':
		pixel_matrix = u_ilgoji;
		break;
	case L'ž':
		pixel_matrix = zh;
		break;
	case L'Ą':
		pixel_matrix = A_nosine;
		break;
	case L'Č':
		pixel_matrix = CH;
		break;
	case L'Ę':
		pixel_matrix = E_nosine;
		break;
	case L'Ė':
		pixel_matrix = EEE;
		break;
	case L'Į':
		pixel_matrix = I_nosine;
		break;
	case L'Š':
		pixel_matrix = SH;
		break;
	case L'Ų':
		pixel_matrix = U_nosine;
		break;
	case L'Ū':
		pixel_matrix = U_ilgoji;
		break;
	case L'Ž':
		pixel_matrix = ZH;
		break;
	default:
		break;
	}

	return pixel_matrix;
}