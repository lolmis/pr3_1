// utils.h
#pragma once
#include <string>
#include <regex>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cstdio>

const std::vector<std::string> allowedExtensions = {
    ".exe", ".txt", ".pdf", ".docx", ".xlsx", ".jpg", ".png", ".cpp", ".h"
};

// Объявления функций (forward declarations)
inline std::string formatSize(long long bytes);
inline bool exportToCSV(const std::vector<Resource>& resources, const std::string& filename);
inline bool exportToTXT(const std::vector<Resource>& resources, const std::string& filename,
    long long totalCount, long long totalSize);
inline std::string truncate(const std::string& str, size_t maxLen = 15);

// Реализации функций
inline bool isExtensionAllowed(const std::string& filename) {
    size_t dotPos = filename.find_last_of('.');
    if (dotPos == std::string::npos) return false;
    std::string ext = filename.substr(dotPos);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    for (const auto& allowed : allowedExtensions) {
        if (ext == allowed) return true;
    }
    return false;
}

inline bool isValidFilename(const std::string& name) {
    if (name.empty()) return false;
    std::regex invalidChars(R"([/\\:*?\"<>|])");
    return !std::regex_search(name, invalidChars);
}

inline std::string truncate(const std::string& str, size_t maxLen) {
    if (str.length() <= maxLen) return str;
    return str.substr(0, maxLen - 3) + "...";
}

inline bool isNumber(const std::string& str) {
    if (str.empty()) return false;
    for (char c : str) {
        if (!std::isdigit(c)) return false;
    }
    return true;
}

inline std::string formatSize(long long bytes) {
    const char* units[] = { "B", "KB", "MB", "GB", "TB" };
    int unitIndex = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024 && unitIndex < 4) {
        size /= 1024;
        unitIndex++;
    }

    char buffer[50];
    sprintf_s(buffer, sizeof(buffer), "%.2f %s", size, units[unitIndex]);
    return std::string(buffer);
}

inline bool exportToCSV(const std::vector<Resource>& resources, const std::string& filename) {
    FILE* file = nullptr;
    fopen_s(&file, filename.c_str(), "w");
    if (!file) return false;

    // Заголовки
    fprintf(file, "ID,Name,Size,CategoryID,CategoryName,OwnerID,OwnerName,CreatedDate,isDeleted\n");

    // Данные
    for (const auto& res : resources) {
        fprintf(file, "%d,\"%s\",%lld,%d,\"%s\",%d,\"%s\",\"%s\",%d\n",
            res.id,
            res.name.c_str(),
            res.size,
            res.categoryId,
            res.categoryName.c_str(),
            res.ownerId,
            res.ownerName.c_str(),
            res.createdDate.c_str(),
            res.isDeleted ? 1 : 0);
    }

    fclose(file);
    return true;
}

inline bool exportToTXT(const std::vector<Resource>& resources, const std::string& filename,
    long long totalCount, long long totalSize) {
    FILE* file = nullptr;
    fopen_s(&file, filename.c_str(), "w");
    if (!file) return false;

    // Заголовок отчета
    fprintf(file, "===========================================\n");
    fprintf(file, "ОТЧЕТ ПО ЦИФРОВЫМ АКТИВАМ\n");
    fprintf(file, "Дата: %s\n", __DATE__);
    fprintf(file, "===========================================\n\n");

    fprintf(file, "ОБЩАЯ СТАТИСТИКА:\n");
    fprintf(file, "  Всего ресурсов: %lld\n", totalCount);
    fprintf(file, "  Общий размер: %lld байт (%s)\n\n", totalSize, formatSize(totalSize).c_str());

    fprintf(file, "ДЕТАЛЬНЫЙ СПИСОК РЕСУРСОВ:\n");
    fprintf(file, "-------------------------------------------------------------------------------\n");
    fprintf(file, "%-5s | %-30s | %-12s | %-15s | %-15s | %-15s\n",
        "ID", "Имя", "Размер", "Категория", "Владелец", "Дата создания");
    fprintf(file, "-------------------------------------------------------------------------------\n");

    for (const auto& res : resources) {
        fprintf(file, "%-5d | %-30s | %-12lld | %-15s | %-15s | %-15s\n",
            res.id,
            truncate(res.name, 28).c_str(),
            res.size,
            truncate(res.categoryName, 13).c_str(),
            truncate(res.ownerName, 13).c_str(),
            res.createdDate.c_str());
    }

    fprintf(file, "-------------------------------------------------------------------------------\n");
    fprintf(file, "Всего записей: %zu\n", resources.size());

    fclose(file);
    return true;
}