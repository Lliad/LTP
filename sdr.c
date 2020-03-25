/* sdr.c */

#include "sdr.h"
#include "platform.h"
#include "sdrxn.h"
#include "sdrP.h"
#include "struct.h"

int	sdr_begin_xn(Sdr sdrv)
{
	CHKZERO(sdrv);
	if (takeSdr(sdrv->sdr) < 0)
	{
		return 0;	/*	Failed to begin transaction.	*/
	}

	sdrv->modified = 0;
	return 1;		/*	Began transaction.		*/
}

int	sdr_end_xn(Sdr sdrv)
{
	SdrState	*sdr;/* def in sdrP.h */

	CHKERR(sdrv);
	sdr = sdrv->sdr;
	if (sdr_in_xn(sdrv))
	{
		sdr->xnDepth--;
		if (sdr->xnDepth == 0)
		{
			terminateXn(sdrv);
		}

		return 0;
	}

	return -1;
}

Object	sdr_insert(Sdr sdrv, char *from,
		size_t size)
{
	Object	obj;

	if (!(sdr_in_xn(sdrv)))
	{
		oK(_iEnd(file, line, _notInXnMsg()));
		return 0;
	}

	joinTrace(sdrv, file, line);
	obj = _sdrmalloc(sdrv, size);
	if (obj)
	{
		_sdrput(file, line, sdrv, (Address) obj, from, size, SystemPut);
	}

	return obj;
}