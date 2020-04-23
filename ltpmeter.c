#include <stdlib.h>
#include <unistd.h>

#include "ltpP.h"

int	main(int argc, char *argv[])
{
	Sdr		sdr;
	LtpVdb		*vdb;
	LtpVspan	*vspan;
	uvast	remoteEngineId;
	PsmAddress	vspanElt;
	Object		spanObj;
	LtpSpan		span;
	ExportSession	session;
	Lyst		extents;
	ExportExtent	*extent;
	unsigned int	ckptSerialNbr;
	int		returnCode = 0;
	int		result;
	
	/* check command-line variable vailate or not */
	remoteEngineId = strtoull(argv[1], NULL, 10);
	if (remoteEngineId == 0)
	{
		printf("need remoteEngineId.\n");
		return 0;
	}

	if (ltpInit(0) < 0)
	{
		printf("init ltp failed\n.");
		return 1;
	}

	vdb = getLtpVdb();
	sdr = getIonsdr();
	sdr_begin_xn(sdr);
	
	findSpan(remoteEngineId, &vspan, &vspanElt);
	if (vspanElt == 0)
	{
		printf("can not find engineId: %lu.\n", remoteEngineId);
		sdr_exit_xn(sdr);
		return 1;
	}

	if (vspan->meterPid != getpid())
	{
		printf("ltpmeter task is already started for this engine.\n");
		sdr_exit_xn(sdr);
		return 1;
	}

	spanObj = sdr_list_data(sdr, vspan->spanElt);
	sdr_stage(sdr, (char *) &span, spanObj, sizeof(LtpSpan));
	if (span.currentExportSessionObj == 0) /* new span, must init span */
	{
		sdr_exit_xn(sdr);
		
		startExportSession(sdr, spanObj, vspan);
		sdr_begin_xn(sdr);
		sdr_stage(sdr, (char *) &span, spanObj, sizeof(LtpSpan));
	}

	/* main loop */
	printf("ltpmeter is running.\n");
	while (returnCode == 0)
	{
		/*	wait for bufClosedSemaphore  */

		if (sm_SemTake(vspan->bufClosedSemaphore))
		{
			printf("take bufClosedSemaphore failed.\n");
			sdr_end_xn(sdr);
			returnCode = 1;
			break;
		}

		if (sm_SemEnded(vspan->bufClosedSemaphore))
		{
			printf("ltpmeter is stopped on engine: %lu.\n", remoteEngineId);
			returnCode = 1;
			break;
		}

		/*	Now segment the block that is currently
		 *	aggregated in the buffer, giving the span's
		 *	segSemaphore once per segment.			*/

		sdr_stage(sdr, (char *) &session, span.currentExportSessionObj,
				sizeof(ExportSession));
		session.clientSvcId = span.clientSvcIdOfBufferedBlock;
		session.totalLength = span.lengthOfBufferedBlock;
		session.redPartLength = span.redLengthOfBufferedBlock;
		encodeSdnv(&(session.clientSvcIdSdnv), session.clientSvcId);

		/*	We can now compute the upper limit on the number
		 *	of checkpoints we will send in the course of
		 *	this session.  We send one initial checkpoint
		 *	plus one more checkpoint in response to every
		 *	report except the last, which elicits only a
		 *	report acknowledgment.  So the maximum number
		 *	of reports that we expect from the receiver
		 *	determines the maximum number of checkpoints
		 *	we will send.					*/

		extents = lyst_create_using(getIonMemoryMgr());
		extent = (ExportExtent *) MTAKE(sizeof(ExportExtent));
		if (extent && extents)
		{
			lyst_insert_last(extents, extent);
		}
		else
		{
			printf("init extent and extents failed.\n");
			sdr_end_xn(sdr);
			returnCode = 1;
			break;
		}

		extent->offset = 0;
		extent->length = session.totalLength;
		do
		{
			ckptSerialNbr = rand();

			/*	Limit serial number SDNV length.	*/

			ckptSerialNbr %= LTP_SERIAL_NBR_LIMIT;
		} while (ckptSerialNbr == 0);
		result = issueSegments(sdr, &span, vspan, &session,
				span.currentExportSessionObj, extents, 0,
				ckptSerialNbr);
		MRELEASE(extent);
		lyst_destroy(extents);
		if (result)
		{
			printf("segment block failed.\n");
			sdr_end_xn(sdr);
			returnCode = 1;
			break;
		}

		/*	Segment issuance succeeded.			*/

		if (enqueueNotice(vdb->clients + session.clientSvcId,
				vdb->ownEngineId, session.sessionNbr,
				0, 0, LtpExportSessionStart, 0, 0, 0) < 0)
		{
			printf("can not post ExportSessionStart notice.\n");
			sdr_cancel_xn(sdr);
			returnCode = 1;
			break;
		}

		/*	update sdr  */

		sdr_write(sdr, span.currentExportSessionObj, (char *) &session,
				sizeof(ExportSession));

		/*	reinit span	*/

		span.ageOfBufferedBlock = 0;
		span.lengthOfBufferedBlock = 0;
		span.redLengthOfBufferedBlock = 0;
		span.clientSvcIdOfBufferedBlock = 0;
		span.currentExportSessionObj = 0;
		sdr_write(sdr, spanObj, (char *) &span, sizeof(LtpSpan));
		sdr_end_xn(sdr);

		/*	Start new export session for the next block.	*/

		startExportSession(sdr, spanObj, vspan);
		sdr_begin_xn(sdr);
		sdr_stage(sdr, (char *) &span, spanObj, sizeof(LtpSpan));
	}

	printf("ltpmeter has ended.\n");
	ionDetach();
	
	return returnCode;
}
