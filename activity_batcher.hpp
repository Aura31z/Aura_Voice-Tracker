#ifndef ACTIVITY_BATCHER_HPP
#define ACTIVITY_BATCHER_HPP

#include <string>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include "db/db_manager.hpp"

struct UserKey {
    std::string guild_id;
    std::string user_id;

    bool operator==(const UserKey& other) const {
        return guild_id == other.guild_id && user_id == other.user_id;
    }
};

namespace std {
    template<>
    struct hash<UserKey> {
        size_t operator()(const UserKey& k) const {
            return hash<string>()(k.guild_id) ^ (hash<string>()(k.user_id) << 1);
        }
    };
}

struct UserDelta {
    int64_t message_delta = 0;
    int64_t voice_seconds_delta = 0;
    std::chrono::system_clock::time_point last_active;
};

class ActivityBatcher {
public:
    ActivityBatcher(DbManager& db_manager, int flush_interval_sec = 5, int max_batch_size = 50);
    ~ActivityBatcher();

    void start();
    void stop();

    void add_messages(const std::string& guild_id, const std::string& user_id, int count = 1);
    void add_voice_seconds(const std::string& guild_id, const std::string& user_id, int64_t seconds);

    void flush();

private:
    DbManager& db_manager_;
    int flush_interval_sec_;
    int max_batch_size_;

    std::unordered_map<UserKey, UserDelta> pending_updates_;
    std::unordered_map<UserKey, int64_t> voice_seconds_remainder_; // Carryover sub-minute seconds
    std::mutex mutex_;

    std::thread worker_thread_;
    std::atomic<bool> running_{false};
};

#endif // ACTIVITY_BATCHER_HPP
