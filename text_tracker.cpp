#include "tracker/text_tracker.hpp"
#include <iostream>

TextTracker::TextTracker(ActivityBatcher& batcher, int cooldown_seconds)
    : batcher_(batcher), cooldown_seconds_(cooldown_seconds) {}

void TextTracker::handle_message_create(const dpp::message_create_t& event) {
    const auto& msg = event.msg;

    // Ignore bot messages and webhooks
    if (msg.author.is_bot() || msg.webhook_id != 0) {
        return;
    }

    // Ignore DMs if only guild tracking is desired
    if (msg.guild_id == 0) {
        return;
    }

    std::string guild_id_str = std::to_string(msg.guild_id);
    std::string user_id_str = std::to_string(msg.author.id);
    UserKey key{guild_id_str, user_id_str};

    auto now = std::chrono::system_clock::now();
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = last_message_times_.find(key);
    if (it != last_message_times_.end()) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->second).count();
        if (elapsed < cooldown_seconds_) {
            // Message sent during cooldown period; ignore for stats counter
            return;
        }
    }

    // Update timestamp & trigger increment in batcher
    last_message_times_[key] = now;
    batcher_.add_messages(guild_id_str, user_id_str, 1);
}
