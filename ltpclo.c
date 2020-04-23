#include <unistd.h>
#include <stdlib.h>

#include "ltpcla.h"
#include "ipnfw.h"
#include "zco.h"
#include "platform_sm.c"

int	main(int argc, char *argv[])
{
	char		*ductName = (argc > 1 ? argv[1] : NULL);

	Sdr		sdr;
	VOutduct	*vduct;
	PsmAddress	vductElt;
	Address addr;
	uvast 		destEngineId;
	Outduct		outduct;
	Object		bundleZco;
	BpAncillaryData	ancillaryData;
	unsigned int	redPartLength;
	LtpSessionId	sessionId;
	
	int		running_state = 1;

	/* check command-line variable vailate or not */
	if (ductName == NULL)
	{
		printf("command need ductName.\n");
		return 0;
	}
	
	sdr = getIonsdr();
	findOutduct("ltp", ductName, &vduct, &vductElt);
	if (vductElt == 0)
	{
		printf("can not find %s.\n", *ductName);
		return -1;
	}

	if (vduct->cloPid != getpid())
	{
		printf("CLO task is already started for this duct.\n");
		return -1;
	}
	
	/* init */
	if (bpAttach() < 0)
	{
		printf("attach bp failed.\n")；
		return -1;
	}

	ipnInit();
	if (ltp_attach() < 0)
	{
		printf("attach ltp failed.\n");
		return -1;
	}
	
	sdr_begin_xn(sdr);		/*	Lock the heap */
	addr = sdr_list_data(sdr, vduct->outductElt);
	sdr_read(sdr, (char *) &outduct, addr, sizeof(Outduct));
	sdr_exit_xn(sdr);			/*	Unlock	*/
	
	destEngineId = strtoull(ductName, NULL, 10);

	/*  main loop */
	printf("ltpclo is running.\n");
	while (running_state)
	{
		bpDequeue(vduct, &bundleZco, &ancillaryData, -1);

		if (bundleZco == 0)	/*	Outduct closed	*/
		{
			running_state = 0;	/*	stop clo */
			break;
		}

		if (bundleZco == 1)	/* corrupt bundle	*/
		{
			continue;	/* next loop */
		}

		if (ancillaryData.flags & BP_BEST_EFFORT)
		{
			redPartLength = 0;
		}
		else
		{
			redPartLength = LTP_ALL_RED;
		}
		
		/* ltp_send need to be fix */
		switch (ltp_send(destEngineId, BpLtpClientId, bundleZco,
				redPartLength, &sessionId))
		{	
		case -1:
			printf("ltp send failed.\n"); /* send failed */
			running_state = 0;	/*	stop clo. */
		
		case 1:
			continue; /* send success */
		}
	}
	
	printf("ltpclo duct has ended.\n");
	ionDetach();
	
	return 0;
}
