/*  struct.h  */

/* * *  public field * * */

typedef long long		vast;
typedef unsigned long long	uvast;
typedef unsigned long		uaddr;

typedef uaddr		SdrObject;
#define	Object		SdrObject

typedef struct
{
	int		length;
	unsigned char	text[10];
} Sdnv;

typedef struct sdrv_str
{
	SdrState	*sdr;

	int		dsfile;
	char		*dssm;
	uaddr		dssmId;

	int		logfile;
	char		*logsm;
	uaddr		logsmId;

	Lyst		knownObjects;
	int		modified;

	PsmView		traceArea;
	PsmView		*trace;
	const char	*currentSourceFileName;
	int		currentSourceFileLine;
} SdrView;

typedef struct sdrv_str	*Sdr;

/* * *  sdr field * * */

typedef uaddr		PsmAddress;

typedef struct sdr_str
{
	char		name[32];
	PsmAddress	sdrsElt;
	int		configFlags;
	size_t		initHeapWords;
	size_t		heapSize;
	size_t		dsSize;
	int		dsKey;
	size_t		logSize;
	int		logKey;

	sm_SemId	sdrSemaphore;
	int		sdrOwnerTask;		/*	Task ID.	*/
	pthread_t	sdrOwnerThread;		/*	Thread ID.	*/
	int		xnDepth;
	int		xnCanceled;		/*	Boolean.	*/
	int		logLength;		/*	All entries.	*/
	int		maxLogLength;		/*	Max Log Length  */
	PsmAddress	logEntries;		/*	Offsets in log.	*/


	int		traceKey;
	size_t		traceSize;

	char		pathName[MAXPATHLEN];

	int		halted;
	char		restartCmd[32];
	time_t		restartTime;
} SdrState;

/* * * zco field * * */

typedef struct
{
	Object	zco;
	int	trackFileOffset;		/*	Boolean control	*/
	vast	headersLengthCopied;		/*	within extents	*/
	vast	sourceLengthCopied;		/*	within extents	*/
	vast	trailersLengthCopied;		/*	within extents	*/
	vast	lengthCopied;			/*	incl. capsules	*/
} ZcoReader;

typedef enum
{
	ZcoInbound = 0,
	ZcoOutbound = 1,
	ZcoUnknown = 2
} ZcoAcct;

typedef struct
{
	Object		firstHeader;		/*	Capsule		*/
	Object		lastHeader;		/*	Capsule		*/

	/*	Note that prepending headers and appending trailers
	 *	increases the lengths of the linked list for headers
	 *	and trailers but DOES NOT affect the headersLength
	 *	and trailersLength fields.  These fields indicate only
	 *	how much of the concatenated content of all extents
	 *	in the linked list of extents is currently believed
	 *	to constitute ADDITIONAL opaque header and trailer
	 *	information, just as sourceLength indicates how much
	 *	of the concatenated content of all extents is believed
	 *	to constitute source data.  The total length of the
	 *	ZCO is the sum of the lengths of the extents (some of
	 *	which sum is source data and some of which may be
	 *	opaque header and trailer information) plus the sum
	 *	of the lengths of all explicitly attached headers and
	 *	trailers.						*/

	Object		firstExtent;		/*	SourceExtent	*/
	Object		lastExtent;		/*	SourceExtent	*/
	vast		headersLength;		/*	within extents	*/
	vast		sourceLength;		/*	within extents	*/
	vast		trailersLength;		/*	within extents	*/

	Object		firstTrailer;		/*	Capsule		*/
	Object		lastTrailer;		/*	Capsule		*/

	vast		aggregateCapsuleLength;
	vast		totalLength;		/*	incl. capsules	*/

	ZcoAcct		acct;
} Zco;

typedef struct
{
	double		fileOccupancy;
	double		maxFileOccupancy;
	double		bulkOccupancy;
	double		maxBulkOccupancy;
	double		heapOccupancy;
	double		maxHeapOccupancy;
} ZcoBook;

typedef struct
{
	ZcoBook		books[2];
} ZcoDB;

typedef struct
{
	Object		text;		/*	header or trailer	*/
	vast		length;
	Object		prevCapsule;
	Object		nextCapsule;
} Capsule;

typedef struct
{
	int		refCount[2];	/*	ZcoInbound, ZcoOutbound	*/
	unsigned long	inode;		/*	For detecting change.	*/
	unsigned long	fileLength;	/*	For detecting EOF.	*/
	unsigned long	xmitProgress;	/*	For detecting EOF.	*/
	char		pathName[256];
	char		cleanupScript[256];
	char		okayToDestroy;	/*	Boolean.		*/
	char		unlinkOnDestroy;/*	Boolean.		*/
} FileRef;

typedef struct
{
	int		refCount[2];	/*	ZcoInbound, ZcoOutbound	*/
	unsigned long	item;		/*	Bulk item location.	*/
	vast		length;		/*	Length of object.	*/
	char		okayToDestroy;	/*	Boolean.		*/
} BulkRef;

typedef struct
{
	int		refCount[2];	/*	ZcoInbound, ZcoOutbound	*/
	Object		object;		/*	Heap address of object.	*/
	vast		length;		/*	Length of object.	*/
	char		okayToDestroy;	/*	Boolean.		*/
} ObjRef;

typedef struct
{
	int		refCount[2];	/*	ZcoInbound, ZcoOutbound	*/
	Object		location;	/*	Heap address of FileRef.*/
	vast		length;		/*	Length of lien on file.	*/
} ZcoFileLien;

typedef struct
{
	int		refCount[2];	/*	ZcoInbound, ZcoOutbound	*/
	Object		location;	/*	Heap address of BulkRef.*/
	vast		length;		/*	Length of lien on item.	*/
} ZcoBulkLien;

typedef struct
{
	int		refCount[2];	/*	ZcoInbound, ZcoOutbound	*/
	Object		location;	/*	Heap address of ObjRef.	*/
	vast		length;		/*	Length of lien on obj.	*/
} ZcoObjLien;