#ifndef VOICE_TRACKER_HPP
#define VOICE_TRACKER_HPP

#include <dpp/dpp.h>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include "db/activity_batcher.hpp"

struct VoiceSession {
    dpp::snowflake channel_id;
    std::chrono::system_clock::time_point last_active_start;
    int64_t accumulated_active_seconds = 0;
    bool is_active = false;
    bool is_muted = false;
    bool is_deafened = false;
    bool is_afk = false;
};

class VoiceTracker {
public:
    explicit VoiceTracker(ActivityBatcher& batcher);
    ~VoiceTracker() = default;

    void handle_voice_state_update(const dpp::voice_state_update_t& event, dpp::cluster& bot);

    // Call on bot shutdown or guild sync to flush open voice sessions
    void flush_all_active_sessions();

private:
    ActivityBatcher& batcher_;
    std::unordered_map<UserKey, VoiceSession> sessions_;
    std::mutex mutex_;

    dpp::snowflake get_afk_channel_id(dpp::snowflake guild_id, dpp::cluster& bot);
};

#endif // VOICE_TRACKER_HPP
