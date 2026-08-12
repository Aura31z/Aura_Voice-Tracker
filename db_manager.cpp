#include "db/db_manager.hpp"
#include <iostream>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <mongocxx/exception/exception.hpp>

using bsoncxx::builder::basic::make_document;
using bsoncxx::builder::basic::kvp;

DbManager::DbManager(const std::string& uri_str, const std::string& db_name, const std::string& collection_name)
    : db_name_(db_name), collection_name_(collection_name) {
    
    // Initialize mongocxx driver instance once
    instance_ = std::make_unique<mongocxx::instance>();
    
    mongocxx::uri uri(uri_str);
    pool_ = std::make_unique<mongocxx::pool>(uri);
    
    std::cout << "[DbManager] Initialized MongoDB connection pool for " << uri_str << std::endl;
}

mongocxx::pool::entry DbManager::get_connection() {
    return pool_->acquire();
}

void DbManager::ensure_indexes() {
    try {
        auto client = get_connection();
        auto collection = (*client)[db_name_][collection_name_];

        // 1. Compound Unique Index on { guild_id: 1, user_id: 1 }
        {
            auto index_keys = make_document(kvp("guild_id", 1), kvp("user_id", 1));
            auto index_options = make_document(kvp("unique", true));
            collection.create_index(index_keys.view(), index_options.view());
        }

        // 2. Index for Text Leaderboard { guild_id: 1, total_messages: -1 }
        {
            auto index_keys = make_document(kvp("guild_id", 1), kvp("total_messages", -1));
            collection.create_index(index_keys.view());
        }

        // 3. Index for Voice Leaderboard { guild_id: 1, total_voice_minutes: -1 }
        {
            auto index_keys = make_document(kvp("guild_id", 1), kvp("total_voice_minutes", -1));
            collection.create_index(index_keys.view());
        }

        std::cout << "[DbManager] Ensured database indexes on " << db_name_ << "." << collection_name_ << std::endl;
    } catch (const mongocxx::exception& e) {
        std::cerr << "[DbManager] Error creating indexes: " << e.what() << std::endl;
    }
}

