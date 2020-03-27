/* zco.h */

#include "platform.h"
#include "sdrxn.h"

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
	ZcoMedium	sourceMedium;
	Object		location;	/*	of applicable lien	*/
	vast		offset;		/*	within file/item/object	*/
	vast		length;
	Object		nextExtent;
} SourceExtent;

extern void	zco_start_receiving(Object zco, ZcoReader *reader);
				
vast	zco_receive_source(Sdr sdr, ZcoReader *reader, vast length, char *buffer)
				
extern void	zco_destroy(Sdr sdr,Object zco);

/* for sda use */
Object	zco_clone(Sdr sdr, Object fromZcoObj, vast offset, vast length);

vast	zco_source_data_length(Sdr sdr, Object zcoObj);
