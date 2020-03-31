#include "zco.h"

/* main */

void	zco_start_receiving(Object zco, ZcoReader *reader)
{
	if(!zco)
	{
		return;
	}
	
	if(!reader)
	{
		return;
	}
	
	memset((char *) reader, 0, sizeof(ZcoReader));
	reader->zco = zco;
}

vast	zco_receive_source(Sdr sdr, ZcoReader *reader, vast length,
		char *buffer)
{
	vast		bytesToSkip;
	vast		bytesToReceive;
	vast		bytesReceived;
	vast		bytesAvbl;/*  the lenth each time copied  */
	
	Zco		zco;
	Object		obj;
	SourceExtent	extent;
	
	int		failed = 0;

	if(!sdr)
	{
		return ERROR;
	}
	
		if(!reader)
	{
		return ERROR;
	}
	
		if(lenth < 0)
	{
		return ERROR;
	}
	
	if (length == 0)
	{
		return 0;
	}

	sdr_read(sdr, (char *) &zco, reader->zco, sizeof(Zco));
	bytesToSkip = zco.headersLength + reader->sourceLengthCopied;
	bytesToReceive = length;
	bytesReceived = 0;
	for (obj = zco.firstExtent; obj; obj = extent.nextExtent)
	{
		sdr_read(sdr, (char *) &extent, obj, sizeof(SourceExtent));
		bytesAvbl = extent.length;
		if (bytesToSkip >= bytesAvbl)
		{
			bytesToSkip -= bytesAvbl;
			continue;	/*	not the target	*/
		}

		bytesAvbl -= bytesToSkip;
		if (bytesToReceive < bytesAvbl)
		{
			bytesAvbl = bytesToReceive;
		}

		if (buffer)
		{
			if (copyFromSource(sdr, buffer, &extent, bytesToSkip,
					bytesAvbl, reader) < bytesAvbl)
			{
				failed = 1;	/*	Source problem.	*/
			}

			buffer += bytesAvbl;
		}

		bytesToSkip = 0; /* skip just once */
		bytesToReceive -= bytesAvbl;
		bytesReceived += bytesAvbl;
		reader->sourceLengthCopied += bytesAvbl;

		if (bytesToReceive == 0)
		{
			break;
		}
	}

	if (failed)
	{
		return 0;
	}

	return bytesReceived;
}

void	zco_destroy(Sdr sdr, Object zco)
{
	if(!sdr)
	{
		return;
	}
	if(!zco)
	{
		return;
	}
	destroyZco(sdr, zco);
}

/* for other use */

Object	zco_clone(Sdr sdr, Object fromZcoObj, vast offset, vast length)
{
}

vast	zco_source_data_length(Sdr sdr, Object zcoObj)
{
}

int	zco_prepend_header(Sdr sdr, Object zco, char *text, vast length)
{
}

int	zco_bond(Sdr sdr, Object zco)
{
}
