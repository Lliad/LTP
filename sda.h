#include "library.h"

/* all relay on ltp block */

void	sda_interrupt();

int 	sda_send(uvast destinationEngineId, unsigned int clientSvcId,
		Object clientServiceData);

int		sda_run(SdaDelimiterFn delimiter, SdaHandlerFn handler);