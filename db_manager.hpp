#ifndef DB_MANAGER_HPP
#define DB_MANAGER_HPP

#include <memory>
#include <string>
#include <mongocxx/instance.hpp>
#include <mongocxx/pool.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/client.hpp>

class DbManager {
public:
    DbManager(const std::string& uri_str, const std::string& db_name, const std::string& collection_name);
    ~DbManager() = default;

    // Retrieve a client connection from the pool
    mongocxx::pool::entry get_connection();

    std::string get_db_name() const { return db_name_; }
    std::string get_collection_name() const { return collection_name_; }

    void ensure_indexes();

private:
    std::unique_ptr<mongocxx::instance> instance_;
    std::unique_ptr<mongocxx::pool> pool_;
    std::string db_name_;
    std::string collection_name_;
};

#endif // DB_MANAGER_HPP
