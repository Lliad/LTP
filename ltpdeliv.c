/*
	ltpdeliv
*/

#include "ltp.h"

int main(int argc, char *argv[])
{
	/*定义一些变量*/
	Object		elt;
	
	if (ltpInit(0) < 0)
	{
		putErrmsg("ltpdeliv can't initialize LTP.", NULL);
		return 1;
	}
	
	/*main loop*/
	writeMemo("ltpdeliv is running.");
	while(deliverySemaphore != 0)//传输东西的信号量不为零，可以进行传输
	{
		elt = sdr_list_first();//获取第一个要传输的东西
		if(elt == NULL)
			break;//没有要传输的东西
		
		ltp_trans(elt);//传输
		update_sdr_list();//更新传输列表
	}
	
	writeMemo("ltpdeliv has ended.");
	return 0;
}