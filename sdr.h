#include "library.h"

/* main */

extern int		sdr_begin_xn(Sdr sdr); /* done */

extern int		sdr_end_xn(Sdr sdr); /* undone: terminateXn */

#define sdr_insert(sdr, from, size) \
Sdr_insert(__FILE__, __LINE__, sdr, from, size)
extern Object		Sdr_insert(const char *file, int line,
				Sdr sdr, char *from, size_t size); /* undone: joinTrace
													*		_sdrmalloc
													*		_sdrput */

/* for other use */

void	sdr_read(Sdr sdrv, char *into, Address from, size_t length);

int	sdr_in_xn(Sdr sdrv);

void	sdr_stage(Sdr sdrv, char *into, Object from, size_t length);

#define sdr_write(sdr, into, from, size) \
Sdr_write(__FILE__, __LINE__, sdr, into, from, size)
extern void		Sdr_write(const char *file, int line,
				Sdr sdr, Address into, char *from, size_t size);