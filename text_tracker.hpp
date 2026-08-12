#ifndef TEXT_TRACKER_HPP
#define TEXT_TRACKER_HPP

#include <dpp/dpp.h>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include "db/activity_batcher.hpp"

class TextTracker {
public:
    TextTracker(ActivityBatcher& batcher, int cooldown_seconds = 15);
    ~TextTracker() = default;

    void handle_message_create(const dpp::message_create_t& event);

private:
    ActivityBatcher& batcher_;
    int cooldown_seconds_;
    std::unordered_map<UserKey, std::chrono::system_clock::time_point> last_message_times_;
    std::mutex mutex_;
};

#endif // TEXT_TRACKER_HPP
