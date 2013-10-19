/* declare there's another function defined in other place (naskfunc.obj) */

void io_hlt(void);


void HariMain(void)
{

fin:
	io_hlt(); /* ‚±‚ê‚Ånaskfunc.nas‚Ì_io_hlt‚ªÀs‚³‚ê‚Ü‚· */
	goto fin;

}
