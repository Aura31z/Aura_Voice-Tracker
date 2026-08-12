#include "db/activity_batcher.hpp"
#include <iostream>
#include <vector>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <mongocxx/model/update_one.hpp>
#include <mongocxx/exception/exception.hpp>

using bsoncxx::builder::basic::make_document;
using bsoncxx::builder::basic::kvp;

ActivityBatcher::ActivityBatcher(DbManager& db_manager, int flush_interval_sec, int max_batch_size)
    : db_manager_(db_manager), flush_interval_sec_(flush_interval_sec), max_batch_size_(max_batch_size) {}

ActivityBatcher::~ActivityBatcher() {
    stop();
}

void ActivityBatcher::start() {
    if (running_) return;
    running_ = true;
    worker_thread_ = std::thread([this]() {
        while (running_) {
            std::this_thread::sleep_for(std::chrono::seconds(flush_interval_sec_));
            flush();
        }
    });
}

void ActivityBatcher::stop() {
    if (!running_) return;
    running_ = false;
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
    flush();
}

void ActivityBatcher::add_messages(const std::string& guild_id, const std::string& user_id, int count) {
    std::lock_guard<std::mutex> lock(mutex_);
    UserKey key{guild_id, user_id};
    auto& delta = pending_updates_[key];
    delta.message_delta += count;
    delta.last_active = std::chrono::system_clock::now();
}

void ActivityBatcher::add_voice_seconds(const std::string& guild_id, const std::string& user_id, int64_t seconds) {
    if (seconds <= 0) return;
    std::lock_guard<std::mutex> lock(mutex_);
    UserKey key{guild_id, user_id};
    auto& delta = pending_updates_[key];
    delta.voice_seconds_delta += seconds;
    delta.last_active = std::chrono::system_clock::now();
}

void ActivityBatcher::flush() {
    std::unordered_map<UserKey, UserDelta> updates_to_process;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (pending_updates_.empty()) return;
        updates_to_process.swap(pending_updates_);
    }

    try {
        auto client = db_manager_.get_connection();
        auto collection = (*client)[db_manager_.get_db_name()][db_manager_.get_collection_name()];

        mongocxx::bulk_write bulk_op{mongocxx::options::bulk_write{}.ordered(false)};
        int ops_count = 0;

        for (const auto& [key, delta] : updates_to_process) {
            int64_t total_seconds = delta.voice_seconds_delta;
            
            auto rem_it = voice_seconds_remainder_.find(key);
            if (rem_it != voice_seconds_remainder_.end()) {
                total_seconds += rem_it->second;
            }

            int64_t minutes = total_seconds / 60;
            int64_t new_remainder = total_seconds % 60;

            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (new_remainder > 0) {
                    voice_seconds_remainder_[key] = new_remainder;
                } else {
                    voice_seconds_remainder_.erase(key);
                }
            }

            if (delta.message_delta == 0 && minutes == 0) {
                continue;
            }

            auto filter_doc = make_document(
                kvp("guild_id", key.guild_id),
                kvp("user_id", key.user_id)
            );

            bsoncxx::builder::basic::document update_builder{};

            if (delta.message_delta > 0 || minutes > 0) {
                bsoncxx::builder::basic::document inc_doc{};
                if (delta.message_delta > 0) {
                    inc_doc.append(kvp("total_messages", delta.message_delta));
                }
                if (minutes > 0) {
                    inc_doc.append(kvp("total_voice_minutes", minutes));
                }
                update_builder.append(kvp("$inc", inc_doc.extract()));
            }

            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                delta.last_active.time_since_epoch()
            );
            update_builder.append(kvp("$max", make_document(
                kvp("last_active_timestamp", bsoncxx::types::b_date{ms})
            )));

            auto update_doc = update_builder.extract();

            mongocxx::model::update_one update_model{filter_doc.view(), update_doc.view()};
            update_model.upsert(true);

            bulk_op.append(update_model);
            ops_count++;
        }

        if (ops_count > 0) {
            collection.bulk_write(bulk_op);
        }
    } catch (const std::exception& e) {
        std::cerr << "[ActivityBatcher] Flush error: " << e.what() << std::endl;
    }
}


