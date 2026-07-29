#pragma once
#include <algorithm>
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
    inline size_t subscribe(const callback_t& callback) {
        auto it = std::find(callback_list_.begin(), callback_list_.end(), nullptr);
        if(it != callback_list_.end()) {
            *it = callback;
            return std::distance(callback_list_.begin(), it);
        } else {
            callback_list_.push_back(callback);
            return callback_list_.size() - 1;
        }
    }

    void unsubscribe(size_t id) {
        callback_list_[id] = nullptr;
    }

    IEvent(const IEvent&) = delete;
    IEvent& operator=(const IEvent&) = delete;
};

template<typename ...T> class EventInvoker : public IEvent<T...> {
    public:
    void invoke(const T&... arg) {
        for(typename IEvent<T...>::callback_t callback : this->callback_list_) {
            if(callback)
                callback(arg...);
        }
    }
};
