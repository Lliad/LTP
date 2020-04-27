#include "ltpP.h"

/* funcs need to be fix :
 *		getLtpVdb  
 *
 *      findSpan        */

int	ltp_engine_is_started()
{/* ok */
	LtpVdb	*vdb;
	
	vdb = getLtpVdb();
	if (vdb)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

static int	sduCanBeAppendedToBlock(LtpSpan *span,
			unsigned int clientSvcId,
			unsigned int redPartLength)
{ /* ok */
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
{ /* ok */
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

void assign_notice(LtpNotice notice, LtpNoticeType *type,
		LtpSessionId *sessionId, unsigned char *reasonCode,
		unsigned char *endOfBlock, unsigned int *dataOffset,
		unsigned int *dataLength, Object *data)
{/* ok */
	*data = notice.data;
	*type = notice.type;
	*reasonCode = notice.reasonCode;
	*endOfBlock = notice.endOfBlock;
	*dataOffset = notice.dataOffset;
	*dataLength = notice.dataLength;
	sessionId->sourceEngineId = notice.sessionId.sourceEngineId;
	sessionId->sessionNbr = notice.sessionId.sessionNbr;
}
		
int	ltp_get_notice(unsigned int clientSvcId, LtpNoticeType *type,
		LtpSessionId *sessionId, unsigned char *reasonCode,
		unsigned char *endOfBlock, unsigned int *dataOffset,
		unsigned int *dataLength, Object *data)
{/* ok */
	Sdr		sdr;
	LtpVdb		*vdb;
	LtpVclient	*client;
	Object		elt;
	Object		noticeAddr;
	LtpNotice	notice;

	/* parameter check */
	if (clientSvcId > MAX_LTP_CLIENT_NBR)
	{
		return -1;
	}
	if(type == NULL)
	{
		return -1;
	}
	if(sessionId == NULL)
	{
		return -1;
	}
	if(reasonCode == NULL)
	{
		return -1;
	}
	if (endOfBlock == NULL)
	{
		return -1;
	}
	if (dataOffset == NULL)
	{
		return -1;
	}
	if (dataLength == NULL)
	{
		return -1;
	}
	if (data == NULL)
	{
		return -1;
	}
	
	*type = LtpNoNotice;	/*	Default.			*/
	*data = 0;		/*	Default.			*/
	
	vdb = getLtpVdb();
	sdr = getIonsdr();
	
	sdr_begin_xn(sdr);
	client = vdb->clients + clientSvcId;

	elt = sdr_list_first(sdr, client->notices);
	while (elt == 0)
	{
		sdr_exit_xn(sdr);

		/*	wait for semaphore from ltp	 */
		if (sm_SemTake(client->semaphore) < 0)
		{
			printf("take semaphore failed.\n");
			return -1;
		}

		if (sm_SemEnded(client->semaphore))
		{
			printf("cilent server is terminated.\n");
			return -1;
		}

		sdr_begin_xn(sdr);
		elt = sdr_list_first(sdr, client->notices);
	}

	/*	Got next inbound notice.  Remove it from the queue
	 *	for this client.					*/
	noticeAddr = sdr_list_data(sdr, elt);
	sdr_list_delete(sdr, elt, (SdrListDeleteFn) NULL, NULL);
	sdr_read(sdr, (char *) &notice, noticeAddr, sizeof(LtpNotice));
	sdr_free(sdr, noticeAddr);
	
	assign_notice(notice, type, sessionId, reasonCode, endOfBlock, 
				dataOffset, dataLength, data);

	sdr_end_xn(sdr);
	return 0;
}

void	ltp_interrupt(unsigned int clientSvcId)
{/* ok */
	LtpVdb		*vdb;
	LtpVclient	*client;

	if (clientSvcId <= MAX_LTP_CLIENT_NBR)
	{
		vdb = getLtpVdb();
		client = vdb->clients + clientSvcId;
		if (client->semaphore != SM_SEM_NONE)
		{
			sm_SemGive(client->semaphore);
		}
	}
}

void	ltp_release_data(Object data)
{/* ok */
	Sdr	sdr = getIonsdr();

	if (data)
	{
		sdr_begin_xn(sdr);
		zco_destroy(sdr, data);
		sdr_end_xn(sdr);
	}
}

void	ltp_close(unsigned int clientSvcId)
{/* ok */
	Sdr		sdr;
	LtpVdb		*vdb;
	LtpVclient	*client;

	if (clientSvcId > MAX_LTP_CLIENT_NBR)
	{
		return;
	}
	
	sdr = getIonsdr();
	sdr_begin_xn(sdr);
	
	vdb = getLtpVdb();
	client = vdb->clients + clientSvcId;
	client->pid = -1;
	
	sdr_exit_xn(sdr);
}