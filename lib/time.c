#include <inc/lib.h>
#include <inc/stdio.h>
#include <inc/clock.h>

#define MAX_LEN 1024
static char buf[MAX_LEN];

static char *months[] = {
	"Jan", "Feb", "Mar", "Apr", 
	"May", "Jun", "Jul", "Aug",
	"Sep", "Oct", "Nov", "Dec"
};

// transform Rtc to ASCII
// not thread safe
char *asctime(const struct Rtc *rtc) {
	int len = snprintf(buf, MAX_LEN, "%d %s %d %d:%d:%d",
			rtc->year, months[rtc->month], rtc->day,
			rtc->hour, rtc->minute, rtc->second);
	if(len > MAX_LEN)
		panic("the coresponding string of Rtc is too long!\n");
	return buf;
}

