/*

	libltp.c

*/

int ltp_trans(unsigned int clientId,
			engineId destinationEngineId,
			Object clientServiceData,
			unsigned int redLength)
{
}
					
int ltp_cancel_trans(LtpSessionId *sessionId)
{
}

int ltp_cancel_reception(LtpSessionId *sessionId)
{
}

int transSessionStart_indication(LtpSessionId *sessionId)
{
}

int receptionSessionStart_indication(LtpSessionId *sessionId)
{
}

int RedpartReception_indication(LtpSessionId *sessionId,
								unsigned int red_lenth,
								engineId sourceEngineID)
{
}

int InitTransCompletion_indication(LtpSessionId *sessionId)
{
}

int TransSessionCompletion_indication(LtpSessionId *sessionId)
{
}

int TransSessionCancel_indication(LtpSessionId *sessionId,
								int reason_code)
{
}

int ReceptionSessionCancel_indication(LtpSessionId *sessionId,
									int reason_code)
{
}

/* 不同的报文类型，应该有不同的处理方法 */

static int	handleDS()
{
}

static int	handleRS()
{
}

static int	handleRA()
{
}

static int	handleCS()
{
}

static int	handleCAS()
{
}

static int  handleCR()
{
}

static int  handleCAR()
{
}

int	ltpHandleSegment()
{
	/* 定义一些基本变量 */
	Sdr		sdr;
	LtpRecvSeg	segment;
	char		versionNbr;
	LtpSeg		*seg = &segment.seg;
	uvast		sourceEngineId;
	unsigned int	sessionNbr;
	
	/*	Handle segment according to its segment type code.	*/

	if ((seg->segTypeCode & LTP_CTRL_FLAG) == 0)	/*	Data.	*/
	{
		result = handleDataSegment();
	}
	else
	{
		/*	Segment is a control segment.			*/
 
		switch (seg->segTypeCode)
		{
		case LtpRS:
			result = handleRS();
			break;

		case LtpRAS:
			result = handleRA();
			break;

		case LtpCS:
			result = handleCS();
			break;

		case LtpCAS:
			result = handleCAS();
			break;

		case LtpCR:
			result = handleCR();
			break;

		case LtpCAR:
			result = handleCAR();
			break;

		default:
			break;
		}
	}
	
	return 0;
}