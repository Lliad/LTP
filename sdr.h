#include "library.h"

/* main */

extern int		sdr_begin_xn(Sdr sdr);

extern int		sdr_end_xn(Sdr sdr);

#define sdr_insert(sdr, from, size) \
Sdr_insert(__FILE__, __LINE__, sdr, from, size)
extern Object		Sdr_insert(const char *file, int line,
				Sdr sdr, char *from, size_t size);

/* for other use */

void	sdr_read(Sdr sdrv, char *into, Address from, size_t length);

int	sdr_in_xn(Sdr sdrv);