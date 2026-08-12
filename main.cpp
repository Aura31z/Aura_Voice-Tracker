#include <dpp/dpp.h>
#include <iostream>
#include <csignal>
#include "config/bot_config.hpp"
#include "db/db_manager.hpp"
#include "db/activity_batcher.hpp"
#include "tracker/voice_tracker.hpp"
#include "tracker/text_tracker.hpp"
#include "commands/leaderboard_cmd.hpp"

static ActivityBatcher* g_batcher = nullptr;
static VoiceTracker* g_voice_tracker = nullptr;
static dpp::cluster* g_bot = nullptr;

void signal_handler(int signal) {
    std::cout << "\n[System] Signal " << signal << " received. Shutting down..." << std::endl;
    if (g_voice_tracker) g_voice_tracker->flush_all_active_sessions();
    if (g_batcher) g_batcher->stop();
    if (g_bot) g_bot->shutdown();
    exit(0);
}

int main() {
    BotConfig config = BotConfig::load("config.json");
    if (config.bot_token.empty()) {
        std::cerr << "[Error] Bot token missing! Provide it in config.json or DISCORD_TOKEN env." << std::endl;
        return 1;
    }

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    DbManager db_manager(config.mongodb_uri, config.database_name, config.collection_name);
    db_manager.ensure_indexes();

    ActivityBatcher batcher(db_manager, config.batch_flush_interval_seconds, config.batch_max_size);
    batcher.start();
    g_batcher = &batcher;

    VoiceTracker voice_tracker(batcher);
    g_voice_tracker = &voice_tracker;

    TextTracker text_tracker(batcher, config.text_cooldown_seconds);
    LeaderboardCommand leaderboard_command(db_manager);

    uint32_t intents = dpp::i_default_intents | dpp::i_message_content | dpp::i_guild_voice_states | dpp::i_guild_messages;
    dpp::cluster bot(config.bot_token, intents);
    g_bot = &bot;

    bot.on_log(dpp::utility::cout_logger());

    bot.on_ready([&bot](const dpp::ready_t&) {
        std::cout << "[Bot] Logged in as " << bot.me.username << " (" << bot.me.id << ")" << std::endl;
        if (dpp::run_once<struct register_bot_commands>()) {
            bot.global_bulk_command_create({ LeaderboardCommand::get_command_definition(bot.me.id) });
        }
    });

    bot.on_message_create([&text_tracker](const dpp::message_create_t& event) {
        text_tracker.handle_message_create(event);
    });

    bot.on_voice_state_update([&voice_tracker, &bot](const dpp::voice_state_update_t& event) {
        voice_tracker.handle_voice_state_update(event, bot);
    });

    bot.on_slashcommand([&leaderboard_command, &bot](const dpp::slashcommand_t& event) {
        if (event.command.get_command_name() == "leaderboard") {
            leaderboard_command.handle_command(event, bot);
        }
    });

    bot.start(dpp::st_wait);
    return 0;
}

