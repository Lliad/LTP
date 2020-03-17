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

/*不同的报文类型，应该有不同的处理方法 */

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