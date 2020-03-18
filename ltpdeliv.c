/*
	ltpdeliv.c:ltp进行seg收发处理的daemon
*/

#include "ltp.h"

int main(int argc, char *argv[])
{
	/*定义一些变量*/
	
	Object		elt;
	Object		delivObj;
	Object		currentElt;
	LtpSeg		seg;
	unsigned int	clientSvcId;
	unsigned int	sessionNbr;
	uvast		sourceEngineId;
	Sdr		sdr;
	
	if (ltpInit(0) < 0)
	{
		putErrmsg("ltpdeliv can't initialize LTP.", NULL);
		return 1;
	}
	
	/*main loop*/
	writeMemo("ltpdeliv is running.");
	while(handle_que != 0)
	{
		ltpHandleSegment(seg);
		ltp_trans(seg);
		update_handle_list();
	}
	
	writeMemo("ltpdeliv has ended.");
	return 0;
}