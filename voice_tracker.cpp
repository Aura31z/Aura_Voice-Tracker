#include "tracker/voice_tracker.hpp"
#include <iostream>

VoiceTracker::VoiceTracker(ActivityBatcher& batcher)
    : batcher_(batcher) {}

dpp::snowflake VoiceTracker::get_afk_channel_id(dpp::snowflake guild_id, dpp::cluster& /*bot*/) {
    dpp::guild* g = dpp::find_guild(guild_id);
    return g ? g->afk_channel_id : 0;
}

void VoiceTracker::handle_voice_state_update(const dpp::voice_state_update_t& event, dpp::cluster& bot) {
    const auto& state = event.state;
    dpp::snowflake guild_id = state.guild_id;
    dpp::snowflake user_id = state.user_id;

    if (guild_id == 0 || user_id == 0) return;

    std::string guild_id_str = std::to_string(guild_id);
    std::string user_id_str = std::to_string(user_id);
    UserKey key{guild_id_str, user_id_str};

    auto now = std::chrono::system_clock::now();
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = sessions_.find(key);

    // User left voice completely (channel_id == 0)
    if (state.channel_id == 0) {
        if (it != sessions_.end()) {
            auto& session = it->second;
            if (session.is_active) {
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - session.last_active_start).count();
                session.accumulated_active_seconds += elapsed;
            }

            if (session.accumulated_active_seconds > 0) {
                batcher_.add_voice_seconds(guild_id_str, user_id_str, session.accumulated_active_seconds);
            }

            sessions_.erase(it);
        }
        return;
    }

    // User joined or updated state in a voice channel
    dpp::snowflake afk_channel_id = get_afk_channel_id(guild_id, bot);
    bool muted = state.is_self_mute() || state.is_mute();
    bool deafened = state.is_self_deaf() || state.is_deaf();
    bool is_afk_channel = (afk_channel_id != 0 && state.channel_id == afk_channel_id);

    bool should_be_active = (!muted && !deafened && !is_afk_channel);

    if (it != sessions_.end()) {
        auto& session = it->second;

        if (session.is_active != should_be_active || session.channel_id != state.channel_id) {
            if (session.is_active) {
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - session.last_active_start).count();
                session.accumulated_active_seconds += elapsed;
            }

            session.channel_id = state.channel_id;
            session.is_active = should_be_active;
            session.is_muted = muted;
            session.is_deafened = deafened;
            session.is_afk = is_afk_channel;

            if (should_be_active) {
                session.last_active_start = now;
            }
        }
    } else {
        VoiceSession session;
        session.channel_id = state.channel_id;
        session.is_active = should_be_active;
        session.is_muted = muted;
        session.is_deafened = deafened;
        session.is_afk = is_afk_channel;
        session.last_active_start = now;
        session.accumulated_active_seconds = 0;

        sessions_[key] = session;
    }
}

void VoiceTracker::flush_all_active_sessions() {
    auto now = std::chrono::system_clock::now();
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& [key, session] : sessions_) {
        if (session.is_active) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - session.last_active_start).count();
            session.accumulated_active_seconds += elapsed;
            session.last_active_start = now;
        }

        if (session.accumulated_active_seconds > 0) {
            batcher_.add_voice_seconds(key.guild_id, key.user_id, session.accumulated_active_seconds);
            session.accumulated_active_seconds = 0;
        }
    }
}

