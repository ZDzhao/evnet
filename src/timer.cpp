#include "timer.h"


static void timer_cb(int fd, short event, void *arg) {
    TimerCBContext *tc = (TimerCBContext*)arg;
    if (tc) {
        tc->cb();
    }
}

Timer::Timer(event_base *base, const timerTask &task) {
    tc = new TimerCBContext{task};
    timer = evtimer_new(base, timer_cb, tc);
    pending = false;
}

Timer::Timer(event_base *base, int second, const timerTask &task, bool temporary) {
    tc = new TimerCBContext{task};
    if(temporary) {
        timer = evtimer_new(base, timer_cb, tc);
    } else {
        timer = event_new(base, -1, EV_PERSIST, timer_cb, tc);
    }

    struct timeval tv;
    tv.tv_sec = second;
    tv.tv_usec = 0;
    evtimer_add(timer, &tv);
    pending = true;
}

Timer::~Timer() {
    cancle();
}

void Timer::add(int seconds) {
    if(!pending) {
        struct timeval tv;
        tv.tv_sec = seconds;
        tv.tv_usec = 0;
        evtimer_add(timer, &tv);
    }
}

void Timer::cancle() {
    if(timer) {
        event_del(timer);
    }
    event_free(timer);
    delete tc;
}
