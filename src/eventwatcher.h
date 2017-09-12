#ifndef EVENTWATCHER_H
#define EVENTWATCHER_H

#include <functional>

struct event;
struct event_base;

class EventWatcher {
  public:
    typedef std::function<void()> Handler;

    virtual ~EventWatcher();

    bool Init();

    //It MUST be called in the event thread.
    void Cancel();

    void SetCancelCallback(const Handler& cb);

  protected:
    //It MUST be called in the event thread.
    bool Watch();

  protected:
    EventWatcher(struct event_base* evbase, const Handler& handler);

    void Close();
    void FreeEvent();

    virtual bool DoInit() = 0;
    virtual void DoClose() {}

  protected:
    struct event* event_;
    struct event_base* evbase_;
    bool attached_;
    Handler handler_;
    Handler cancel_callback_;
};

class PipeEventWatcher : public EventWatcher {
  public:
    PipeEventWatcher(struct event_base* event_base, const Handler& handler);

    bool AsyncWait();
    void Notify();
    int wfd() const {
        return pipe_[0];
    }
  private:
    virtual bool DoInit();
    virtual void DoClose();
    static void HandlerFn(int fd, short which, void* v);

    int pipe_[2]; // Write to pipe_[0] , Read from pipe_[1]
};

#endif // EVENTWATCHER_H
