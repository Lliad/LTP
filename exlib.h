/* exlib.h : some struct copy from ION source code */

#include "library.h"

/* zco field */

typedef struct
{
	double		fileOccupancy;
	double		maxFileOccupancy;
	double		bulkOccupancy;
	double		maxBulkOccupancy;
	double		heapOccupancy;
	double		maxHeapOccupancy;
} ZcoBook;

typedef struct
{
	ZcoBook		books[2];
} ZcoDB;