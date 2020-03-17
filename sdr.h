/*
	sdr.h
*/

typedef unsigned long		uaddr;
typedef uaddr		SdrObject;
#define	Object		SdrObject
typedef uaddr		SdrAddress;
#define	Address		SdrAddress

/*		Private definitions of SDR list structures.		*/

typedef struct
{
	Address		userData;
	Object		first;
	Object		last;
	size_t		length;
} SdrList;

typedef struct
{
	Object		list;	/*	list that this element is in	*/
	Object		prev;
	Object		next;
	Object		data;
} SdrListElt;

/*	List management functions	*/

static void	sdr_list__clear(SdrList *list)
{
	list->userData = 0;
	list->first = 0;
	list->last = 0;
	list->length = 0;
}

static void	sdr_list__elt_clear(SdrListElt *elt)
{
	elt->list = 0;
	elt->prev = 0;
	elt->next = 0;
	elt->data = 0;
}