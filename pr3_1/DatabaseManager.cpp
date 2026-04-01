#include "DatabaseManager.h"
#include <iostream>

DatabaseManager::DatabaseManager() : hEnv(nullptr), hDbc(nullptr), hStmt(nullptr), connected(false) {
}

DatabaseManager::~DatabaseManager() {
    disconnect();
}

void DatabaseManager::showODBCError(SQLHANDLE handle, SQLSMALLINT type) {
    SQLCHAR sqlState[6];
    SQLCHAR message[256];
    SQLINTEGER nativeError;
    SQLSMALLINT textLength;

    if (SQLGetDiagRecA(type, handle, 1, sqlState, &nativeError, message, sizeof(message), &textLength) == SQL_SUCCESS) {
        std::cerr << "ODBC Error: " << message << " (SQL State: " << sqlState << ")" << std::endl;
    }
}

bool DatabaseManager::connect(const std::string& server, const std::string& database,
    const std::string& user, const std::string& password) {
    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) return false;

    SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    ret = SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) return false;

    std::string connStr;
    if (user.empty()) {
        connStr = "DRIVER={SQL Server};SERVER=" + server + ";DATABASE=" + database + ";Trusted_Connection=yes;";
    }
    else {
        connStr = "DRIVER={SQL Server};SERVER=" + server + ";DATABASE=" + database + ";UID=" + user + ";PWD=" + password + ";";
    }

    ret = SQLDriverConnectA(hDbc, NULL, (SQLCHAR*)connStr.c_str(), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);

    if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
        connected = true;
        SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
        logAction("Подключение к базе данных");
        return true;
    }
    return false;
}

void DatabaseManager::disconnect() {
    if (hStmt) { SQLFreeHandle(SQL_HANDLE_STMT, hStmt); hStmt = nullptr; }
    if (hDbc) { SQLDisconnect(hDbc); SQLFreeHandle(SQL_HANDLE_DBC, hDbc); hDbc = nullptr; }
    if (hEnv) { SQLFreeHandle(SQL_HANDLE_ENV, hEnv); hEnv = nullptr; }
    connected = false;
}

bool DatabaseManager::ping() {
    if (!connected) return false;
    SQLRETURN ret = SQLExecDirectA(hStmt, (SQLCHAR*)"SELECT 1", SQL_NTS);
    SQLFreeStmt(hStmt, SQL_CLOSE);
    return (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO);
}

void DatabaseManager::logAction(const std::string& description) {
    if (!connected) return;
    std::string sql = "INSERT INTO Logs (ActionDescription) VALUES (?)";
    SQLPrepareA(hStmt, (SQLCHAR*)sql.c_str(), SQL_NTS);
    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 500, 0,
        (SQLCHAR*)description.c_str(), 0, NULL);
    SQLExecute(hStmt);
    SQLFreeStmt(hStmt, SQL_CLOSE);
}

bool DatabaseManager::addResource(const Resource& res) {
    if (!connected) return false;

    SQLHSTMT hCheckStmt;
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hCheckStmt);
    std::string checkSql = "SELECT COUNT(*) FROM Resources WHERE Name = ? AND isDeleted = 0";
    SQLPrepareA(hCheckStmt, (SQLCHAR*)checkSql.c_str(), SQL_NTS);
    SQLBindParameter(hCheckStmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 255, 0,
        (SQLCHAR*)res.name.c_str(), 0, NULL);
    SQLExecute(hCheckStmt);

    SQLINTEGER count = 0;
    SQLLEN cbCount = 0;
    SQLBindCol(hCheckStmt, 1, SQL_C_LONG, &count, 0, &cbCount);
    SQLFetch(hCheckStmt);
    SQLFreeHandle(SQL_HANDLE_STMT, hCheckStmt);

    if (count > 0) {
        std::cout << "Ошибка: Файл с именем '" << res.name << "' уже существует!" << std::endl;
        return false;
    }

    std::string sql = "INSERT INTO Resources (Name, Size, CategoryID, OwnerID) VALUES (?, ?, ?, ?)";
    SQLPrepareA(hStmt, (SQLCHAR*)sql.c_str(), SQL_NTS);
    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 255, 0,
        (SQLCHAR*)res.name.c_str(), 0, NULL);
    SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, 0, 0,
        (SQLPOINTER)&res.size, 0, NULL);
    SQLBindParameter(hStmt, 3, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0,
        (SQLPOINTER)&res.categoryId, 0, NULL);
    SQLBindParameter(hStmt, 4, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0,
        (SQLPOINTER)&res.ownerId, 0, NULL);

    SQLRETURN ret = SQLExecute(hStmt);
    SQLFreeStmt(hStmt, SQL_CLOSE);

    if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
        logAction("Добавлен ресурс: " + res.name);
        return true;
    }
    return false;
}

bool DatabaseManager::updateResource(int id, const std::string& newName) {
    if (!connected) return false;
    std::string sql = "UPDATE Resources SET Name = ? WHERE ResourceID = ? AND isDeleted = 0";
    SQLPrepareA(hStmt, (SQLCHAR*)sql.c_str(), SQL_NTS);
    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 255, 0,
        (SQLCHAR*)newName.c_str(), 0, NULL);
    SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0,
        (SQLPOINTER)&id, 0, NULL);
    SQLRETURN ret = SQLExecute(hStmt);
    SQLFreeStmt(hStmt, SQL_CLOSE);
    if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
        logAction("Обновлен ресурс ID: " + std::to_string(id));
        return true;
    }
    return false;
}

bool DatabaseManager::softDeleteResource(int id) {
    if (!connected) return false;
    std::string sql = "UPDATE Resources SET isDeleted = 1 WHERE ResourceID = ?";
    SQLPrepareA(hStmt, (SQLCHAR*)sql.c_str(), SQL_NTS);
    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0,
        (SQLPOINTER)&id, 0, NULL);
    SQLRETURN ret = SQLExecute(hStmt);
    SQLFreeStmt(hStmt, SQL_CLOSE);
    if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
        logAction("Ресурс ID: " + std::to_string(id) + " перемещен в корзину");
        return true;
    }
    return false;
}

bool DatabaseManager::restoreResource(int id) {
    if (!connected) return false;
    std::string sql = "UPDATE Resources SET isDeleted = 0 WHERE ResourceID = ?";
    SQLPrepareA(hStmt, (SQLCHAR*)sql.c_str(), SQL_NTS);
    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0,
        (SQLPOINTER)&id, 0, NULL);
    SQLRETURN ret = SQLExecute(hStmt);
    SQLFreeStmt(hStmt, SQL_CLOSE);
    if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
        logAction("Ресурс ID: " + std::to_string(id) + " восстановлен");
        return true;
    }
    return false;
}

bool DatabaseManager::permanentDeleteResource(int id) {
    if (!connected) return false;
    std::string sql = "DELETE FROM Resources WHERE ResourceID = ?";
    SQLPrepareA(hStmt, (SQLCHAR*)sql.c_str(), SQL_NTS);
    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0,
        (SQLPOINTER)&id, 0, NULL);
    SQLRETURN ret = SQLExecute(hStmt);
    SQLFreeStmt(hStmt, SQL_CLOSE);
    if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
        logAction("Ресурс ID: " + std::to_string(id) + " полностью удален");
        return true;
    }
    return false;
}

std::vector<Resource> DatabaseManager::getResources(const std::string& searchMask,
    const std::string& orderBy,
    bool ascending,
    int offset, int limit) {
    std::vector<Resource> result;
    if (!connected) return result;

    std::string sql = "SELECT r.ResourceID, r.Name, r.Size, r.CategoryID, c.CategoryName, "
        "r.OwnerID, u.UserName, r.CreatedDate, r.isDeleted "
        "FROM Resources r "
        "JOIN Categories c ON r.CategoryID = c.CategoryID "
        "JOIN Users u ON r.OwnerID = u.UserID "
        "WHERE r.isDeleted = 0 ";

    if (!searchMask.empty()) sql += "AND r.Name LIKE ? ";
    sql += "ORDER BY " + orderBy + (ascending ? " ASC " : " DESC ");
    sql += "OFFSET ? ROWS FETCH NEXT ? ROWS ONLY";

    SQLPrepareA(hStmt, (SQLCHAR*)sql.c_str(), SQL_NTS);
    int paramIndex = 1;
    if (!searchMask.empty()) {
        std::string pattern = "%" + searchMask + "%";
        SQLBindParameter(hStmt, paramIndex++, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 255, 0,
            (SQLCHAR*)pattern.c_str(), 0, NULL);
    }
    SQLBindParameter(hStmt, paramIndex++, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0,
        (SQLPOINTER)&offset, 0, NULL);
    SQLBindParameter(hStmt, paramIndex++, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0,
        (SQLPOINTER)&limit, 0, NULL);

    SQLExecute(hStmt);

    Resource res;
    SQLCHAR name[256], categoryName[256], userName[256];
    SQL_TIMESTAMP_STRUCT createdDate;
    SQLLEN cbName, cbCatName, cbUserName, cbCreatedDate;

    SQLBindCol(hStmt, 1, SQL_C_LONG, &res.id, 0, NULL);
    SQLBindCol(hStmt, 2, SQL_C_CHAR, name, sizeof(name), &cbName);
    SQLBindCol(hStmt, 3, SQL_C_SBIGINT, &res.size, 0, NULL);
    SQLBindCol(hStmt, 4, SQL_C_LONG, &res.categoryId, 0, NULL);
    SQLBindCol(hStmt, 5, SQL_C_CHAR, categoryName, sizeof(categoryName), &cbCatName);
    SQLBindCol(hStmt, 6, SQL_C_LONG, &res.ownerId, 0, NULL);
    SQLBindCol(hStmt, 7, SQL_C_CHAR, userName, sizeof(userName), &cbUserName);
    SQLBindCol(hStmt, 8, SQL_C_TYPE_TIMESTAMP, &createdDate, sizeof(createdDate), &cbCreatedDate);
    SQLBindCol(hStmt, 9, SQL_C_BIT, &res.isDeleted, 0, NULL);

    while (SQLFetch(hStmt) == SQL_SUCCESS) {
        res.name = (char*)name;
        res.categoryName = (char*)categoryName;
        res.ownerName = (char*)userName;
        char dateBuffer[30];
        sprintf_s(dateBuffer, "%04d-%02d-%02d", createdDate.year, createdDate.month, createdDate.day);
        res.createdDate = dateBuffer;
        result.push_back(res);
    }

    SQLFreeStmt(hStmt, SQL_CLOSE);
    return result;
}

long long DatabaseManager::getTotalFileCount() {
    if (!connected) return 0;
    long long count = 0;
    SQLExecDirectA(hStmt, (SQLCHAR*)"SELECT COUNT(*) FROM Resources WHERE isDeleted = 0", SQL_NTS);
    SQLBindCol(hStmt, 1, SQL_C_SBIGINT, &count, 0, NULL);
    SQLFetch(hStmt);
    SQLFreeStmt(hStmt, SQL_CLOSE);
    return count;
}

long long DatabaseManager::getTotalFileSize() {
    if (!connected) return 0;
    long long totalSize = 0;
    SQLExecDirectA(hStmt, (SQLCHAR*)"SELECT ISNULL(SUM(Size), 0) FROM Resources WHERE isDeleted = 0", SQL_NTS);
    SQLBindCol(hStmt, 1, SQL_C_SBIGINT, &totalSize, 0, NULL);
    SQLFetch(hStmt);
    SQLFreeStmt(hStmt, SQL_CLOSE);
    return totalSize;
}

std::vector<Resource> DatabaseManager::getOldResources(int daysOld) {
    std::vector<Resource> result;
    if (!connected) return result;

    std::string sql = "SELECT r.ResourceID, r.Name, r.Size, r.CategoryID, c.CategoryName, "
        "r.OwnerID, u.UserName, r.CreatedDate, r.isDeleted "
        "FROM Resources r JOIN Categories c ON r.CategoryID = c.CategoryID "
        "JOIN Users u ON r.OwnerID = u.UserID "
        "WHERE r.isDeleted = 0 AND DATEDIFF(day, r.CreatedDate, GETDATE()) > ?";

    SQLPrepareA(hStmt, (SQLCHAR*)sql.c_str(), SQL_NTS);
    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, (SQLPOINTER)&daysOld, 0, NULL);
    SQLExecute(hStmt);

    Resource res;
    SQLCHAR name[256], categoryName[256], userName[256];
    SQL_TIMESTAMP_STRUCT createdDate;
    SQLLEN cbName, cbCatName, cbUserName, cbCreatedDate;

    SQLBindCol(hStmt, 1, SQL_C_LONG, &res.id, 0, NULL);
    SQLBindCol(hStmt, 2, SQL_C_CHAR, name, sizeof(name), &cbName);
    SQLBindCol(hStmt, 3, SQL_C_SBIGINT, &res.size, 0, NULL);
    SQLBindCol(hStmt, 4, SQL_C_LONG, &res.categoryId, 0, NULL);
    SQLBindCol(hStmt, 5, SQL_C_CHAR, categoryName, sizeof(categoryName), &cbCatName);
    SQLBindCol(hStmt, 6, SQL_C_LONG, &res.ownerId, 0, NULL);
    SQLBindCol(hStmt, 7, SQL_C_CHAR, userName, sizeof(userName), &cbUserName);
    SQLBindCol(hStmt, 8, SQL_C_TYPE_TIMESTAMP, &createdDate, sizeof(createdDate), &cbCreatedDate);
    SQLBindCol(hStmt, 9, SQL_C_BIT, &res.isDeleted, 0, NULL);

    while (SQLFetch(hStmt) == SQL_SUCCESS) {
        res.name = (char*)name;
        res.categoryName = (char*)categoryName;
        res.ownerName = (char*)userName;
        result.push_back(res);
    }

    SQLFreeStmt(hStmt, SQL_CLOSE);
    return result;
}

bool DatabaseManager::cleanOldResources(int daysOld) {
    if (!connected) return false;
    std::string sql = "DELETE FROM Resources WHERE isDeleted = 0 AND DATEDIFF(day, CreatedDate, GETDATE()) > ?";
    SQLPrepareA(hStmt, (SQLCHAR*)sql.c_str(), SQL_NTS);
    SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, (SQLPOINTER)&daysOld, 0, NULL);
    SQLRETURN ret = SQLExecute(hStmt);
    SQLFreeStmt(hStmt, SQL_CLOSE);
    if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
        logAction("Очистка старых данных (старше " + std::to_string(daysOld) + " дней)");
        return true;
    }
    return false;
}

std::vector<Category> DatabaseManager::getCategories() {
    std::vector<Category> result;
    if (!connected) return result;

    SQLExecDirectA(hStmt, (SQLCHAR*)"SELECT CategoryID, CategoryName FROM Categories", SQL_NTS);
    Category cat;
    SQLCHAR name[256];
    SQLLEN cbName;
    SQLBindCol(hStmt, 1, SQL_C_LONG, &cat.id, 0, NULL);
    SQLBindCol(hStmt, 2, SQL_C_CHAR, name, sizeof(name), &cbName);
    while (SQLFetch(hStmt) == SQL_SUCCESS) {
        cat.name = (char*)name;
        result.push_back(cat);
    }
    SQLFreeStmt(hStmt, SQL_CLOSE);
    return result;
}

std::vector<User> DatabaseManager::getUsers() {
    std::vector<User> result;
    if (!connected) return result;

    SQLExecDirectA(hStmt, (SQLCHAR*)"SELECT UserID, UserName FROM Users", SQL_NTS);
    User user;
    SQLCHAR name[256];
    SQLLEN cbName;
    SQLBindCol(hStmt, 1, SQL_C_LONG, &user.id, 0, NULL);
    SQLBindCol(hStmt, 2, SQL_C_CHAR, name, sizeof(name), &cbName);
    while (SQLFetch(hStmt) == SQL_SUCCESS) {
        user.name = (char*)name;
        result.push_back(user);
    }
    SQLFreeStmt(hStmt, SQL_CLOSE);
    return result;
}

bool DatabaseManager::changeDatabase(const std::string& newDatabase) {
    if (!connected) return false;
    std::string sql = "USE " + newDatabase;
    SQLRETURN ret = SQLExecDirectA(hStmt, (SQLCHAR*)sql.c_str(), SQL_NTS);
    SQLFreeStmt(hStmt, SQL_CLOSE);
    if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
        logAction("Смена базы данных на: " + newDatabase);
        return true;
    }
    return false;
}