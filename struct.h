/*  struct.h  */

/* * *  public field * * */

typedef long long		vast;
typedef unsigned long long	uvast;
typedef unsigned long		uaddr;

typedef uaddr		SdrObject;
#define	Object		SdrObject

#define CHKERR(e)    		if (!(e)) return ERROR
#define CHKZERO(e)    		if (!(e)) return 0
#define CHKNULL(e)    		if (!(e)) return NULL
#define CHKVOID(e)    		if (!(e)) return

typedef struct
{
	int		length;
	unsigned char	text[10];
} Sdnv;

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

/* * *  sdr field * * */

typedef uaddr		PsmAddress;

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