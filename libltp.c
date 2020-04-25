#include "ltpP.h"

static int	sduCanBeAppendedToBlock(LtpSpan *span,
			unsigned int clientSvcId,
			unsigned int redPartLength)
{
	Sdr	sdr = getIonsdr();

	if (span->lengthOfBufferedBlock == 0)
	{
		if (sdr_list_length(sdr, span->exportSessions)
				> span->maxExportSessions)
		{
			if (redPartLength) /*	red data. */
			{
				return 0;
			}

			return 1; /* all green. */
		}

		return 1;
	}

	if (span->lengthOfBufferedBlock > span->redLengthOfBufferedBlock)
	{
		return 0;
	}

	if (span->lengthOfBufferedBlock >= span->aggrSizeLimit)
	{
		return 0;
	}

	if (clientSvcId != span->clientSvcIdOfBufferedBlock)
	{
		return 0;
	}

	return 1; /* ok */
}

int	ltp_send(uvast destinationEngineId, unsigned int clientSvcId,
		Object clientServiceData, unsigned int redPartLength,
		LtpSessionId *sessionId)
{
	Sdr		sdr;
	LtpVspan	*vspan;
	PsmAddress	vspanElt;
	unsigned int	dataLength;
	Object		spanObj;
	LtpSpan		span;
	ExportSession	sessionBUF;
	ExportSession 	*session = &sessionBUF;

	if(clientSvcId > MAX_LTP_CLIENT_NBR)
	{
		return -1;
	}

	if(!clientServiceData)
	{
		return -1;
	}

	sdr = getIonsdr();
	sdr_begin_xn(sdr);
	findSpan(destinationEngineId, &vspan, &vspanElt);
	if (vspanElt == 0)
	{
		sdr_exit_xn(sdr);
		printf("Destination engine unknown.");
		return -1;
	}

	dataLength = zco_length(sdr, clientServiceData);
	if (redPartLength > dataLength)
	{
		redPartLength = dataLength;
	}

	spanObj = sdr_list_data(sdr, vspan->spanElt);
	sdr_stage(sdr, (char *) &span, spanObj, sizeof(LtpSpan));

	while (1)
	{
		if (span.currentExportSessionObj)
		{
			if (sduCanBeAppendedToBlock(&span, clientSvcId,
					redPartLength))
			{
				break; /* Out of loop. */
			}
		}

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
				printf("span is stopped.");
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
				printf("span is stopped.");
				return -1;
			}
		}

		sdr_begin_xn(sdr);
		sdr_stage(sdr, (char *) &span, spanObj, sizeof(LtpSpan));
	}
	
	/* append to the block */
	sdr_read(sdr, (char *) &sessionBUF, span.currentExportSessionObj, sizeof(ExportSession));
	sdr_list_insert_last(sdr, session->svcDataObjects, clientServiceData);
	
	/* update the span */
	span.clientSvcIdOfBufferedBlock = clientSvcId;
	span.lengthOfBufferedBlock += dataLength;
	span.redLengthOfBufferedBlock += redPartLength;
	
	sdr_write(sdr, spanObj, (char *) &span, sizeof(LtpSpan));
	sdr_end_xn(sdr);

	return 1;
}
