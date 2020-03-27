/* sda.c */

#include "sda.h"
#include "stdio.h"

#define	SdaLtpClientId	(2)

static int	_running(int *newState)
{
	static int	state = 0;

	if (newState)
	{
		state = *newState;
	}

	return state;
}

int	sda_send(uvast destinationEngineId, unsigned int clientSvcId,
		Object clientServiceData)
{
	Sdr		sdr = getIonsdr();/* def in ion.h */
	Sdnv		sdnvBuf;/* def in platform.h */
	Object		sdaZco;
	LtpSessionId	sessionId;/* def in ltp.h */

	encodeSdnv(&sdnvBuf, clientSvcId);
	sdaZco = zco_clone(sdr, clientServiceData, 0,
			zco_source_data_length(sdr, clientServiceData));
	if (sdaZco != (Object) ERROR && sdaZco != 0)
	{
		zco_prepend_header(sdr, sdaZco, (char *) sdnvBuf.text,
				sdnvBuf.length);
		zco_bond(sdr, sdaZco);
	}

	if (sdr_end_xn(sdr) < 0)
	{
		printf("Can't prepend client service ID.\n");
		return -1;
	}

	switch (ltp_send(destinationEngineId, SdaLtpClientId, sdaZco,
			LTP_ALL_RED, &sessionId))/* def in ltp.h */
	{
	case 0:
		printf("Unable to send service data item via LTP.");
		break;

	case -1:
		printf("sd_send failed.\n");
		return -1;
	}
	
	/*  succes */
	return 0;
}

static int	receiveSdaItems(SdaDelimiterFn delimiter, SdaHandlerFn handler,
			Object zco, uvast senderEngineNbr)
{
	Sdr		sdr = getIonsdr();
	uvast		bytesHandled = 0;
	ZcoReader	reader;
	vast		bytesReceived;
	unsigned char	buffer[2048];
	int		offset;
	uvast		clientId;
	vast		itemLength;
	Object		itemZco;
	int 		state; /* for return */

	/* receiving loop */
	zco_start_receiving(zco, &reader);
	while (1)
	{ 
		bytesReceived = zco_receive_source(sdr, &reader, sizeof buffer,
				(char *) buffer);
		switch (bytesReceived)
		{
		case -1:
			printf("Error");
			return -1;

		case 0:
			return 0;
		}
		
		/* succes */

		offset = decodeSdnv(&clientId, buffer);
		if (offset == 0)
		{
			printf("No SDA item at start of LTP block.");
			return 0;
		}

		bytesHandled += offset;
		itemLength = delimiter(clientId, buffer + offset,
				bytesReceived - offset);
		switch (itemLength)
		{
		case -1:
			printf("Failure calculating SDA item length.\n");
			return -1;

		case 0:
			printf("Invalid SDA item in LTP block.\n");
			return 0;
		}

		/*	Clone the client data unit   */

		itemZco = zco_clone(sdr, zco, bytesHandled, itemLength);

		zco_destroy(sdr, itemZco);
		bytesHandled += itemLength;

		zco_start_receiving(zco, &reader);
		state = zco_receive_source(sdr, &reader, bytesHandled, NULL);
		switch (state)
		{
		case -1:
			printf("Can't skip over handled items.");
			return -1;

		case 0:
			printf("LTP-SDA block file access error.");
			return 0;
		}
	}
}

int	sda_run(SdaDelimiterFn delimiter, SdaHandlerFn handler)
{
	Sdr		sdr;/* def in sdrxn.h */
	int		state = 0;
	LtpNoticeType	type;/* def in ltp.h */
	LtpSessionId	sessionId;/* def in ltp.h */
	unsigned char	reasonCode;
	unsigned char	endOfBlock;
	unsigned int	dataOffset;
	unsigned int	dataLength;
	Object		data;
	
	/* init */

	if (ltp_attach() < 0)
	{
		printf("SDA can't initialize LTP.");
		return -1;
	}

	if (ltp_open(SdaLtpClientId) < 0)
	{
		printf("SDA can't open client access.\n");
		return -1;
	}

	sdr = getIonsdr();
	
	/* main loop */
	while (_running(NULL))
	{
		if (ltp_get_notice(SdaLtpClientId, &type, &sessionId,
				&reasonCode, &endOfBlock, &dataOffset,
				&dataLength, &data) < 0)
		{
			printf("SDA failed getting LTP notice.\n");
			continue;
		}

		switch (type)
		{
		case LtpExportSessionComplete:
		case LtpExportSessionCanceled:
			if (data == 0)
			{
				break;
			}

			zco_destroy(sdr, data);
			if (sdr_end_xn(sdr) < 0)
			{
				printf("Can't destroy block.\n");
				_running(&state);
			}

			break;

		case LtpImportSessionCanceled:
			break;

		case LtpRecvRedPart:
			if (!endOfBlock)
			{
				printf("SDA block partially green.\n");
				zco_destroy(sdr, data);
				if (sdr_end_xn(sdr) < 0)
				{
					printf("Can't destroy block.\n");
					_running(&state);
				}

				break;
			}

			if (receiveSdaItems(delimiter, handler, data,
					sessionId.sourceEngineId) < 0)
			{
				printf("Can't get SDA item.\n");
				_running(&state);
			}

			zco_destroy(sdr, data);
			if (sdr_end_xn(sdr) < 0)
			{
				printf("Can't destroy block.\n");
				_running(&state);
			}

			break;

		case LtpRecvGreenSegment:
			printf("SDA received a green segment.\n");
			zco_destroy(sdr, data);
			if (sdr_end_xn(sdr) < 0)
			{
				printf("Can't destroy block.\n");
				_running(&state);
			}

			break;

		default:
			break;
		}
	}

	printf("SDA reception has ended.\n");
	ltp_close(SdaLtpClientId);
	
	return 0;
}

void	sda_interrupt()
{
	ltp_interrupt(SdaLtpClientId); /* def in ltp.h */
}
