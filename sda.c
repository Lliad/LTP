#include "sda.h"
#include <sched.h> /* for sched_yield */

void	sda_interrupt()
{
	ltp_interrupt(SdaLtpClientId); /* an undeal func */
}

int	sda_send(uvast destinationEngineId, unsigned int clientSvcId,
		Object clientServiceData)
{
	Sdr		sdr = getIonsdr();
	Sdnv		sdnvBuf;
	Object		sdaZco;
	LtpSessionId	sessionId;

	if(!destinationEngineId)
	{
		return ERROR;
	}

	if(!clientSvcId)
	{
		return ERROR;
	}
	
	if(!clientServiceData)
	{
		return ERROR;
	}
	
	encodeSdnv(&sdnvBuf, clientSvcId);
	if(!(sdr_begin_xn(sdr)))
	{
		return ERROR;
	}
	
	sdaZco = zco_clone(sdr, clientServiceData, 0, /* here is an undeal func: zco_clone */
			zco_source_data_length(sdr, clientServiceData)); /* here is an undeal func: zco_source_data_length */
	if (sdaZco)
	{
		zco_prepend_header(sdr, sdaZco, (char *) sdnvBuf.text, /* here is an undeal func: zco_prepend_header */
				sdnvBuf.length);
		zco_bond(sdr, sdaZco); /* here is an undeal func: zco_bond */
	}

	if (sdr_end_xn(sdr) < 0)
	{
		printf("Can't prepend client service ID.\n");
		return -1;
	}

	switch (ltp_send(destinationEngineId, SdaLtpClientId, sdaZco, /* here is an undeal func: ltp_send, def in ltp.h*/
			LTP_ALL_RED, &sessionId))
	{
	case 0:
		printf("Unable to send service data item via LTP.\n");
		break;

	case -1:
		printf("sd_send failed.\n");
		return -1;
	}

	return 0;
}

int	sda_run(SdaDelimiterFn delimiter, SdaHandlerFn handler)
{
	Sdr		sdr;
	int		state = 1;
	LtpNoticeType	type;
	LtpSessionId	sessionId;
	unsigned char	reasonCode;
	unsigned char	endOfBlock;
	unsigned int	dataOffset;
	unsigned int	dataLength;
	Object		data;

	if (ltp_attach() < 0)
	{
		printf("SDA can't initialize LTP.\n");
		return -1;
	}

	if (ltp_open(SdaLtpClientId) < 0)
	{
		printf("SDA can't open client access.\n");
		return -1;
	}

	sdr = getIonsdr();

	/*	Can now start receiving notices.  On failure,
	 *	terminate SDA.						*/

	//isignal(SIGINT, handleQuit);
	_running(&state);
	state = 0;	/*	Prepare for stop.			*/
	while (_running(NULL))
	{
		if (ltp_get_notice(SdaLtpClientId, &type, &sessionId,
				&reasonCode, &endOfBlock, &dataOffset,
				&dataLength, &data) < 0)
		{
			printf("[?] SDA failed getting LTP notice.\n");
			_running(&state);
			continue;
		}

		switch (type)
		{
		case LtpExportSessionComplete:	/*	Xmit success.	*/
		case LtpExportSessionCanceled:	/*	Xmit failure.	*/
			if (data == 0)		/*	Ignore it.	*/
			{
				break;		/*	Out of switch.	*/
			}

			if(!(sdr_begin_xn(sdr)))
			{
				return ERROR;
			}
			zco_destroy(sdr, data);
			if (sdr_end_xn(sdr) < 0)
			{
				printf("Crashed on data cleanup.\n");
				_running(&state);
			}

			break;		/*	Out of switch.		*/

		case LtpImportSessionCanceled:
			/*	None of the red data for the import
			 *	session (if any) have been received
			 *	yet, so nothing to discard.		*/

			break;		/*	Out of switch.		*/

		case LtpRecvRedPart:
			if (!endOfBlock)
			{
				/*	Block is partially red and
				 *	partially green, an error.	*/

				printf("SDA block partially green.\n");
				if(!(sdr_begin_xn(sdr)))
				{
					return ERROR;
				}
				zco_destroy(sdr, data);
				if (sdr_end_xn(sdr) < 0)
				{
					printf("Can't destroy block.\n");
					_running(&state);
				}

				break;		/*	Out of switch.	*/
			}

			if(!(sdr_begin_xn(sdr)))
			{
				return ERROR;
			}
			if (receiveSdaItems(delimiter, handler, data,
					sessionId.sourceEngineId) < 0)
			{
				printf("Can't acquire SDA item(s).\n");
				_running(&state);
			}

			zco_destroy(sdr, data);
			if (sdr_end_xn(sdr) < 0)
			{
				printf("Can't release block.\n");
				_running(&state);
			}

			break;		/*	Out of switch.		*/

		case LtpRecvGreenSegment:
			printf("SDA received a green segment.\n");
			if(!(sdr_begin_xn(sdr)))
			{
				return ERROR;
			}
			zco_destroy(sdr, data);
			if (sdr_end_xn(sdr) < 0)
			{
				printf("Can't destroy item.\n");
				_running(&state);
			}

			break;		/*	Out of switch.		*/

		default:
			break;		/*	Out of switch.		*/
		}

		/*	Make sure other tasks have a chance to run.	*/

		sched_yield();
	}

	printf("[i] SDA reception has ended.");

	/*	Free resources.						*/

	ltp_close(SdaLtpClientId);
	return 0;
}