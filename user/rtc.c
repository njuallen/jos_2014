// get rtc
#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	struct Rtc rtc;
	sys_read_rtc(&rtc);
	cprintf("second: %d\nminute: %d\nhour: %d\n"
			"day: %d\nmonth: %d\nyear: %d\n",
			rtc.second, rtc.minute, rtc.hour,
			rtc.day, rtc.month, rtc.year);
}
