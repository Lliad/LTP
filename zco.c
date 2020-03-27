/* zco.c */

#include "zco.h"
#include "struct.h"

void	zco_start_receiving(Object zco, ZcoReader *reader)
{
	memset((char *) reader, 0, sizeof(ZcoReader));
	reader->zco = zco;
}

vast	zco_receive_source(Sdr sdr, ZcoReader *reader, vast length,
		char *buffer)
{
	Zco		zco;
	vast		bytesToSkip;
	vast		bytesToReceive;
	vast		bytesReceived;
	vast		bytesAvbl;
	Object		obj;
	SourceExtent	extent;
	int		failed = 0;

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
			continue;
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
				failed = 1;
			}

			buffer += bytesAvbl;
		}

		bytesToSkip = 0;

		reader->sourceLengthCopied += bytesAvbl;
		bytesToReceive -= bytesAvbl;
		bytesReceived += bytesAvbl;
		if (bytesToReceive == 0)	/*	Done.		*/
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
	destroyZco(sdr, zco);
}

/* for sda use */

vast	zco_source_data_length(Sdr sdr, Object zcoObj)
{
	int	headersLength;
	int	trailersLength;
	
	OBJ_POINTER(Zco, zco);
	GET_OBJ_POINTER(sdr, Zco, zco, zcoObj);
	
	headersLength = zco->headersLength;
	trailersLength = zco->trailersLength;

	return zco->sourceLength + headersLength + trailersLength;
}

Object	zco_clone(Sdr sdr, Object fromZcoObj, vast offset, vast length)
{

}
