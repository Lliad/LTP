/* sdr.h */

extern int		sdr_begin_xn(Sdr sdr);

extern int		sdr_end_xn(Sdr sdr);

extern Object		sdr_insert(Sdr sdr, char *from, 
								size_t size);
