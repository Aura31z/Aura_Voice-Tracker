# Aura - Server Analytics & Voice Tracker 📊

A high-performance Discord bot built in C++ to track user engagement, log voice session durations, and generate dynamic leaderboards. Designed specifically to handle real-time activity for the PARADISE community without lag or high memory consumption.

## ✨ Features

*   **🎙️ Voice Activity Tracking:** Calculates exact minutes spent in voice channels. Automatically ignores AFK, muted, or deafened statuses to ensure accurate data.
*   **💬 Text Activity Tracking:** Logs message counts with built-in spam-prevention cooldowns.
*   **🏆 Dynamic Leaderboards:** Generates clean, visually appealing top 10 rankings for both text and voice activity via the `/leaderboard` command.
*   **⚡ High Performance:** Written in C++ using the D++ (DPP) library for instant response times and minimal RAM usage.
*   **💾 Persistent Storage:** Integrates securely with MongoDB to save user data across bot restarts.

## 🛠️ Tech Stack

*   **Language:** C++17 (or higher)
*   **Discord API:** [D++ (DPP)](https://dpp.dev/)
*   **Database:** MongoDB (using `mongocxx` driver)
*   **Build System:** CMake

## 📋 Prerequisites

Before compiling and running Aura, ensure you have the following installed on your system:

*   A modern C++ compiler (GCC, Clang, or MSVC)
*   [CMake](https://cmake.org/) (v3.15+)
*   [D++ Library](https://dpp.dev/) 
*   MongoDB C++ Driver (`mongocxx`)
*   A running MongoDB cluster (local or MongoDB Atlas)

## 🚀 Installation & Setup

**1. Clone the repository:**
```bash
git clone https://github.com/yourusername/aura-analytics-bot.git
cd aura-analytics-bot
```

**2. Configure your environment:**
Create a `.env` file or export the following variables in your environment:
```env
DISCORD_TOKEN=your_bot_token_here
MONGO_URI=mongodb+srv://username:password@cluster.mongodb.net/
```

**3. Build the project using CMake:**
```bash
mkdir build
cd build
cmake ..
make -j4
```

**4. Run the bot:**
```bash
./AuraBot
```

## 💻 Commands

| Command | Description | Options |
| :--- | :--- | :--- |
| `/leaderboard` | Displays the top 10 most active members. | `type`: Select either "Voice" or "Text" |
| `/ping` | Checks the bot's response time. | None |

## 🗄️ Database Schema

Aura utilizes a clean NoSQL structure for fast read/writes. Core user documents are structured as follows:

```json
{
  "user_id": "123456789012345678",
  "guild_id": "876543210987654321",
  "total_messages": 452,
  "total_voice_minutes": 1205,
  "last_active_timestamp": "2026-08-12T01:54:33Z"
}
```

## 📜 License

Distributed under the MIT License. See `LICENSE` for more information.

