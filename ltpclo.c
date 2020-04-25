#include "ltpcla.h"
#include "ipnfw.h"
#include "zco.h"

#include <stdlib.h>

int	main(int argc, char *argv[])
{
	char		*ductName;
	Sdr		sdr;
	VOutduct	*vduct;
	PsmAddress	vductElt;
	uvast		destEngineNbr;
	Outduct		outduct;
	int		running = 1;
	Object		bundleZco;
	BpAncillaryData	ancillaryData;
	unsigned int	redPartLength;
	LtpSessionId	sessionId;
	
	if(argc > 1)
	{
		ductName = argv[1];
	}
	else
	{
		printf("need destination engine number.\n");
		return 0;
	}
	destEngineNbr = strtoull(ductName, NULL, 10);

	if (bpAttach() < 0)
	{
		printf("attach bp failed.\n");
		return -1;
	}

	sdr = getIonsdr();
	findOutduct("ltp", ductName, &vduct, &vductElt);
	if (vductElt == 0)
	{
		printf("can not find this duct.\n");
		return -1;
	}
	
	ipnInit();
	sdr_begin_xn(sdr);
	sdr_read(sdr, (char *) &outduct, sdr_list_data(sdr, vduct->outductElt),
			sizeof(Outduct));
	sdr_exit_xn(sdr);
	
	if (ltp_attach() < 0)
	{
		printf("attach ltp failed\n.");
		return -1;
	}
	
	/*	main loop	*/
	printf("ltpclo is running.");
	while (running)
	{
		bpDequeue(vduct, &bundleZco, &ancillaryData, -1);

		if (bundleZco == 0)
		{
			break;
		}

		if (bundleZco == 1)	/* corrupt bundle. */
		{
			continue; /* next loop.	*/
		}

		if (ancillaryData.flags & BP_BEST_EFFORT)
		{
			redPartLength = 0;
		}
		else
		{
			redPartLength = LTP_ALL_RED;
		}

		switch (ltp_send(destEngineNbr, BpLtpClientId, bundleZco,
				redPartLength, &sessionId))
		{
		case 1: /* send success */
			continue;

		case -1:
			printf("ltp send failed.\n");
			break;	
		}
	}
	ionDetach();
	
	return 0;
}