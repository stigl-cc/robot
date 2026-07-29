#include <algorithm>
#include <callback.hh>

template<typename ...T> size_t IEvent<T...>::subscribe(const callback_t& callback) {
    auto it = std::find(callback_list_.begin(), callback_list_.end(), nullptr);
    if(it != callback_list_.end()) {
        *it = callback;
        return std::distance(callback_list_.begin(), it);
    } else {
        callback_list_.push_back(callback);
        return callback_list_.size() - 1;
    }
}

template<typename ...T> void IEvent<T...>::unsubscribe(size_t id) {
    callback_list_[id] = nullptr;
}

template<typename ...T> void EventInvoker<T...>::invoke(const T&... arg) {
    for(typename IEvent<T...>::callback_t callback : this->callback_list_) {
        if(callback)
            callback(arg...);
    }
}
