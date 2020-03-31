#include "library.h"

/* main */

void	zco_start_receiving(Object zco, ZcoReader *reader);

vast	zco_receive_source(Sdr sdr, ZcoReader *reader, vast length,
		char *buffer);

void	zco_destroy(Sdr sdr, Object zco);
		
/* for other use */

Object	zco_clone(Sdr sdr, Object fromZcoObj, vast offset, vast length);

vast	zco_source_data_length(Sdr sdr, Object zcoObj);

int	zco_prepend_header(Sdr sdr, Object zco, char *text, vast length);

int	zco_bond(Sdr sdr, Object zco);