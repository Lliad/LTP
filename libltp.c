#include "ltpP.h"

static int	sduCanBeAppendedToBlock(LtpSpan *span,
			unsigned int clientSvcId,
			unsigned int redPartLength)
{
	Sdr	sdr = getIonsdr();

	if (span->lengthOfBufferedBlock == 0) /* empty block */
	{
		if (sdr_list_length(sdr, span->exportSessions)
				> span->maxExportSessions)
		{
			if (redPartLength)	/*	red data */
			{
				return 0;	/*	Okay. */
			}

			return 1;	/*	all green data	*/
		}
		
		return 1;
	}

	/*	block all ready contains data	*/
	if (span->lengthOfBufferedBlock > span->redLengthOfBufferedBlock)
	{
		return 0; /* contains red and green data */
	}
	
	if (clientSvcId != span->clientSvcIdOfBufferedBlock)
	{
		return 0; /* different destination */
	}

	if (span->lengthOfBufferedBlock >= span->aggrSizeLimit)
	{
		return 0; /* error */
	}

	return 1; /* can be appended */
}

int	ltp_send(uvast destinationEngineId, unsigned int clientSvcId,
		Object clientServiceData, unsigned int redPartLength,
		LtpSessionId *sessionId)
{
	LtpVdb		*vdb;
	Sdr		sdr;
	LtpVspan	*vspan;
	PsmAddress	vspanElt;
	unsigned int	dataLength;
	Object		spanObj;
	LtpSpan		span;
	OBJ_POINTER(ExportSession, session);

	if(clientSvcId > MAX_LTP_CLIENT_NBR)
	{
		printf("clientSvcId error.\n");
		return -1;
	}
	if(!clientServiceData)
	{
		printf("no data to trans.\n");
		return -1;
	}
	
	vdb = getLtpVdb();
	sdr = getIonsdr();

	sdr_begin_xn(sdr);
	findSpan(destinationEngineId, &vspan, &vspanElt);
	if (vspanElt == 0)
	{
		printf("can not find engineId: %lu.\n", destinationEngineId);
		sdr_exit_xn(sdr);
		return -1;
	}

	dataLength = zco_length(sdr, clientServiceData);

	/*	We spare the client service from needing to know the
	 *	exact length of the ZCO before calling ltp_send():
	 *	if the client service data is all red, the red length
	 *	LTP_ALL_RED can be specified and we simply reduce it
	 *	to the actual ZCO length here.				*/

	if (redPartLength > dataLength)
	{
		redPartLength = dataLength;
	}

	spanObj = sdr_list_data(sdr, vspan->spanElt);
	sdr_stage(sdr, (char *) &span, spanObj, sizeof(LtpSpan));

	/*	All service data units aggregated into any single
	 *	block must have the same client service ID, and
	 *	no service data unit can be added to a block that
	 *	has any green data (only all-red service data units
	 *	can be aggregated in a single block).			*/

	while (1)
	{
		if (span.currentExportSessionObj)
		{
			/*	Span has been initialized with a
			 *	session buffer (block) into which
			 *	service data can be inserted.		*/

			if (sduCanBeAppendedToBlock(&span, clientSvcId,
					redPartLength))
			{
				break;
			}
		}

		/*	Can't append service data unit to block.  Wait
		 *	until block is open for insertion of SDUs of
		 *	the same color as the SDU we're trying to send,
		 *	based on redPartLength.				*/

		sdr_exit_xn(sdr);
		if (redPartLength) /* red data */
		{
			if (sm_SemTake(vspan->bufOpenRedSemaphore) < 0)
			{
				printf("take buffer open semaphore failed.\n");
				return -1;
			}

			if (sm_SemEnded(vspan->bufOpenRedSemaphore))
			{
				printf("Span has been stopped.\n");
				return -1;
			}
		}
		else /* green data */
		{
			if (sm_SemTake(vspan->bufOpenGreenSemaphore) < 0)
			{
				printf("take buffer open semaphore failed.\n");
				return -1;
			}

			if (sm_SemEnded(vspan->bufOpenGreenSemaphore))
			{
				printf("Span has been stopped.\n");
				return -1;
			}
		}

		sdr_begin_xn(sdr);
		sdr_stage(sdr, (char *) &span, spanObj, sizeof(LtpSpan));
	}

	/*	Now append the outbound SDU to the block that is
	 *	currently being aggregated for this span and, if the
	 *	block buffer is now full or the block buffer contains
	 *	any green data, notify ltpmeter that block segmentation
	 *	can begin.						*/

	GET_OBJ_POINTER(sdr, ExportSession, session,
			span.currentExportSessionObj);
	sdr_list_insert_last(sdr, session->svcDataObjects, clientServiceData);
	
	/* update span */
	span.clientSvcIdOfBufferedBlock = clientSvcId;
	span.lengthOfBufferedBlock += dataLength;
	span.redLengthOfBufferedBlock += redPartLength;
	sdr_write(sdr, spanObj, (char *) &span, sizeof(LtpSpan));
	
	if (span.lengthOfBufferedBlock >= span.aggrSizeLimit
	|| span.redLengthOfBufferedBlock < span.lengthOfBufferedBlock)
	{
		sm_SemGive(vspan->bufClosedSemaphore);
	}
	sdr_end_xn(sdr);
	
	sessionId->sourceEngineId = vdb->ownEngineId;
	sessionId->sessionNbr = session->sessionNbr;
	
	return 1;
}