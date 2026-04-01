#pragma once
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <windows.h>
#include "Models.h"
#include "Utils.h"

inline void setColor(int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

inline void resetColor() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, 7);
}

inline void printResources(const std::vector<Resource>& resources) {
    if (resources.empty()) {
        setColor(12);
        std::cout << "Нет данных для отображения." << std::endl;
        resetColor();
        return;
    }

    std::cout << std::left
        << std::setw(5) << "ID"
        << std::setw(25) << "Имя"
        << std::setw(12) << "Размер"
        << std::setw(15) << "Категория"
        << std::setw(15) << "Владелец"
        << std::setw(20) << "Дата создания" << std::endl;
    std::cout << std::string(92, '-') << std::endl;

    for (const auto& res : resources) {
        if (res.categoryName == "Архивы") {
            setColor(14);
        }
        std::cout << std::left
            << std::setw(5) << res.id
            << std::setw(25) << truncate(res.name, 22)
            << std::setw(12) << res.size
            << std::setw(15) << truncate(res.categoryName, 13)
            << std::setw(15) << truncate(res.ownerName, 13)
            << std::setw(20) << res.createdDate << std::endl;
        resetColor();
    }
    std::cout << std::endl;
}