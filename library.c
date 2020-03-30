/* library.c ： func for use */

#include "library.h"
/* * * public field * * */


/* * * sda field * * */

void	encodeSdnv(Sdnv *sdnv, uvast val)
{
	static uvast	sdnvMask = ((uvast) -1) / 128;
	uvast		remnant = val;
	char		result[10];
	int		length = 1;
	unsigned char	*text;

	/*	Thanks to Cheol Koo of KARI for optimizing this
	 *	function.  29 August 2019				*/
	
	if(!sdnv)
	{
		return;
	}

	/*	First extract the value of what will become the low-
	 *	order byte of the SDNV text; its high-order bit is 0.	*/

	result[0] = remnant & (uvast) 0x7f;
	remnant = (remnant >> 7) & sdnvMask;

	/*	Now extract the values of all remaining bytes, in
	 *	increasing order, setting high-order bit to 1 for
	 *	each one.  The results array will contain the values
	 *	of the bytes of the SDNV text in reverse order.		*/

	while (remnant)
	{
		result[length] = (remnant & (uvast) 0x7f) | 0x80;
		remnant = (remnant >> 7) & sdnvMask;
		length++;
	}

	/*	Now copy the extracted values into the text of the
	 *	SDNV, starting with the highest-order value.		*/

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

	if(!val)
	{
		return 0;
	}
	if(!sdnvTxt)
	{
		return 0;
	}
	
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

static int	_running(int *newState)
{
	static int	state = 0;

	if (newState)
	{
		state = *newState;
	}

	return state;
}

/* * * zco field * * */

static int	copyFromSource(Sdr sdr, char *buffer, SourceExtent *extent,
			vast bytesToSkip, vast bytesAvbl, ZcoReader *reader)
{
	
}

static void	destroyZco(Sdr sdr, Object zcoObj)
{

}
/* * * sdr field * * */