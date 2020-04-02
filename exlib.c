/* exlib.c : some funcs copy from ION source code */

#include "exlib.h"

/* zco field */

void	zco_reduce_heap_occupancy(Sdr sdr, vast delta, ZcoAcct acct)
{
	Object	obj;
	ZcoDB	db;
	ZcoBook	*book;

	CHKVOID(sdr);
	CHKVOID(delta >= 0);
	obj = getZcoDB(sdr);
	if (obj)
	{
		sdr_stage(sdr, (char *) &db, obj, sizeof(ZcoDB));
		book = &(db.books[((int) acct)]);
		book->heapOccupancy -= delta;
		sdr_write(sdr, obj, (char *) &db, sizeof(ZcoDB));
	}
}

static void	destroyExtentText(Sdr sdr, SourceExtent *extent, ZcoAcct acct)
{
	Object		lienObj;
	ZcoObjLien	objLien;
	Object		objRefObj;
	ObjRef		objRef;
	ZcoBulkLien	bulkLien;
	Object		bulkRefObj;
	BulkRef		bulkRef;
	ZcoFileLien	fileLien;
	Object		fileRefObj;
	FileRef		fileRef;

	if (extent->sourceMedium == ZcoObjSource)
	{
		lienObj = extent->location;
		sdr_stage(sdr, (char *) &objLien, lienObj, sizeof(ZcoObjLien));
		objLien.refCount[acct]--;
		if (objLien.refCount[acct] == 0)
		{
			zco_reduce_heap_occupancy(sdr, objLien.length
					+ sizeof(ZcoFileLien), acct);

			/*	In addition, the object reference count
			 *	for this account is now reduced by 1.
			 *	(There is now 1 less reference to this
			 *	obj reference object in this account.	*/

			objRefObj = objLien.location;
			sdr_stage(sdr, (char *) &objRef, objRefObj,
					sizeof(ObjRef));
			objRef.refCount[acct]--;
			if (objRef.refCount[acct] == 0)
			{
				zco_reduce_heap_occupancy(sdr, sizeof(ObjRef),
						acct);
			}

			/*	So now the object reference object may
			 *	may no longer be needed.		*/

			if (objRef.refCount[0] == 0 && objRef.refCount[1] == 0
			&& objRef.okayToDestroy)
			{
				destroyObjReference(sdr, &objRef, objRefObj);
			}
			else	/*	Just update reference count.	*/
			{
				sdr_write(sdr, objRefObj, (char *) &objRef,
						sizeof(ObjRef));
			}
		}

		if (objLien.refCount[0] == 0 && objLien.refCount[1] == 0)
		{
			/*	Destroy the lien object.		*/

			sdr_free(sdr, lienObj);
		}
		else	/*	Just update the lien reference count.	*/
		{
			sdr_write(sdr, lienObj, (char *) &objLien,
					sizeof(ZcoObjLien));
		}

		return;
	}

	if (extent->sourceMedium == ZcoBulkSource)
	{
		lienObj = extent->location;
		sdr_stage(sdr, (char *) &bulkLien, lienObj,
				sizeof(ZcoBulkLien));
		bulkLien.refCount[acct]--;
		if (bulkLien.refCount[acct] == 0)
		{
			zco_reduce_bulk_occupancy(sdr, bulkLien.length, acct);
			zco_reduce_heap_occupancy(sdr, sizeof(ZcoBulkLien),
					acct);

			/*	In addition, the bulk reference count
			 *	for this account is now reduced by 1.
			 *	(There is now 1 less reference to this
			 *	bulk reference object in this account.	*/

			bulkRefObj = bulkLien.location;
			sdr_stage(sdr, (char *) &bulkRef, bulkRefObj,
					sizeof(BulkRef));
			bulkRef.refCount[acct]--;
			if (bulkRef.refCount[acct] == 0)
			{
				zco_reduce_heap_occupancy(sdr, sizeof(BulkRef),
						acct);
			}

			/*	So now the bulk reference object may
			 *	may no longer be needed.		*/

			if (bulkRef.refCount[0] == 0 && bulkRef.refCount[1] == 0
			&& bulkRef.okayToDestroy)
			{
				destroyBulkReference(sdr, &bulkRef, bulkRefObj);
			}
			else	/*	Just update reference count.	*/
			{
				sdr_write(sdr, bulkRefObj, (char *) &bulkRef,
						sizeof(BulkRef));
			}
		}

		if (bulkLien.refCount[0] == 0 && bulkLien.refCount[1] == 0)
		{
			/*	Destroy the lien object.		*/

			sdr_free(sdr, lienObj);
		}
		else	/*	Just update the lien reference count.	*/
		{
			sdr_write(sdr, lienObj, (char *) &bulkLien,
					sizeof(ZcoBulkLien));
		}

		return;
	}

	if (extent->sourceMedium == ZcoFileSource)
	{
		lienObj = extent->location;
		sdr_stage(sdr, (char *) &fileLien, lienObj,
				sizeof(ZcoFileLien));
		fileLien.refCount[acct]--;
		if (fileLien.refCount[acct] == 0)
		{
			zco_reduce_file_occupancy(sdr, fileLien.length, acct);
			zco_reduce_heap_occupancy(sdr, sizeof(ZcoFileLien),
					acct);

			/*	In addition, the file reference count
			 *	for this account is now reduced by 1.
			 *	(There is now 1 less reference to this
			 *	file reference object in this account.	*/

			fileRefObj = fileLien.location;
			sdr_stage(sdr, (char *) &fileRef, fileRefObj,
					sizeof(FileRef));
			fileRef.refCount[acct]--;
			if (fileRef.refCount[acct] == 0)
			{
				zco_reduce_heap_occupancy(sdr, sizeof(FileRef),
						acct);
			}

			/*	So now the file reference object may
			 *	may no longer be needed.		*/

			if (fileRef.refCount[0] == 0 && fileRef.refCount[1] == 0
			&& fileRef.okayToDestroy)
			{
				destroyFileReference(sdr, &fileRef, fileRefObj);
			}
			else	/*	Just update reference count.	*/
			{
				sdr_write(sdr, fileRefObj, (char *) &fileRef,
						sizeof(FileRef));
			}
		}

		if (fileLien.refCount[0] == 0 && fileLien.refCount[1] == 0)
		{
			/*	Destroy the lien object.		*/

			sdr_free(sdr, lienObj);
		}
		else	/*	Just update the lien reference count.	*/
		{
			sdr_write(sdr, lienObj, (char *) &fileLien,
					sizeof(ZcoFileLien));
		}

		return;
	}

	putErrmsg("Extent source medium invalid", itoa(extent->sourceMedium));
}

static void	destroyFirstExtent(Sdr sdr, Object zcoObj, Zco *zco)
{
	SourceExtent	extent;

	sdr_read(sdr, (char *) &extent, zco->firstExtent, sizeof(SourceExtent));

	/*	Release the extent's content text.			*/

	destroyExtentText(sdr, &extent, zco->acct);

	/*	Destroy the extent itself.				*/

	sdr_free(sdr, zco->firstExtent);
	zco_reduce_heap_occupancy(sdr, sizeof(SourceExtent), zco->acct);

	/*	Erase the extent from the ZCO.				*/

	zco->firstExtent = extent.nextExtent;
	zco->totalLength -= extent.length;
	if (extent.length > zco->headersLength)
	{
		extent.length -= zco->headersLength;
		zco->headersLength = 0;
	}
	else
	{
		zco->headersLength -= extent.length;
		extent.length = 0;
	}

	if (extent.length > zco->sourceLength)
	{
		extent.length -= zco->sourceLength;
		zco->sourceLength = 0;
	}
	else
	{
		zco->sourceLength -= extent.length;
		extent.length = 0;
	}

	if (extent.length > zco->trailersLength)
	{
		extent.length -= zco->trailersLength;
		zco->trailersLength = 0;
	}
	else
	{
		zco->trailersLength -= extent.length;
		extent.length = 0;
	}
}