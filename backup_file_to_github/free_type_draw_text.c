#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <freetype2/freetype/config/ftheader.h>
#include <freetype2/freetype/freetype.h>
#include <freetype2/freetype/ftglyph.h>
#include <wchar.h>

#define NIL 0x0000

typedef struct
{
	unsigned short code;
	unsigned short isolated;
	unsigned short initial;
	unsigned short medial;
	unsigned short final;
} ArabicCharRep;

typedef struct
{
	unsigned short code[2];
	unsigned short isolated;
	unsigned short initial;
	unsigned short medial;
	unsigned short final;
} ArabicCombineCharRep;

static ArabicCharRep arabic_tramsform_chars_map[] =
{
	{0x0621, 0xFE80, NIL   , NIL   , NIL   }, /* ARABIC LETTER HAMZA */
	{0x0622, 0xFE81, NIL   , NIL   , 0xFE82}, /* ARABIC LETTER ALEF WITH MADDA ABOVE */
	{0x0623, 0xFE83, NIL   , NIL   , 0xFE84}, /* ARABIC LETTER ALEF WITH HAMZA ABOVE */
	{0x0624, 0xFE85, NIL   , NIL   , 0xFE86}, /* ARABIC LETTER WAW WITH HAMZA ABOVE */
	{0x0625, 0xFE87, NIL   , NIL   , 0xFE88}, /* ARABIC LETTER ALEF WITH HAMZA BELOW */
	{0x0626, 0xFE89, 0xFE8B, 0xFE8C, 0xFE8A}, /* ARABIC LETTER YEH WITH HAMZA ABOVE */
	{0x0627, 0xFE8D, NIL   , NIL   , 0xFE8E}, /* ARABIC LETTER ALEF */
	{0x0628, 0xFE8F, 0xFE91, 0xFE92, 0xFE90}, /* ARABIC LETTER BEH */
	{0x0629, 0xFE93, NIL   , NIL   , 0xFE94}, /* ARABIC LETTER TEH MARBUTA */
	{0x062A, 0xFE95, 0xFE97, 0xFE98, 0xFE96}, /* ARABIC LETTER TEH */
	{0x062B, 0xFE99, 0xFE9B, 0xFE9C, 0xFE9A}, /* ARABIC LETTER THEH */
	{0x062C, 0xFE9D, 0xFE9F, 0xFEA0, 0xFE9E}, /* ARABIC LETTER JEEM */
	{0x062D, 0xFEA1, 0xFEA3, 0xFEA4, 0xFEA2}, /* ARABIC LETTER HAH */
	{0x062E, 0xFEA5, 0xFEA7, 0xFEA8, 0xFEA6}, /* ARABIC LETTER KHAH */
	{0x062F, 0xFEA9, NIL   , NIL   , 0xFEAA}, /* ARABIC LETTER DAL */
	{0x0630, 0xFEAB, NIL   , NIL   , 0xFEAC}, /* ARABIC LETTER THAL */
	{0x0631, 0xFEAD, NIL   , NIL   , 0xFEAE}, /* ARABIC LETTER REH */
	{0x0632, 0xFEAF, NIL   , NIL   , 0xFEB0}, /* ARABIC LETTER ZAIN */
	{0x0633, 0xFEB1, 0xFEB3, 0xFEB4, 0xFEB2}, /* ARABIC LETTER SEEN */
	{0x0634, 0xFEB5, 0xFEB7, 0xFEB8, 0xFEB6}, /* ARABIC LETTER SHEEN */
	{0x0635, 0xFEB9, 0xFEBB, 0xFEBC, 0xFEBA}, /* ARABIC LETTER SAD */
	{0x0636, 0xFEBD, 0xFEBF, 0xFEC0, 0xFEBE}, /* ARABIC LETTER DAD */
	{0x0637, 0xFEC1, 0xFEC3, 0xFEC4, 0xFEC2}, /* ARABIC LETTER TAH */
	{0x0638, 0xFEC5, 0xFEC7, 0xFEC8, 0xFEC6}, /* ARABIC LETTER ZAH */
	{0x0639, 0xFEC9, 0xFECB, 0xFECC, 0xFECA}, /* ARABIC LETTER AIN */
	{0x063A, 0xFECD, 0xFECF, 0xFED0, 0xFECE}, /* ARABIC LETTER GHAIN */
	{0x0640, 0x0640, NIL   , NIL   , NIL   }, /* ARABIC LETTER TATWEEL */
	{0x0641, 0xFED1, 0xFED3, 0xFED4, 0xFED2}, /* ARABIC LETTER FEH */
	{0x0642, 0xFED5, 0xFED7, 0xFED8, 0xFED6}, /* ARABIC LETTER QAF */
	{0x0643, 0xFED9, 0xFEDB, 0xFEDC, 0xFEDA}, /* ARABIC LETTER KAF */
	{0x0644, 0xFEDD, 0xFEDF, 0xFEE0, 0xFEDE}, /* ARABIC LETTER LAM */
	{0x0645, 0xFEE1, 0xFEE3, 0xFEE4, 0xFEE2}, /* ARABIC LETTER MEEM */
	{0x0646, 0xFEE5, 0xFEE7, 0xFEE8, 0xFEE6}, /* ARABIC LETTER NOON */
	{0x0647, 0xFEE9, 0xFEEB, 0xFEEC, 0xFEEA}, /* ARABIC LETTER HEH */
	{0x0648, 0xFEED, NIL   , NIL   , 0xFEEE}, /* ARABIC LETTER WAW */
	{0x0649, 0xFEEF, 0xFEF3, 0xFEF4, 0xFEF0}, /* ARABIC LETTER ALEF MAKSURA */
	{0x064A, 0xFEF1, 0xFEF3, 0xFEF4, 0xFEF2}, /* ARABIC LETTER YEH */
	{0x0679, 0xFB66, 0xFB68, 0xFB69, 0xFB67}, /* ARABIC LETTER TTEH */
	{0x067E, 0xFB56, 0xFB58, 0xFB59, 0xFB57}, /* ARABIC LETTER PEH */
	{0x0686, 0xFB7A, 0xFB7C, 0xFB7D, 0xFB7B}, /* ARABIC LETTER TCHEH */
	{0x0688, 0xFB88, NIL   , NIL   , 0xFB89}, /* ARABIC LETTER DDAL */
	{0x0691, 0xFB8C, NIL   , NIL   , 0xFB8D}, /* ARABIC LETTER RREH */
	{0x0698, 0xFB8A, NIL   , NIL   , 0xFB8B}, /* ARABIC LETTER JEH */
	{0x06A9, 0xFB8E, 0xFB90, 0xFB91, 0xFB8F}, /* ARABIC LETTER KEHEH */
	{0x06AF, 0xFB92, 0xFB94, 0xFB95, 0xFB93}, /* ARABIC LETTER GAF */
	{0x06BA, 0xFB9E, 0xFEE7, 0xFEE8, 0xFB9F}, /* ARABIC LETTER NOON GHUNNA */
	{0x06BE, 0xFBAA, 0xFBAC, 0xFBAD, 0xFBAB}, /* ARABIC LETTER HEH DOACHASHMEE */
	{0x06C1, 0xFBA6, 0xFBA8, 0xFBA9, 0xFBA7}, /* ARABIC LETTER HEH GOAL */
	{0x06CC, 0xFEEF, 0xFEF3, 0xFEF4, 0xFEF0}, /* ARABIC LETTER FARSI YEH */
};

static ArabicCombineCharRep arabic_combine_chars_map[] =
{
	{{0x0644, 0x0622}, 0xFEF5, NIL, NIL, 0xFEF6}, /* LAM_ALEF_MADDA */
	{{0x0644, 0x0623}, 0xFEF7, NIL, NIL, 0xFEF8}, /* LAM_ALEF_HAMZA_ABOVE */
	{{0x0644, 0x0625}, 0xFEF9, NIL, NIL, 0xFEFA}, /* LAM_ALEF_HAMZA_BELOW */
	{{0x0644, 0x0627}, 0xFEFB, NIL, NIL, 0xFEFC}  /* LAM_ALEF */
};

static unsigned int arabic_trans_chars[] =
{
	0x0610, /* ARABIC SIGN SALLALLAHOU ALAYHE WASSALLAM */
	0x0612, /* ARABIC SIGN ALAYHE ASSALLAM */
	0x0613, /* ARABIC SIGN RADI ALLAHOU ANHU */
	0x0614, /* ARABIC SIGN TAKHALLUS */
	0x0615, /* ARABIC SMALL HIGH TAH */
	0x064B, /* ARABIC FATHATAN */
	0x064C, /* ARABIC DAMMATAN */
	0x064D, /* ARABIC KASRATAN */
	0x064E, /* ARABIC FATHA */
	0x064F, /* ARABIC DAMMA */
	0x0650, /* ARABIC KASRA */
	0x0651, /* ARABIC SHADDA */
	0x0652, /* ARABIC SUKUN */
	0x0653, /* ARABIC MADDAH ABOVE */
	0x0654, /* ARABIC HAMZA ABOVE */
	0x0655, /* ARABIC HAMZA BELOW */
	0x0656, /* ARABIC SUBSCRIPT ALEF */
	0x0657, /* ARABIC INVERTED DAMMA */
	0x0658, /* ARABIC MARK NOON GHUNNA */
	0x0670, /* ARABIC LETTER SUPERSCRIPT ALEF */
	0x06D6, /* ARABIC SMALL HIGH LIGATURE SAD WITH LAM WITH ALEF MAKSURA */
	0x06D7, /* ARABIC SMALL HIGH LIGATURE QAF WITH LAM WITH ALEF MAKSURA */
	0x06D8, /* ARABIC SMALL HIGH MEEM INITIAL FORM */
	0x06D9, /* ARABIC SMALL HIGH LAM ALEF */
	0x06DA, /* ARABIC SMALL HIGH JEEM */
	0x06DB, /* ARABIC SMALL HIGH THREE DOTS */
	0x06DC, /* ARABIC SMALL HIGH SEEN */
	0x06DF, /* ARABIC SMALL HIGH ROUNDED ZERO */
	0x06E0, /* ARABIC SMALL HIGH UPRIGHT RECTANGULAR ZERO */
	0x06E1, /* ARABIC SMALL HIGH DOTLESS HEAD OF KHAH */
	0x06E2, /* ARABIC SMALL HIGH MEEM ISOLATED FORM */
	0x06E3, /* ARABIC SMALL LOW SEEN */
	0x06E4, /* ARABIC SMALL HIGH MADDA */
	0x06E7, /* ARABIC SMALL HIGH YEH */
	0x06E8, /* ARABIC SMALL HIGH NOON */
	0x06EA, /* ARABIC EMPTY CENTRE LOW STOP */
	0x06EB, /* ARABIC EMPTY CENTRE HIGH STOP */
	0x06EC, /* ARABIC ROUNDED HIGH STOP WITH FILLED CENTRE */
	0x06ED  /* ARABIC SMALL LOW MEEM */
};

static int arabic_tramsform_map_contains(unsigned int char_code)
{
	int index;

	for (index = 0; index < sizeof(arabic_tramsform_chars_map) / sizeof(arabic_tramsform_chars_map[0]); index++)
	{
		if (arabic_tramsform_chars_map[index].code == char_code)
		{
			return 1;
		}
	}
	return 0;
}

static ArabicCharRep get_arabic_transform_char_rep(unsigned int char_code)
{
	ArabicCharRep char_rep = {NIL, NIL, NIL, NIL, NIL};
	int index;

	for (index = 0; index < sizeof(arabic_tramsform_chars_map) / sizeof(arabic_tramsform_chars_map[0]); index++)
	{
		if (arabic_tramsform_chars_map[index].code == char_code)
		{
			char_rep = arabic_tramsform_chars_map[index];
			break;
		}
	}
	return char_rep;
}

static ArabicCombineCharRep get_arabic_combine_char_rep(unsigned int char_code1, unsigned int char_code2)
{
	ArabicCombineCharRep combine_char_rep = {{NIL, NIL}, NIL, NIL, NIL, NIL};
	int index = 0;

	for (index = 0; index < sizeof(arabic_combine_chars_map) / sizeof(arabic_combine_chars_map[0]); index++)
	{
		if (arabic_combine_chars_map[index].code[0] == char_code1 && arabic_combine_chars_map[index].code[1] == char_code2)
		{
			combine_char_rep = arabic_combine_chars_map[index];
			break;
		}
	}
	return combine_char_rep;
}

static int is_arabic_transparent_char(unsigned int char_code)
{
	int index = 0;

	for (index = 0; index < sizeof(arabic_trans_chars) / sizeof(arabic_trans_chars[0]); index++)
	{
		if (arabic_trans_chars[index] == char_code)
		{
			return 1;
		}
	}
	return 0;
}

static int is_neutral_char(unsigned int char_code)
{
	if ((char_code >= 0x20 && char_code <= 0x2C)
		|| (char_code >= 0x2E && char_code <= 0x2F)
		|| (char_code >= 0x3A && char_code <= 0x40)
		|| (char_code >= 0x5B && char_code <= 0x5E)
		|| (char_code == 0x60)
		|| (char_code >= 0x7B && char_code <= 0x7E))
	{
		return 1;
	}
	return 0;
}

static int is_rtl_char(unsigned int char_code)
{
	/* arabic digit, need use left to right direction */
	if(char_code >= 0x660 && char_code <= 0x669)
	{
		return 0;
	}
	if((char_code >= 0x0600 && char_code <= 0x077F) || (char_code >= 0xFB50 && char_code <= 0xFEFF))
	{
		return 1;
	}
	return 0;
}

static int arabic_text_transform(unsigned int *dst, unsigned int *src)
{
	int src_length;
	int src_index;
	int dst_index;
	unsigned int current_char;
	ArabicCharRep char_rep;
	ArabicCombineCharRep combine_rep;

	src_length = wcslen(src);
	dst_index = 0;
	for (src_index = 0; src_index < src_length; ++src_index)
	{
		current_char = src[src_index];
		if (arabic_tramsform_map_contains(current_char))
		{
			unsigned int prev = NIL;
			unsigned int next = NIL;
			int prev_id = src_index - 1;
			int next_id = src_index + 1;

			/*
			 * Transparent characters have no effect in the shaping process.
			 * So, ignore all the transparent characters that are BEFORE the
			 * current character.
			 */
			for (; prev_id >= 0; prev_id--)
			{
				if (!is_arabic_transparent_char(src[prev_id]))
				{
					break;
				}
			}
			if ((prev_id < 0) || !arabic_tramsform_map_contains(prev = src[prev_id])
				|| (!((char_rep = get_arabic_transform_char_rep(prev)).initial != NIL) && !(char_rep.medial != NIL)))
			{
				prev = NIL;
			}

			/*
			 * Transparent characters have no effect in the shaping process.
			 * So, ignore all the transparent characters that are AFTER the
			 * current character.
			 */
			for (; next_id < src_length; next_id++)
			{
				if (!is_arabic_transparent_char(src[next_id]))
				{
					break;
				}
			}
			if ((next_id >= src_length) || !arabic_tramsform_map_contains(next = src[next_id])
				|| (!((char_rep = get_arabic_transform_char_rep(next)).medial != NIL) && !(char_rep.final != NIL) && (next != 0x0640)))
			{
				next = NIL;
			}

			/* Combinations */
			if (current_char == 0x0644 && (next == 0x0622 || next == 0x0623 || next == 0x0625 || next == 0x0627))
			{
				combine_rep = get_arabic_combine_char_rep(current_char, next);
				if (prev != NIL)
				{
					dst[dst_index++] = combine_rep.final;
				}
				else
				{
					dst[dst_index++] = combine_rep.isolated;
				}
				src_index++;
				continue;
			}

			char_rep = get_arabic_transform_char_rep(current_char);
			if (prev != NIL && next != NIL && char_rep.medial != NIL)	/* Medial */
			{
				dst[dst_index++] = char_rep.medial;
				continue;
			}
			else if (prev != NIL && char_rep.final != NIL)	/* Final */
			{
				dst[dst_index++] = char_rep.final;
				continue;
			}
			else if (next != NIL && char_rep.initial != NIL)	/* Initial */
			{
				dst[dst_index++] = char_rep.initial;
				continue;
			}
			/* Isolated */
			dst[dst_index++] = char_rep.isolated;
		}
		else
		{
			dst[dst_index++] = current_char;
		}
	}
	dst[dst_index] = NIL;

	return 0;
}

static int s_get_one_utf8_size(unsigned char Input)
{
	int firstch = Input;
	int temp = 0x80;
	int num = 0;
	while (temp & firstch)
	{
		num++;
		temp = (temp >> 1);
	}
	return num;
}

static int s_one_utf8_to_unicode(unsigned char* input, int utfbytes, unsigned int *uni_code)
{
	// b1 表示UTF-8编码的pInput中的高字节, b2 表示次高字节, ...
	unsigned char b1, b2, b3, b4, b5, b6;
	unsigned char *output = (unsigned char *)uni_code;

	switch (utfbytes)
	{
		case 0:
			*output = *input;
			*(output + 1) = 0;
			utfbytes += 1;
			break;
		case 2:
			b1 = *input;
			b2 = *(input + 1);
			if((b2 & 0xC0) != 0x80)  //此高位10xx xxxx
				return 0;
			*output = (b1 << 6) + (b2 & 0x3F);
			*(output + 1) = (b1 >> 2) & 0x07;  //2位范围0000 0080-0000 07ff
			break;
		case 3:
			b1 = *input;
			b2 = *(input + 1);
			b3 = *(input + 2);
			if(((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80))
				return 0;
			*output = (b2 << 6) + (b3 & 0x3F);
			*(output + 1) = (b1 << 4) + ((b2 >> 2) & 0x0F);
			break;
		case 4:
			b1 = *input;
			b2 = *(input + 1);
			b3 = *(input + 2);
			b4 = *(input + 3);
			if(((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80) || ((b4 & 0xC0) != 0x80))
				return 0;
			*output = (b3 << 6) + (b4 & 0x3F);
			*(output + 1) = (b2 << 4) + ((b3 >> 2) & 0x0F);
			*(output + 2) = ((b1 << 2) & 0x1C) + ((b2 >> 4) & 0x03);
			break;
		case 5:
			b1 = *input;
			b2 = *(input + 1);
			b3 = *(input + 2);
			b4 = *(input + 3);
			b5 = *(input + 4);
			if(((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80) || ((b4 & 0xC0) != 0x80) || ((b5 & 0xC0) != 0x80))
				return 0;
			*output = (b4 << 6) + (b5 & 0x3F);
			*(output + 1) = (b3 << 4) + ((b4 >> 2) & 0x0F);
			*(output + 2) = (b2 << 2) + ((b3 >> 4) & 0x03);
			*(output + 3) = (b1 << 6);
			break;
		case 6:
			b1 = *input;
			b2 = *(input + 1);
			b3 = *(input + 2);
			b4 = *(input + 3);
			b5 = *(input + 4);
			b6 = *(input + 5);
			if(((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80) || ((b4 & 0xC0) != 0x80) || ((b5 & 0xC0) != 0x80) || ((b6 & 0xC0) != 0x80))
				return 0;
			*output = (b5 << 6) + (b6 & 0x3F);
			*(output + 1) = (b5 << 4) + ((b6 >> 2) & 0x0F);
			*(output + 2) = (b3 << 2) + ((b4 >> 4) & 0x03);
			*(output + 3) = ((b1 << 6) & 0x40) + (b2 & 0x3F);
			break;
		default:
			return 0;
			break;
	}
	return utfbytes;
}

static int s_utf8_to_unicode(const char* src, unsigned int *des)
{
	if(src == NULL || des == NULL)
	{
		return -1;
	}
	int len = strlen(src);
	int n = 0;
	int byte = 0;
	int i = 0;

	for(n = 0; n < len; )
	{
		byte = s_get_one_utf8_size(src[n]);
		*(des + i) = 0;
		byte = s_one_utf8_to_unicode((unsigned char *)(src + n), byte, des + i);
		if(byte == 0)
		{
			return 1;
		}
		n += byte;
		i++;
	}

	return 0;
}

#define TTF_FILE_NAME ("test.ttf")
#define PIXEL_WIDTH (24)
#define PIXEL_HEIGHT (24)

static unsigned int swap_bracket_pair(unsigned int char_code)
{
	unsigned int left_bracket[] = {'(', '{', '[', '<'};
	unsigned int right_bracket[] = {')', '}', ']', '>'};
	int i;

	for (i = 0; i < sizeof(left_bracket) / sizeof(left_bracket[0]); i++)
	{
		if (char_code == left_bracket[i])
		{
			return right_bracket[i];
		}
		else if (char_code == right_bracket[i])
		{
			return left_bracket[i];
		}
	}
	return char_code;
}

static void swap_ucs2_text_bracket_pair(unsigned int *ucs2_text, int count)
{
	int index;

	for (index = 0; index < count; ++index)
	{
		ucs2_text[index] = swap_bracket_pair(ucs2_text[index]);
	}
}

static int s_swap_ucs_str(unsigned int *ucs2_text, int count)
{
	int index = 0;
	unsigned int tmp = 0;

	for(index = 0; index < count / 2; index++)
	{
		tmp = ucs2_text[index];
		ucs2_text[index] = ucs2_text[count - 1 - index];
		ucs2_text[count - 1 - index] = tmp;
	}
	return 0;
}

static int s_swap_ucs_str_by_auto_direction(unsigned int *ucs2_text)
{
	int text_length = 0;
	int index = 0;
	int text_dir = -1;
	int tmp_dir = 0;
	int start_index = 0;
	int text_seq_length = 0;
	int swap_str = 0;

	text_length = wcslen(ucs2_text);
	for(index = 0; index < text_length; index++)
	{
		swap_str = 0;
		tmp_dir = is_rtl_char(ucs2_text[index]);
		if(tmp_dir)
		{
			if(text_dir == -1)
			{
				text_dir = tmp_dir;
				start_index = index;
			}
		}
		else
		{
			if(is_neutral_char(ucs2_text[index]) == 0)
			{
				if(text_dir == 1)
				{
					swap_str = 1;
				}
				text_dir = -1;
			}
		}

		if((index == (text_length - 1)) && text_dir == 1)
		{
			swap_str = 1;
		}

		if(swap_str)
		{
			text_seq_length = index - start_index + 1;
			if(text_seq_length > 1)
			{
				s_swap_ucs_str(&ucs2_text[start_index], text_seq_length);
				swap_ucs2_text_bracket_pair(&ucs2_text[start_index], text_seq_length);
			}
		}
	}

	return 0;
}

static int get_transformed_ucs2_text_width(unsigned int *ucs2_text, float x_times, float y_times)
{
	int n, num_chars;
	FT_Library library;
	FT_Error error;
	FT_Face face;
	double angle = 0;
	FT_Matrix matrix;
	FT_Vector pen;
	int width = 0;

	if(ucs2_text == NULL)
	{
		return width;
	}

	error = FT_Init_FreeType(&library);
	error = FT_New_Face(library, TTF_FILE_NAME, 0, &face);
	error = FT_Set_Pixel_Sizes(face, PIXEL_WIDTH * x_times, PIXEL_HEIGHT * y_times);

	matrix.xx = (FT_Fixed)(cos(angle) * 0x10000L);
	matrix.xy = (FT_Fixed)(-sin(angle) * 0x10000L);
	matrix.yx = (FT_Fixed)(sin(angle) * 0x10000L);
	matrix.yy = (FT_Fixed)(cos(angle) * 0x10000L);

	pen.x = 0;
	pen.y = 0;

	num_chars = wcslen(ucs2_text);

	for(n = 0; n < num_chars; n++)
	{
		FT_Set_Transform(face, &matrix, &pen);
		error = FT_Load_Char(face, ucs2_text[n], FT_LOAD_RENDER);
		if(error)
			continue;

		pen.x += face->glyph->advance.x;
		pen.y += face->glyph->advance.y;

		width += (face->glyph->advance.x / 64);
	}

	FT_Done_Face(face);
	FT_Done_FreeType(library);

	return width;
}

static int s_mk_ft_draw_bitmap(FT_Bitmap* bitmap, int x, int y, int color)
{
	HI_RECT DstRect = {0, 0, 0, 0};
	HI_S32 s32Ret = HI_FAILURE;
	HI_HANDLE surface;
	HI_U8 *pu8Surface = NULL;
	HI_PIXELDATA pData;
	HIGO_BLTOPT_S stBlitOpt;
	int p, q;

	DstRect.x = x;
	DstRect.y = y;
	DstRect.w = bitmap->width;
	DstRect.h = bitmap->rows;

	s32Ret = HI_GO_CreateSurface(bitmap->width, bitmap->rows, HIGO_PF_8888, &surface);
	if(s32Ret != HI_SUCCESS)
	{
		return 1;
	}

	HI_GO_LockSurface(surface, pData, HI_TRUE);

	pu8Surface = (HI_U8*)pData[0].pData;
	if (NULL == pu8Surface)
	{
		HI_GO_UnlockSurface(surface);
		HI_GO_FreeSurface(surface);
		return 2;
	}

	for(p = 0; p < bitmap->rows; p++)
	{
		for(q = 0; q < bitmap->width; q++)
		{
			pu8Surface[p * pData[0].Pitch + 4 * q + 3] = bitmap->buffer[p * bitmap->width + q];
			pu8Surface[p * pData[0].Pitch + 4 * q + 2] = (color >> 16) & 0xff;
			pu8Surface[p * pData[0].Pitch + 4 * q + 1] = (color >> 8) & 0xff;
			pu8Surface[p * pData[0].Pitch + 4 * q + 0] = color & 0xff;
		}
	}

    memset(&stBlitOpt, 0, sizeof(HIGO_BLTOPT_S));
	stBlitOpt.EnableScale = HI_TRUE;
	stBlitOpt.EnablePixelAlpha = HI_TRUE;
	stBlitOpt.PixelAlphaComp = HIGO_COMPOPT_SRCOVER;

	HI_GO_Blit(surface, NULL, s_osd_surface, &DstRect, &stBlitOpt);
	HI_GO_UnlockSurface(surface);
	HI_GO_FreeSurface(surface);

	return 0;
}

static int s_draw_transformed_ucs_text_internal(int x, int y, unsigned int *ucs2_text, int text_color, float x_times, float y_times)
{
	int n = 0;
	FT_Library library;
	FT_Error error;
	FT_Face face;
	double angle = 0;
	FT_Matrix matrix;
	FT_Vector pen;
	int width = 0;
	int string_len = 0;
	int col = x;
	int row = y;
	int y_offset = 0;

	if(ucs2_text == NULL)
	{
		return width;
	}

	s_swap_ucs_str_by_auto_direction(ucs2_text);

	error = FT_Init_FreeType(&library);
	error = FT_New_Face(library, TTF_FILE_NAME, 0, &face);
	error = FT_Set_Pixel_Sizes(face, PIXEL_WIDTH * x_times, PIXEL_HEIGHT * y_times);

	matrix.xx = (FT_Fixed)(cos(angle) * 0x10000L);
	matrix.xy = (FT_Fixed)(-sin(angle) * 0x10000L);
	matrix.yx = (FT_Fixed)(sin(angle) * 0x10000L);
	matrix.yy = (FT_Fixed)(cos(angle) * 0x10000L);

	pen.x = 0;
	pen.y = 0;

	string_len = wcslen(ucs2_text);

	for(n = 0; n < string_len; n++)
	{
		FT_Set_Transform(face, &matrix, &pen);
		error = FT_Load_Char(face, ucs2_text[n], FT_LOAD_RENDER);
		if(error)
			continue;

		y_offset= PIXEL_HEIGHT * y_times - (face->size->metrics.ascender / 64 - face->size->metrics.descender / 64);
		s_mk_ft_draw_bitmap(&face->glyph->bitmap, col + face->glyph->metrics.horiBearingX / 64,
		        row + face->size->metrics.ascender / 64 - face->glyph->bitmap_top + y_offset, text_color);

		pen.x += face->glyph->advance.x;
		pen.y += face->glyph->advance.y;

		col += (face->glyph->advance.x / 64);
		width += (face->glyph->advance.x / 64);
	}

	FT_Done_Face(face);
	FT_Done_FreeType(library);

	return width;
}

static int s_draw_ucs_text_internal(int x, int y, unsigned int *ucs_text, int text_color, float x_times, float y_times)
{
	int text_length = 0;
	int draw_width = 0;
	unsigned int *transformed_text = NULL;

	if(ucs_text == NULL)
	{
		return draw_width;
	}

	text_length = wcslen(ucs_text);
	if(text_length <= 0)
	{
		return draw_width;
	}
	transformed_text = malloc((text_length + 1) * sizeof(unsigned int));
	if(transformed_text == NULL)
	{
		return draw_width;
	}
	memset(transformed_text, 0, (text_length + 1) * sizeof(unsigned int));
	arabic_text_transform(transformed_text, ucs_text);

	draw_width = s_draw_transformed_ucs_text_internal(x, y, transformed_text, text_color, x_times, y_times);

	free(transformed_text);
	return draw_width;
}

static int s_draw_utf8_text_internal(int x, int y, const char *str, int text_color, float x_times, float y_times)
{
	unsigned int *uni_code = NULL;
	int num_chars = strlen(str);
	int n = 0;

	if(str == NULL)
	{
		return 1;
	}

	uni_code = malloc((num_chars + 1) * sizeof(unsigned int));
	if(uni_code == NULL)
	{
		return 2;
	}

	memset(uni_code, 0, (num_chars + 1) * sizeof(unsigned int));
	if(s_utf8_to_unicode(str, uni_code) != 0)
	{
		memset(uni_code, 0, (num_chars + 1) * sizeof(unsigned int));
		for(n = 0; n < num_chars; n++)
		{
			uni_code[n] = str[n];
		}
	}
	s_draw_ucs_text_internal(x, y, uni_code, text_color, x_times, y_times);
	free(uni_code);

	return 0;
}

static int s_get_ucs_text_width(unsigned int *ucs_text, float x_times, float y_times)
{
	int text_length = 0;
	int draw_width = 0;
	unsigned int *transformed_text = NULL;

	if(ucs_text == NULL)
	{
		return draw_width;
	}

	text_length = wcslen(ucs_text);
	if(text_length <= 0)
	{
		return draw_width;
	}
	transformed_text = malloc((text_length + 1) * sizeof(unsigned int));
	if(transformed_text == NULL)
	{
		return draw_width;
	}
	memset(transformed_text, 0, (text_length + 1) * sizeof(unsigned int));
	arabic_text_transform(transformed_text, ucs_text);

	draw_width = get_transformed_ucs2_text_width(transformed_text, x_times, y_times);

	free(transformed_text);
	return draw_width;
}

static int s_get_utf8_text_width_internal(char *str, float x_times, float y_times)
{
	unsigned int *uni_code = NULL;
	int num_chars = strlen(str);
	int n = 0;
	int width = 0;

	if(str == NULL)
	{
		return width;
	}

	uni_code = malloc((num_chars + 1) * sizeof(unsigned int));
	if(uni_code == NULL)
	{
		return width;
	}

	memset(uni_code, 0, (num_chars + 1) * sizeof(unsigned int));
	if(s_utf8_to_unicode(str, uni_code) != 0)
	{
		memset(uni_code, 0, (num_chars + 1) * sizeof(unsigned int));
		for(n = 0; n < num_chars; n++)
		{
			uni_code[n] = str[n];
		}
	}

	width = s_get_ucs_text_width(uni_code, x_times, y_times);
	free(uni_code);

	return width;
}

int mk_ft_draw_text_zoom(int x, int y, const char *str, int text_color, float x_times, float y_times)
{
	return s_draw_utf8_text_internal(x, y, str, text_color, x_times, y_times);
}

int mk_ft_get_text_width_height_zoom(char *str, int *w, int *h, float x_times, float y_times)
{
	if(w)
	{
		*w = s_get_utf8_text_width_internal(str, x_times, y_times);
	}

	if(h)
		*h = PIXEL_HEIGHT * y_times;

	return 0;
}

