/* zco.c */

#include "zco.h"
#include "platform.h"
#include "sdrxn.h"
#include "struct.h"

void	zco_start_receiving(Object zco, ZcoReader *reader)
{
	CHKVOID(zco);
	CHKVOID(reader);
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

	CHKERR(sdr);
	CHKERR(reader);
	CHKERR(length >= 0);
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
			continue;	/*	Take none of this one.	*/
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

		bytesToSkip = 0;

		/*	Note bytes copied.				*/

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

static void	destroyZco(Sdr sdr, Object zcoObj)
{
	Zco	zco;
	Object	obj;
	Capsule	capsule;
	vast	occupancy;

	sdr_read(sdr, (char *) &zco, zcoObj, sizeof(Zco));

	/*	Destroy all source data extents.			*/

	while (zco.firstExtent)
	{
		destroyFirstExtent(sdr, zcoObj, &zco);
	}

	/*	Destroy all headers.					*/

	for (obj = zco.firstHeader; obj; obj = capsule.nextCapsule)
	{
		sdr_read(sdr, (char *) &capsule, obj, sizeof(Capsule));
		sdr_free(sdr, capsule.text);
		occupancy = capsule.length;
		sdr_free(sdr, obj);
		occupancy += sizeof(Capsule);
		zco_reduce_heap_occupancy(sdr, occupancy, zco.acct);
	}

	/*	Destroy all trailers.					*/

	for (obj = zco.firstTrailer; obj; obj = capsule.nextCapsule)
	{
		sdr_read(sdr, (char *) &capsule, obj, sizeof(Capsule));
		sdr_free(sdr, capsule.text);
		occupancy = capsule.length;
		sdr_free(sdr, obj);
		occupancy += sizeof(Capsule);
		zco_reduce_heap_occupancy(sdr, occupancy, zco.acct);
	}

	/*	Finally destroy the ZCO object.				*/

	sdr_free(sdr, zcoObj);
	zco_reduce_heap_occupancy(sdr, sizeof(Zco), zco.acct);
	_zcoCallback(NULL, zco.acct);
}

void	zco_destroy(Sdr sdr, Object zco)
{
	CHKVOID(sdr);
	CHKVOID(zco);
	destroyZco(sdr, zco);
}

vast	zco_source_data_length(Sdr sdr, Object zcoObj)
{
	OBJ_POINTER(Zco, zco);
	int	headersLength;
	int	trailersLength;

	CHKZERO(sdr);
	CHKZERO(zcoObj);
	GET_OBJ_POINTER(sdr, Zco, zco, zcoObj);
	headersLength = zco->headersLength;
	trailersLength = zco->trailersLength;

	/*	Check for truncation.					*/

	CHKZERO(headersLength == zco->headersLength);
	CHKZERO(trailersLength == zco->trailersLength);
	return zco->sourceLength + headersLength + trailersLength;
}

Object	zco_clone(Sdr sdr, Object fromZcoObj, vast offset, vast length)
{
	Zco	fromZco;
	Object	toZcoObj;		/*	Cloned ZCO object.	*/
	Zco	toZco;
	vast	lengthCloned;

	CHKZERO(sdr);
	CHKZERO(fromZcoObj);
	CHKZERO(offset >= 0);
	CHKZERO(length > 0);
	sdr_read(sdr, (char *) &fromZco, fromZcoObj, sizeof(Zco));
	toZcoObj = zco_create(sdr, 0, 0, 0, 0, fromZco.acct);
	if (toZcoObj == (Object) ERROR)
	{
		putErrmsg("Can't create clone ZCO.", NULL);
		return (Object) ERROR;
	}

	sdr_stage(sdr, (char *) &toZco, toZcoObj, sizeof(Zco));
	lengthCloned = appendExtentOfExistingZco(sdr, toZcoObj, &toZco,
			&fromZco, offset, length);
	if (lengthCloned < 0)
	{
		putErrmsg("Can't create clone ZCO.", NULL);
		return (Object) ERROR;
	}

	return toZcoObj;
}