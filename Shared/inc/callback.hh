#pragma once
#include <functional>
#include <sys/types.h>

template<typename ...T> class IEvent {
    public:
    typedef std::function<void(T...)> callback_t;

    protected:
    typedef std::vector<callback_t> callback_list_t;

    callback_list_t callback_list_;
    IEvent() = default;

    public:
    size_t subscribe(const callback_t&);
    void unsubscribe(size_t id);

    IEvent(const IEvent&) = delete;
    IEvent& operator=(const IEvent&) = delete;
};

template<typename ...T> class EventInvoker : public IEvent<T...> {
    public:
    void invoke(const T&... arg);
};
