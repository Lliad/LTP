/* library.c */

#include "struct.h"

/* * * sda field * * */

void	encodeSdnv(Sdnv *sdnv, uvast val)
{
	static uvast	sdnvMask = ((uvast) -1) / 128;
	uvast		remnant = val;
	char		result[10];
	int		length = 1;
	unsigned char	*text;

	CHKVOID(sdnv);

	result[0] = remnant & (uvast) 0x7f;
	remnant = (remnant >> 7) & sdnvMask;

	while (remnant)
	{
		result[length] = (remnant & (uvast) 0x7f) | 0x80;
		remnant = (remnant >> 7) & sdnvMask;
		length++;
	}

	sdnv->length = length;
	text = sdnv->text;
	while (length)
	{
		length--;
		*text = result[length];
		text++;
	}
}

int	decodeSdnv(uvast *val, unsigned char *sdnvTxt)
{
	int		sdnvLength = 0;
	unsigned char	*cursor;

	CHKZERO(val);
	CHKZERO(sdnvTxt);
	*val = 0;
	cursor = sdnvTxt;

	while (1)
	{
		sdnvLength++;
		if (sdnvLength > 10)
		{
			return 0;	/*	More than 70 bits.	*/
		}

		/*	Shift numeric value 7 bits to the left (that
		 *	is, multiply by 128) to make room for 7 bits
		 *	of SDNV byte value.				*/

		*val <<= 7;

		/*	Insert SDNV text byte value (with its high-
		 *	order bit masked off) as low-order 7 bits of
		 *	the numeric value.				*/

		*val |= (*cursor & 0x7f);

		/*	If this SDNV text byte's high-order bit is
		 *	1, then it's the last byte of the SDNV text.	*/

		if (((*cursor) & 0x80) == 0)	/*	Last SDNV byte.	*/
		{
			return sdnvLength;
		}

		/*	Haven't reached the end of the SDNV yet.	*/

		cursor++;
	}
}