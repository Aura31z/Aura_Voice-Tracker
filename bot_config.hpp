#ifndef BOT_CONFIG_HPP
#define BOT_CONFIG_HPP

#include <string>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <nlohmann/json.hpp>

struct BotConfig {
    std::string bot_token;
    std::string mongodb_uri = "mongodb://localhost:27017";
    std::string database_name = "paradise_db";
    std::string collection_name = "user_analytics";
    int text_cooldown_seconds = 15;
    int batch_flush_interval_seconds = 5;
    int batch_max_size = 50;

    static BotConfig load(const std::string& config_file_path = "config.json") {
        BotConfig cfg;
        std::ifstream file(config_file_path);
        
        if (file.is_open()) {
            try {
                nlohmann::json j;
                file >> j;
                if (j.contains("bot_token") && !j["bot_token"].get<std::string>().empty()) {
                    cfg.bot_token = j["bot_token"].get<std::string>();
                }
                if (j.contains("mongodb_uri")) cfg.mongodb_uri = j["mongodb_uri"].get<std::string>();
                if (j.contains("database_name")) cfg.database_name = j["database_name"].get<std::string>();
                if (j.contains("collection_name")) cfg.collection_name = j["collection_name"].get<std::string>();
                if (j.contains("text_cooldown_seconds")) cfg.text_cooldown_seconds = j["text_cooldown_seconds"].get<int>();
                if (j.contains("batch_flush_interval_seconds")) cfg.batch_flush_interval_seconds = j["batch_flush_interval_seconds"].get<int>();
                if (j.contains("batch_max_size")) cfg.batch_max_size = j["batch_max_size"].get<int>();
                std::cout << "[Config] Loaded configuration from " << config_file_path << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "[Config] Error parsing " << config_file_path << ": " << e.what() << std::endl;
            }
        } else {
            std::cout << "[Config] " << config_file_path << " not found, checking environment variables..." << std::endl;
        }

        // Fallback to Environment Variables if missing
        if (cfg.bot_token.empty()) {
            const char* env_token = std::getenv("DISCORD_TOKEN");
            if (env_token) cfg.bot_token = env_token;
        }
        const char* env_mongo = std::getenv("MONGODB_URI");
        if (env_mongo) cfg.mongodb_uri = env_mongo;

        const char* env_dbname = std::getenv("MONGODB_DATABASE");
        if (env_dbname) cfg.database_name = env_dbname;

        return cfg;
    }
};

#endif // BOT_CONFIG_HPP
