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

int	bulk_read(unsigned long item, char *buffer, vast offset, vast length)
{
	char	fileName[MAXPATHLEN];
	int	fd;
	int	result;

	getFileName(item, fileName);/* here is an undeal func: getFileName */
	fd = open(fileName, O_RDONLY, 0);
	if (fd < 0)
	{
		printf("bulk_read failed on open.\n");
		return -1;
	}

	if (offset > 0)
	{
		if (lseek(fd, offset, SEEK_SET) == (off_t) -1)
		{
			printf("bulk_read failed on lseek.\n");
			close(fd);
			return -1;
		}
	}

	result = read(fd, buffer, length);
	close(fd);
	if (result < length)
	{
		printf("bulk_read failed on read.\n");
		return -1;
	}

	return result;
}

static int	copyFromSource(Sdr sdr, char *buffer, SourceExtent *extent,
			vast bytesToSkip, vast bytesAvbl, ZcoReader *reader)
{
	ZcoObjLien	objLien;
	ObjRef		objRef;
	ZcoBulkLien	bulkLien;
	BulkRef		bulkRef;
	ZcoFileLien	fileLien;
	FileRef		fileRef;
	int		fd;
	int		bytesRead;
	struct stat	statbuf;
	unsigned long	xmitProgress = 0;

	switch (extent->sourceMedium)
	{
	case ZcoObjSource:
		sdr_read(sdr, (char *) &objLien, extent->location,
				sizeof(ZcoObjLien));
		sdr_read(sdr, (char *) &objRef, objLien.location,
				sizeof(ObjRef));
		sdr_read(sdr, buffer, objRef.object
				+ extent->offset + bytesToSkip, bytesAvbl);
		return bytesAvbl;

	case ZcoBulkSource:
		sdr_read(sdr, (char *) &bulkLien, extent->location,
				sizeof(ZcoBulkLien));
		sdr_read(sdr, (char *) &bulkRef, bulkLien.location,
				sizeof(BulkRef));
		return bulk_read(bulkRef.item, buffer,
				extent->offset + bytesToSkip, bytesAvbl);

	default:	/*	Source text of extent is a file.	*/
		if (reader->trackFileOffset)
		{
			xmitProgress = extent->offset + bytesToSkip + bytesAvbl;
		}

		sdr_read(sdr, (char *) &fileLien, extent->location,
				sizeof(ZcoFileLien));
		sdr_stage(sdr, (char *) &fileRef, fileLien.location, /* here is an undeal func: sdr_stage */
				sizeof(FileRef));
		fd = open(fileRef.pathName, O_RDONLY, 0);
		if (fd >= 0)
		{
			if (fstat(fd, &statbuf) < 0)
			{
				close(fd);	/*	Can't check.	*/
			}
			else if (statbuf.st_ino != fileRef.inode)
			{
				close(fd);	/*	File changed.	*/
			}
			else if (lseek(fd, extent->offset + bytesToSkip,
					SEEK_SET) < 0)
			{
				close(fd);	/*	Can't position.	*/
			}
			else
			{
				bytesRead = read(fd, buffer, bytesAvbl);
				close(fd);
				if (bytesRead == bytesAvbl)
				{
					/*	Update xmit progress.	*/

					if (xmitProgress > fileRef.xmitProgress)
					{
						fileRef.xmitProgress
							= xmitProgress;
						sdr_write(sdr, /* here is an undeal func: sdr_write */
							fileLien.location,
							(char *) &fileRef,
							sizeof(FileRef));
					}

					return bytesAvbl;
				}
			}
		}

		/*	On any problem reading from file, write fill
		 *	and return read length zero.			*/

		memset(buffer, ZCO_FILE_FILL_CHAR, bytesAvbl);
		return 0;
	}
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
		destroyFirstExtent(sdr, zcoObj, &zco); /* here is an undeal func: destroyFirstExtent */
	}

	/*	Destroy all headers.					*/

	for (obj = zco.firstHeader; obj; obj = capsule.nextCapsule)
	{
		sdr_read(sdr, (char *) &capsule, obj, sizeof(Capsule)); /* here is an undeal func: sdr_free */
		sdr_free(sdr, capsule.text);
		occupancy = capsule.length;
		sdr_free(sdr, obj);
		occupancy += sizeof(Capsule);
		zco_reduce_heap_occupancy(sdr, occupancy, zco.acct); /* here is an undeal func: zco_reduce_heap_occupancy */
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
}
/* * * sdr field * * */

static int	lockSdr(SdrState *sdr)
{
	if (sdr->sdrSemaphore == -1)
	{
		return -1;
	}

	sdr->sdrOwnerThread = pthread_self();
	sdr->sdrOwnerTask = getpid();
	sdr->xnDepth = 1;
	return 0;
}

int	takeSdr(SdrState *sdr)
{
	if(!sdr)
	{
		return ERROR;
	}
	if (sdr->sdrSemaphore == -1)
	{
		return -1;		/*	Can't be taken.		*/
	}

	if (sdr->sdrOwnerTask == getpid()
	&& pthread_equal(sdr->sdrOwnerThread, pthread_self()))
	{
		sdr->xnDepth++;
		return 0;		/*	Already taken.		*/
	}

	return lockSdr(sdr);
}