/* declare there's another function defined in other place (naskfunc.obj) */

void io_hlt(void);


void HariMain(void)
{

fin:
	io_hlt(); /* �����naskfunc.nas��_io_hlt�����s����܂� */
	goto fin;

}
