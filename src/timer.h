#ifndef PERSISTTIMER_H
#define PERSISTTIMER_H

#include <functional>

#include "libevent_headers.h"

typedef std::function<void()> timerTask;

struct TimerCBContext {
    timerTask cb;
};

class Timer {
  public:
    Timer(struct event_base *base, const timerTask &task);
    Timer(struct event_base *base, int second, const timerTask& task, bool temporary=false);
    ~Timer();

    void add(int seconds);
    void cancle();
  private:
    struct event *timer;
    TimerCBContext *tc;
    bool pending;
};

#endif // PERSISTTIMER_H
