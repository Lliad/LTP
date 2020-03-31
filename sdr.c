#include "sdr.h"
#include "pthread.h"

/*  main  */

int	sdr_begin_xn(Sdr sdrv)
{
	if(!sdr)
	{
		return 0;
	}
	
	if (takeSdr(sdrv->sdr) < 0)
	{
		return 0;	/*	Failed to begin transaction.	*/
	}

	sdrv->modified = 0;
	return 1;
}

int	sdr_end_xn(Sdr sdrv)
{
	SdrState	*sdr;

	if(!sdrv)
	{
		return ERROR;
	}
	sdr = sdrv->sdr;
	if (sdr_in_xn(sdrv))
	{
		sdr->xnDepth--;
		if (sdr->xnDepth == 0)
		{
			terminateXn(sdrv);/* here is an undeal func: terminateXn */
		}

		return 0;
	}

	return -1;
}

Object	Sdr_insert(const char *file, int line, Sdr sdrv, char *from,
		size_t size)
{
	Object	obj;

	if (!(sdr_in_xn(sdrv)))
	{
		return 0;
	}

	joinTrace(sdrv, file, line); /* here is an undeal func: joinTrace */
	obj = _sdrmalloc(sdrv, size); /* here is an undeal func: _sdrmalloc */
	if (obj)
	{
		_sdrput(file, line, sdrv, (Address) obj, from, size, SystemPut); /* here is an undeal func: _sdrput */
	}

	return obj;
}

/* for other use */

void	sdr_read(Sdr sdrv, char *into, Address from, size_t length)
{
	SdrState	*sdr;
	Address		to;
	
	/* chk */
	if (lenth <0)
	{
		return;
	}
	
	if (!sdrv)
	{
		return;
	}
	
	if (!into)
	{
		return;
	}

	if (length == 0)
	{
		return;
	}
	
	memset(into, 0, length);		/*	Default value.	*/
	sdr = sdrv->sdr;
	to = from + length;
	
	if((sdr)&&(to < sdr->dsSize))
	{
		memcpy(into, sdrv->dssm + from, length);
	}
}

int	sdr_in_xn(Sdr sdrv)
{
	if(!sdrv)
	{
		return 0;
	}
	
	return (sdrv->sdr != NULL
		&& sdrv->sdr->sdrOwnerTask == getpid()
		&& pthread_equal(sdrv->sdr->sdrOwnerThread, pthread_self()));
}