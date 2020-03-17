/* 

	ltp.h:根据734x1b1.pdf和RFC5326的描述，定义一些基本的函数、数据结构

*/

#ifndef _LTP_H_
#define _LTP_H_

#ifndef _LTP_P_H_
#define _LTP_P_H_

typedef unsigned long		uaddr;
typedef uaddr		SdrObject;
#define	Object		SdrObject

/* LTP segment structure definitions. */

/*
 *
 *	A value of 0 in the CTRL (Control) flag identifies 
 *	the segment as a data segment, while a value of 1 
 *	identifies it as a control segment. A data segment 
 *	with the EXC (Exception) flag set to 0 is a red-part
 *	segment; a data segment with EXC set to 1 is a 
 *	green-part segment. For a control segment, having 
 *	the EXC flag set to 1 indicates that the segment 
 *	pertains to session cancellation activity. 
 *	Any data segment (whether red-part or green-part) 
 *	with both Flag 1 and Flag 0 set to 1 indicates EOB. 
 *	Any data segment (whether red-part or green-part) 
 *	with both Flag 1 and Flag 0 set to 0 indicates data
 *	without any additional protocol significance. Any 
 *	red-part data segment with either flag bit non-zero 
 *	is a checkpoint. Any red-part data segment with 
 *	Flag 1 set to 1 indicates the end of red-part.	*/

#define	LTP_CTRL_FLAG		0x08
#define	LTP_EXC_FLAG		0x04
#define LTP_FLAG_1		0x02
#define	LTP_FLAG_0		0x01


typedef unsigned long long  engineId;

typedef struct{
	engineId	sourceEngineId;
	unsigned int	sessionNbr;
}LtpSessionId;

typedef enum
{
	LtpDsRed = 0,
	LtpDsRedCheckpoint,
	LtpDsRedEORP,
	LtpDsRedEOB,
	LtpDsGreen,
	LtpDsGreenEOB = 7,
	LtpRS,		/*	Report.					*/
	LtpRAS,		/*	Report acknowledgment.			*/
	LtpCS = 12,	/*	Cancel by source of block.		*/
	LtpCAS,		/*	CS acknowledgment.			*/
	LtpCR,		/*	Cancel by block receiver (destination).	*/
	LtpCAR		/*	CR acknowledgment.			*/
} LtpSegmentTypeCode;

typedef enum
{
	LtpCancelByUser = 0,
	LtpClientSvcUnreachable,
	LtpRetransmitLimitExceeded,
	LtpMiscoloredSegment,
	LtpCancelByEngine
} LtpCancelReasonCode;

typedef struct
{
	LtpSegmentTypeCode	segTypeCode;
	unsigned int		headerLength;
	unsigned int		contentLength;
	unsigned int		trailerLength;
	unsigned char		headerExtensionsCount;
	unsigned char		trailerExtensionsCount;
	
	Object			headerExtensions;
	Object			trailerExtensions;

	/*	Fields for data segments.				*/

	unsigned int		ohdLength;	/*	Data seg ohd.	*/
	unsigned int		clientSvcId;	/*	Destination.	*/
	unsigned int		offset;		/*	Within block.	*/
	unsigned int		length;		/*	Of block data.	*/
	Object			block;	/*	Session svcDataObjects.	*/

	/*	Fields for report segments.				*/

	unsigned int		upperBound;
	unsigned int		lowerBound;
	Object			receptionClaims;/*	SDR list.	*/

	/*	Fields for management segments.				*/

	LtpCancelReasonCode	reasonCode;
} LtpSeg;

typedef enum
{
	LtpDataSeg,
	LtpReportSeg,
	LtpRptAckSeg,
	LtpMgtSeg
} LtpSegmentClass;

/*	An LtpRecvSeg encapsulates a segment that has been acquired
 *	by the local LTP engine, for handling.
 *
 *	The session referenced by an LtpRecvSeg depends on seg.segTypeCode:
 *
 *	If the code is 0, 1, 2, 3, 4, 7, 9, 12, or 15, then the
 *	remote engine is the source of the block, the local engine is
 *	the destination, and the session is therefore a ImportSession.
 *
 *	If the code is 8, 13, or 14, then the remote engine is the
 *	destination of the block, the local engine is the source,
 *	and the session is therefore an ExportSession.			*/

typedef struct
{
	unsigned int	acqOffset;	/*	Within acquisition ZCO.	*/
	Object		sessionObj;
	Object		sessionListElt;
	LtpSegmentClass	segmentClass;
	LtpSeg		seg;
} LtpRecvSeg;

/*	An LtpXmitSeg encapsulates a segment that has been produced
 *	by the local LTP engine, for transmission.
 *
 *	The session referenced by an LtpXmitSeg depends on seg.segTypeCode:
 *
 *	If the code is 0, 1, 2, 3, 4, 7, 9, 12, or 15, then the
 *	remote engine is the destination of the block, the local engine
 *	is the source, and the session number therefore identifies
 *	an ExportSession.
 *
 *	If the code is 8, 13, or 14, then the remote engine is the
 *	source of the block, the local engine is the destination,
 *	and the session number therefore identifies a ImportSession.	*/

typedef struct
{
	unsigned int	sessionNbr;
	uvast		remoteEngineId;
	Object		queueListElt;
	Object		ckptListElt;	/*	For checkpoints only.	*/
	Object		sessionObj;	/*	For codes 1-3, 14 only.	*/
	Object		sessionListElt;	/*	For data segments only.	*/
	LtpSegmentClass	segmentClass;
	LtpSeg		seg;
} LtpXmitSeg;

extern int ltp_trans(unsigned int clientId,
					engineId destinationEngineId,
					Object clientServiceData,
					unsigned int redLength);
					
extern int ltp_cancel_trans(LtpSessionId *sessionId);

extern int ltp_cancel_reception(LtpSessionId *sessionId);

extern int transSessionStart_indication(LtpSessionId *sessionId);

extern int receptionSessionStart_indication(LtpSessionId *sessionId);

extern int RedpartReception_indication(LtpSessionId *sessionId,
										unsigned int red_lenth,
										engineId sourceEngineID);

extern int InitTransCompletion_indication(LtpSessionId *sessionId);

extern int TransSessionCompletion_indication(LtpSessionId *sessionId);

extern int TransSessionCancel_indication(LtpSessionId *sessionId,
										int reason_code);

extern int ReceptionSessionCancel_indication(LtpSessionId *sessionId,
										int reason_code);