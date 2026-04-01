#pragma once
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <string>
#include <vector>
#include "Models.h"

class DatabaseManager {
private:
    SQLHENV hEnv;
    SQLHDBC hDbc;
    SQLHSTMT hStmt;
    bool connected;

    void showODBCError(SQLHANDLE handle, SQLSMALLINT type);

public:
    DatabaseManager();
    ~DatabaseManager();

    bool connect(const std::string& server, const std::string& database,
        const std::string& user = "", const std::string& password = "");
    void disconnect();
    bool ping();
    void logAction(const std::string& description);

    bool addResource(const Resource& res);
    bool updateResource(int id, const std::string& newName);
    bool softDeleteResource(int id);
    bool restoreResource(int id);
    bool permanentDeleteResource(int id);

    std::vector<Resource> getResources(const std::string& searchMask = "",
        const std::string& orderBy = "ResourceID",
        bool ascending = true,
        int offset = 0, int limit = 10);

    long long getTotalFileCount();
    long long getTotalFileSize();

    std::vector<Resource> getOldResources(int daysOld = 30);
    bool cleanOldResources(int daysOld = 30);

    std::vector<Category> getCategories();
    std::vector<User> getUsers();

    bool changeDatabase(const std::string& newDatabase);
};