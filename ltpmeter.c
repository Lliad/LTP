#include "ltpP.h"

int	main(int argc, char *argv[])
{
	uvast	remoteEngineId;
	Sdr		sdr;
	LtpVdb		*vdb;
	LtpVspan	*vspan;
	PsmAddress	vspanElt;
	Object		spanObj;
	LtpSpan		span;
	int		returnCode = 0;
	ExportSession	session;
	Lyst		extents;
	ExportExtent	*extent;
	unsigned int	ckptSerialNbr = 0;
	int		result;

	if(argc > 1)
	{
		remoteEngineId = strtoull(argv[1], NULL, 10);
		if (remoteEngineId == 0)
		{
			printf("need remote engine id.\n");
			return 0;
		}
	}

	if (ltpInit(0) < 0)
	{
		printf("init ltp failed.\n");
		return 1;
	}

	sdr = getIonsdr();
	vdb = getLtpVdb();
	
	sdr_begin_xn(sdr);
	findSpan(remoteEngineId, &vspan, &vspanElt);
	if (vspanElt == 0)
	{
		sdr_exit_xn(sdr);
		printf("can not find engine: %lu.", remoteEngineId);
		return 1;
	}

	spanObj = sdr_list_data(sdr, vspan->spanElt);
	sdr_stage(sdr, (char *) &span, spanObj, sizeof(LtpSpan));
	if (span.currentExportSessionObj == 0) /* new span. must init first	*/
	{
		sdr_exit_xn(sdr);

		startExportSession(sdr, spanObj, vspan);
		sdr_begin_xn(sdr);
		sdr_stage(sdr, (char *) &span, spanObj, sizeof(LtpSpan));
	}

	printf("ltpmeter is running.");
	while (returnCode == 0)
	{
		/* wait for semaphore */
		if (span.lengthOfBufferedBlock < span.aggrSizeLimit)
		{
			sdr_exit_xn(sdr);
			if (sm_SemTake(vspan->bufClosedSemaphore) < 0)
			{
				printf("take bufClosedSemaphore failed.\n");
				returnCode = 1;
				break;	/*	Failure.	*/
			}

			if (sm_SemEnded(vspan->bufClosedSemaphore))
			{
				printf("ltp meter to engine %lu is stopped.", remoteEngineId);
				returnCode = 1;
				break;
			}

			sdr_begin_xn(sdr);
			sdr_stage(sdr, (char *) &span, spanObj, sizeof(LtpSpan));
		}

		if (span.lengthOfBufferedBlock == 0)
		{
			continue;	/*	nothing to do  */
		}

		sdr_stage(sdr, (char *) &session, span.currentExportSessionObj,
				sizeof(ExportSession));
		session.clientSvcId = span.clientSvcIdOfBufferedBlock;
		session.totalLength = span.lengthOfBufferedBlock;
		session.redPartLength = span.redLengthOfBufferedBlock;
		encodeSdnv(&(session.clientSvcIdSdnv), session.clientSvcId);

		session.maxCheckpoints = getMaxReports(session.redPartLength,
				vspan, 0);

		extents = lyst_create_using(getIonMemoryMgr());
		extent = (ExportExtent *) MTAKE(sizeof(ExportExtent));
		if(extent && extents)
		{
			lyst_insert_last(extents, extent);
		}

		extent->offset = 0;
		extent->length = session.totalLength;
		while (ckptSerialNbr == 0)
		{
			ckptSerialNbr = rand();
			ckptSerialNbr %= LTP_SERIAL_NBR_LIMIT;
		}
		result = issueSegments(sdr, &span, vspan, &session,
				span.currentExportSessionObj, extents, 0,
				ckptSerialNbr);
		MRELEASE(extent);
		lyst_destroy(extents);
		if (result < 0)
		{
			printf("can not segment block.");
			sdr_cancel_xn(sdr);
			returnCode = 1;
			break;
		}

		enqueueNotice(vdb->clients + session.clientSvcId,
				vdb->ownEngineId, session.sessionNbr,
				0, 0, LtpExportSessionStart, 0, 0, 0);

		sdr_write(sdr, span.currentExportSessionObj, (char *) &session,
				sizeof(ExportSession));

		/* reinit span's block buffer. */
		span.ageOfBufferedBlock = 0;
		span.lengthOfBufferedBlock = 0;
		span.redLengthOfBufferedBlock = 0;
		span.clientSvcIdOfBufferedBlock = 0;
		span.currentExportSessionObj = 0;
		sdr_write(sdr, spanObj, (char *) &span, sizeof(LtpSpan));
		sdr_end_xn(sdr);

		/* start new export session for the next block. */
		startExportSession(sdr, spanObj, vspan);
		sdr_begin_xn(sdr);
		sdr_stage(sdr, (char *) &span, spanObj, sizeof(LtpSpan));
	}

	printf("ltpmeter has ended.");
	ionDetach();
	
	return returnCode;
}