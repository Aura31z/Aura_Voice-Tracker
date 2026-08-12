#include "commands/leaderboard_cmd.hpp"
#include <iostream>
#include <sstream>
#include <thread>
#include <iomanip>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <mongocxx/options/find.hpp>
#include <mongocxx/exception/exception.hpp>

using bsoncxx::builder::basic::make_document;
using bsoncxx::builder::basic::kvp;

LeaderboardCommand::LeaderboardCommand(DbManager& db_manager)
    : db_manager_(db_manager) {}

dpp::slashcommand LeaderboardCommand::get_command_definition(dpp::snowflake bot_id) {
    dpp::slashcommand cmd("leaderboard", "View server activity leaderboards for PARADISE", bot_id);

    dpp::command_option type_option(dpp::co_string, "type", "Select leaderboard category", true);
    type_option.add_choice(dpp::command_option_choice("Text Activity (Most Messages)", std::string("text")));
    type_option.add_choice(dpp::command_option_choice("Voice Activity (Most Time)", std::string("voice")));

    cmd.add_option(type_option);
    return cmd;
}

std::string LeaderboardCommand::format_voice_time(int64_t total_minutes) {
    if (total_minutes < 60) {
        return std::to_string(total_minutes) + "m";
    }
    int64_t hours = total_minutes / 60;
    int64_t mins = total_minutes % 60;
    return std::to_string(hours) + "h " + std::to_string(mins) + "m";
}

static int64_t extract_number(const bsoncxx::document::element& elem) {
    if (!elem) return 0;
    if (elem.type() == bsoncxx::type::k_int32) return elem.get_int32().value;
    if (elem.type() == bsoncxx::type::k_int64) return elem.get_int64().value;
    if (elem.type() == bsoncxx::type::k_double) return static_cast<int64_t>(elem.get_double().value);
    return 0;
}

static std::string get_rank_badge(int rank) {
    switch (rank) {
        case 1: return "🥇";
        case 2: return "🥈";
        case 3: return "🥉";
        case 4: return "4️⃣";
        case 5: return "5️⃣";
        case 6: return "6️⃣";
        case 7: return "7️⃣";
        case 8: return "8️⃣";
        case 9: return "9️⃣";
        case 10: return "🔟";
        default: return "`#" + std::to_string(rank) + "`";
    }
}

void LeaderboardCommand::handle_command(const dpp::slashcommand_t& event, dpp::cluster& /*bot*/) {
    // Defer response to allow asynchronous DB query
    event.thinking();

    std::string type = "text";
    auto param = event.get_parameter("type");
    if (std::holds_alternative<std::string>(param)) {
        type = std::get<std::string>(param);
    }

    std::string guild_id_str = std::to_string(event.command.guild_id);

    // Run MongoDB query on a separate thread to prevent blocking DPP event loop
    std::thread query_thread([this, event, type, guild_id_str]() {
        try {
            auto client = db_manager_.get_connection();
            auto collection = (*client)[db_manager_.get_db_name()][db_manager_.get_collection_name()];

            bsoncxx::builder::basic::document filter_builder{};
            filter_builder.append(kvp("guild_id", guild_id_str));

            bsoncxx::builder::basic::document sort_builder{};

            if (type == "text") {
                sort_builder.append(kvp("total_messages", -1));
                filter_builder.append(kvp("total_messages", make_document(kvp("$gt", 0))));
            } else {
                sort_builder.append(kvp("total_voice_minutes", -1));
                filter_builder.append(kvp("total_voice_minutes", make_document(kvp("$gt", 0))));
            }

            auto filter_doc = filter_builder.extract();
            auto sort_doc = sort_builder.extract();

            mongocxx::options::find options{};
            options.sort(sort_doc.view());
            options.limit(10);

            auto cursor = collection.find(filter_doc.view(), options);

            dpp::embed embed = dpp::embed()
                .set_color(type == "text" ? 0x5865F2 : 0xF1C40F) // Blurple for Text, Gold for Voice
                .set_author("PARADISE Analytics Engine", "", "https://cdn.discordapp.com/embed/avatars/0.png")
                .set_footer(dpp::embed_footer().set_text("PARADISE Activity Tracker • Live Updates"));

            std::stringstream description;

            if (type == "text") {
                embed.set_title("💬 Top Text Activity Leaderboard");
                description << "Here are the top text contributors in **PARADISE**:\n\n";
            } else {
                embed.set_title("🎙️ Top Voice Activity Leaderboard");
                description << "Here are the top voice participants in **PARADISE**:\n\n";
            }

            int rank = 1;
            bool found_any = false;

            for (auto&& doc : cursor) {
                found_any = true;
                std::string user_id = std::string(doc["user_id"].get_string().value);
                
                std::string rank_badge = get_rank_badge(rank);

                if (type == "text") {
                    int64_t msg_count = extract_number(doc["total_messages"]);
                    description << rank_badge << " <@" << user_id << "> — **" 
                                << msg_count << "** messages\n";
                } else {
                    int64_t voice_mins = extract_number(doc["total_voice_minutes"]);
                    std::string formatted_time = format_voice_time(voice_mins);
                    description << rank_badge << " <@" << user_id << "> — **" 
                                << formatted_time << "** (" << voice_mins << " mins)\n";
                }
                rank++;
            }

            if (!found_any) {
                description << "*No activity recorded yet for this server. Start chatting or join voice channels!*";
            }

            embed.set_description(description.str());
            embed.set_timestamp(time(nullptr));

            dpp::message msg(event.command.channel_id, embed);
            event.edit_original_response(msg);

        } catch (const mongocxx::exception& e) {
            std::cerr << "[LeaderboardCommand] DB Error: " << e.what() << std::endl;
            dpp::message error_msg("❌ An error occurred while fetching the leaderboard.");
            event.edit_original_response(error_msg);
        } catch (const std::exception& e) {
            std::cerr << "[LeaderboardCommand] Error: " << e.what() << std::endl;
            dpp::message error_msg("❌ An internal error occurred.");
            event.edit_original_response(error_msg);
        }
    });

    query_thread.detach();
}

