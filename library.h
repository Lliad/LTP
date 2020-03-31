/* library.h: struct for use */

#include <string.h> /* for memset */
#include <unistd.h> /* for getpid */
#include <sys/stat.h> /* for open fstst */
#include <fcntl.h>
#include <sys/types.h> /* for lseek */

/* * * public field * * */

typedef long long		vast;
typedef unsigned long long	uvast;
typedef unsigned long		uaddr;	/*	Pointer-sized integer.	*/

typedef uaddr		SdrObject;
#define	Object		SdrObject

typedef uaddr		SdrAddress;
#define	Address		SdrAddress

typedef int		sm_SemId;

#define ERROR			(-1)


/* * * sda field * * */

#define	SdaLtpClientId	(2)

typedef struct
{
	uvast		sourceEngineId;
	unsigned int	sessionNbr;	/*	Assigned by source.	*/
} LtpSessionId;

typedef vast	(*SdaDelimiterFn)(unsigned int clientId,
			unsigned char *buffer,
			vast bufferLength);
		/*	An SDA delimiter function inspects the client
		 *	service data bytes in "buffer" - the first
		 *	"bufferLength" bytes of the as-yet unprocessed
		 *	remnant of an LTP service data block - to
		 *	determine the length of the client data unit
		 *	at the start of the buffer; the "clientID" of
		 *	the client data unit is provided to aid in
		 *	this determination.  It returns that length
		 *	if the determination was successful, zero if
		 *	there is no valid client service data unit
		 *	at the start of the buffer, -1 on any other
		 *	failure.					*/

typedef int	(*SdaHandlerFn)(uvast sourceEngineId,
			unsigned int clientId,
			Object clientServiceData);	/*	ZCO	*/
		/*	An SDA handler function applies application
		 *	processing to the client service data unit
		 *	for client "clientID" that is identified by
		 *	clientServiceData.  It returns -1 on any
		 *	system error, otherwise zero.			*/

/* * * zco field * * */

#define	ZCO_FILE_FILL_CHAR	' '

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

typedef struct
{
	Object		text;		/*	header or trailer	*/
	vast		length;
	Object		prevCapsule;
	Object		nextCapsule;
} Capsule;

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

typedef enum
{
	ZcoFileSource = 1,
	ZcoBulkSource = 2,
	ZcoObjSource = 3,
	ZcoSdrSource = 4,
	ZcoZcoSource = 5
} ZcoMedium;

typedef struct
{
	ZcoMedium	sourceMedium;
	Object		location;	/*	of applicable lien	*/
	vast		offset;		/*	within file/item/object	*/
	vast		length;
	Object		nextExtent;
} SourceExtent;

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



/* * * sdr field * * */

/* some def in psm.h lyst.h*/

typedef struct sdr_str
{
		/*	General SDR operational parameters.	*/

	char		name[32];
	PsmAddress	sdrsElt;		/*	In sch->sdrs.	*/
	int		configFlags;
	size_t		initHeapWords;		/*	In FULL WORDS.	*/
	size_t		heapSize;		/*	dsSize - map	*/
	size_t		dsSize;			/*	heap + map	*/
	int		dsKey;			/*	RAM DS shmKey	*/
	size_t		logSize;		/*	(if in memory)	*/
	int		logKey;			/*	RAM log shmKey	*/

		/*	Parameters of current transaction.	*/

	sm_SemId	sdrSemaphore;
	int		sdrOwnerTask;		/*	Task ID.	*/
	pthread_t	sdrOwnerThread;		/*	Thread ID.	*/
	int		xnDepth;
	int		xnCanceled;		/*	Boolean.	*/
	int		logLength;		/*	All entries.	*/
	int		maxLogLength;		/*	Max Log Length  */
	PsmAddress	logEntries;		/*	Offsets in log.	*/

		/*	SDR trace data access.			*/

	int		traceKey;		/*	trace shmKey	*/
	size_t		traceSize;		/*	0 = disabled	*/

		/*	Path to directory for files (log, ds).	*/

	char		pathName[MAXPATHLEN];

		/*	Parameters for restart.				*/

	int		halted;			/*	boolean		*/
	char		restartCmd[32];
	time_t		restartTime;
} SdrState;

typedef struct sdrv_str
{
	SdrState	*sdr;		/*	Local SDR state access.	*/

	int		dsfile;		/*	DS in file (fd).	*/
	char		*dssm;		/*	DS in shared memory.	*/
	uaddr		dssmId;		/*	DS shmId if applicable.	*/

	int		logfile;	/*	Xn log file (fd).	*/
	char		*logsm;		/*	Log in shared memory.	*/
	uaddr		logsmId;	/*	Log shmId if applicable.*/

	Lyst		knownObjects;	/*	ObjectExtents.		*/
	int		modified;	/*	Boolean.		*/

	PsmView		traceArea;	/*	local access to trace	*/
	PsmView		*trace;		/*	local access to trace	*/
	const char	*currentSourceFileName;	/*	for tracing	*/
	int		currentSourceFileLine;	/*	for tracing	*/
} SdrView;
typedef struct sdrv_str	*Sdr;

/* func field */

static int	copyFromSource(Sdr sdr, char *buffer, SourceExtent *extent,
			vast bytesToSkip, vast bytesAvbl, ZcoReader *reader);

void	encodeSdnv(Sdnv *sdnv, uvast val)
			
int	decodeSdnv(uvast *val, unsigned char *sdnvTxt);

static int	_running(int *newState);

static void	destroyZco(Sdr sdr, Object zcoObj);