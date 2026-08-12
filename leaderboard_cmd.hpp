#ifndef LEADERBOARD_CMD_HPP
#define LEADERBOARD_CMD_HPP

#include <dpp/dpp.h>
#include "db/db_manager.hpp"

class LeaderboardCommand {
public:
    explicit LeaderboardCommand(DbManager& db_manager);
    ~LeaderboardCommand() = default;

    // Register slash command definition with DPP
    static dpp::slashcommand get_command_definition(dpp::snowflake bot_id);

    // Handle slash command event
    void handle_command(const dpp::slashcommand_t& event, dpp::cluster& bot);

private:
    DbManager& db_manager_;

    std::string format_voice_time(int64_t total_minutes);
};

#endif // LEADERBOARD_CMD_HPP
