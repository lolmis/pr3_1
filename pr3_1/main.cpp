#include <iostream>
#include <string>
#include <iomanip>
#include "DatabaseManager.h"
#include "UI.h"
#include "Utils.h"

int main() {
    setlocale(LC_ALL, "Russian");

    // Красивое приветствие
    std::cout << "\n===========================================" << std::endl;
    std::cout << "    МЕНЕДЖЕР ЦИФРОВЫХ АКТИВОВ" << std::endl;
    std::cout << "===========================================" << std::endl;

    DatabaseManager db;

    // Подключение к базе данных
    std::string server = "localhost\\SQLEXPRESS";
    std::string database = "ByteKeeperDB";

    std::cout << "\n[ИНФО] Подключение к базе данных..." << std::endl;
    std::cout << "        Сервер: " << server << std::endl;
    std::cout << "        База: " << database << std::endl;

    if (!db.connect(server, database)) {
        std::cerr << "\n[ОШИБКА] Не удалось подключиться к базе данных!" << std::endl;
        std::cerr << "Проверьте:" << std::endl;
        std::cerr << "  - Запущен ли SQL Server" << std::endl;
        std::cerr << "  - Правильное имя сервера" << std::endl;
        std::cerr << "  - Существование базы данных" << std::endl;
        return 1;
    }

    std::cout << "\n[УСПЕХ] Подключение к базе данных установлено!" << std::endl;

    // Проверка существования необходимых таблиц
    std::cout << "[ИНФО] Проверка структуры базы данных..." << std::endl;
    if (db.ping()) {
        std::cout << "[УСПЕХ] База данных готова к работе!" << std::endl;
    }
    else {
        std::cout << "[ПРЕДУПРЕЖДЕНИЕ] База данных может быть недоступна" << std::endl;
    }

    // Вывод статистики при запуске
    long long totalFiles = db.getTotalFileCount();
    long long totalSize = db.getTotalFileSize();
    std::cout << "\n[СТАТИСТИКА]" << std::endl;
    std::cout << "  Всего файлов: " << totalFiles << std::endl;
    std::cout << "  Общий размер: " << totalSize << " байт" << std::endl;
    std::cout << "  (" << formatSize(totalSize) << ")" << std::endl;

    std::cout << "\n===========================================" << std::endl;
    std::cout << "Система готова к работе!" << std::endl;
    std::cout << "===========================================" << std::endl;

    int choice;
    do {
        std::cout << "\n=== Менеджер цифровых активов ===" << std::endl;
        std::cout << "1. Показать все ресурсы (пагинация)" << std::endl;
        std::cout << "2. Добавить ресурс" << std::endl;
        std::cout << "3. Поиск по имени" << std::endl;
        std::cout << "4. Статистика" << std::endl;
        std::cout << "5. Корзина (восстановление)" << std::endl;
        std::cout << "6. Удалить ресурс (в корзину)" << std::endl;
        std::cout << "7. Очистка старых данных" << std::endl;
        std::cout << "8. Экспорт в CSV" << std::endl;
        std::cout << "9. Экспорт отчета в TXT" << std::endl;
        std::cout << "10. Сменить базу данных" << std::endl;
        std::cout << "11. Проверка соединения (пинг)" << std::endl;
        std::cout << "0. Выход" << std::endl;
        std::cout << "Выберите действие: ";
        std::cin >> choice;
        std::cin.ignore();

        switch (choice) {
        case 1: {
            // Пагинация
            int page = 1, pageSize = 10;
            char more;
            do {
                int offset = (page - 1) * pageSize;
                auto resources = db.getResources("", "ResourceID", true, offset, pageSize);
                printResources(resources);
                std::cout << "Страница " << page << ". Показать следующую? (y/n): ";
                std::cin >> more;
                if (more == 'y' || more == 'Y') page++;
                else break;
            } while (true);
            break;
        }
        case 2: {
            // Добавление ресурса с валидацией
            Resource newRes;
            std::cout << "Имя файла: ";
            std::getline(std::cin, newRes.name);

            // Валидация имени
            if (!isValidFilename(newRes.name)) {
                std::cout << "Ошибка: Имя содержит недопустимые символы!" << std::endl;
                break;
            }
            // Проверка расширения
            if (!isExtensionAllowed(newRes.name)) {
                std::cout << "Ошибка: Расширение файла запрещено!" << std::endl;
                break;
            }

            std::cout << "Размер (байт): ";
            std::string sizeStr;
            std::getline(std::cin, sizeStr);
            if (!isNumber(sizeStr)) {
                std::cout << "Ошибка: Размер должен быть числом!" << std::endl;
                break;
            }
            newRes.size = std::stoll(sizeStr);

            // Выбор категории
            auto categories = db.getCategories();
            if (categories.empty()) {
                std::cout << "Ошибка: Нет доступных категорий!" << std::endl;
                break;
            }
            std::cout << "Выберите категорию:\n";
            for (const auto& cat : categories) {
                std::cout << cat.id << ". " << cat.name << std::endl;
            }
            std::string catIdStr;
            std::getline(std::cin, catIdStr);
            if (!isNumber(catIdStr)) break;
            newRes.categoryId = std::stoi(catIdStr);

            // Выбор владельца
            auto users = db.getUsers();
            if (users.empty()) {
                std::cout << "Ошибка: Нет доступных пользователей!" << std::endl;
                break;
            }
            std::cout << "Выберите владельца:\n";
            for (const auto& user : users) {
                std::cout << user.id << ". " << user.name << std::endl;
            }
            std::string ownerIdStr;
            std::getline(std::cin, ownerIdStr);
            if (!isNumber(ownerIdStr)) break;
            newRes.ownerId = std::stoi(ownerIdStr);

            if (db.addResource(newRes)) {
                std::cout << "Ресурс успешно добавлен!" << std::endl;
            }
            break;
        }
        case 3: {
            // Поиск по имени
            std::string searchMask;
            std::cout << "Введите имя для поиска: ";
            std::getline(std::cin, searchMask);

            auto resources = db.getResources(searchMask, "Name", true, 0, 100);
            if (resources.empty()) {
                std::cout << "Ничего не найдено." << std::endl;
            }
            else {
                std::cout << "Найдено ресурсов: " << resources.size() << std::endl;
                printResources(resources);
            }
            break;
        }
        case 4: {
            // Статистика
            long long count = db.getTotalFileCount();
            long long size = db.getTotalFileSize();

            std::cout << "\n=== СТАТИСТИКА ХРАНИЛИЩА ===" << std::endl;
            std::cout << "Всего файлов: " << count << std::endl;
            std::cout << "Общий размер: " << size << " байт" << std::endl;
            std::cout << "Форматированный размер: " << formatSize(size) << std::endl;

            // Дополнительная статистика по категориям
            auto resources = db.getResources("", "ResourceID", true, 0, 1000);
            if (!resources.empty()) {
                std::cout << "\nСтатистика по категориям:" << std::endl;
                // Здесь можно добавить подсчет по категориям
            }
            break;
        }
        case 5: {
            // Корзина (восстановление)
            std::cout << "Введите ID ресурса для восстановления: ";
            int id;
            std::cin >> id;
            std::cin.ignore();

            if (db.restoreResource(id)) {
                std::cout << "Ресурс успешно восстановлен!" << std::endl;
            }
            else {
                std::cout << "Не удалось восстановить ресурс!" << std::endl;
            }
            break;
        }
        case 6: {
            // Удалить ресурс (в корзину)
            std::cout << "Введите ID ресурса для удаления: ";
            int id;
            std::cin >> id;
            std::cin.ignore();

            std::cout << "Вы уверены? (y/n): ";
            char confirm;
            std::cin >> confirm;
            std::cin.ignore();

            if (confirm == 'y' || confirm == 'Y') {
                if (db.softDeleteResource(id)) {
                    std::cout << "Ресурс перемещен в корзину!" << std::endl;
                }
                else {
                    std::cout << "Не удалось удалить ресурс!" << std::endl;
                }
            }
            break;
        }
        case 7: {
            // Очистка старых данных
            int days;
            std::cout << "Введите количество дней (старше этого срока будут удалены): ";
            std::cin >> days;
            std::cin.ignore();

            std::cout << "Вы уверены, что хотите удалить все ресурсы старше " << days << " дней? (y/n): ";
            char confirm;
            std::cin >> confirm;
            std::cin.ignore();

            if (confirm == 'y' || confirm == 'Y') {
                if (db.cleanOldResources(days)) {
                    std::cout << "Старые ресурсы успешно удалены!" << std::endl;
                }
                else {
                    std::cout << "Не удалось удалить старые ресурсы!" << std::endl;
                }
            }
            break;
        }
        case 8: {
            // Экспорт в CSV
            std::string filename;
            std::cout << "Введите имя файла для экспорта (например, resources.csv): ";
            std::getline(std::cin, filename);

            auto resources = db.getResources("", "ResourceID", true, 0, 10000);
            if (exportToCSV(resources, filename)) {
                std::cout << "Данные успешно экспортированы в " << filename << std::endl;
            }
            else {
                std::cout << "Ошибка при экспорте данных!" << std::endl;
            }
            break;
        }
        case 9: {
            // Экспорт отчета в TXT
            std::string filename;
            std::cout << "Введите имя файла для отчета (например, report.txt): ";
            std::getline(std::cin, filename);

            auto resources = db.getResources("", "ResourceID", true, 0, 10000);
            if (exportToTXT(resources, filename, db.getTotalFileCount(), db.getTotalFileSize())) {
                std::cout << "Отчет успешно сохранен в " << filename << std::endl;
            }
            else {
                std::cout << "Ошибка при создании отчета!" << std::endl;
            }
            break;
        }
        case 10: {
            // Сменить базу данных
            std::string newDatabase;
            std::cout << "Введите имя новой базы данных: ";
            std::getline(std::cin, newDatabase);

            if (db.changeDatabase(newDatabase)) {
                std::cout << "База данных изменена на: " << newDatabase << std::endl;
            }
            else {
                std::cout << "Не удалось изменить базу данных!" << std::endl;
            }
            break;
        }
        case 11: {
            // Проверка соединения
            if (db.ping()) {
                std::cout << "Соединение с базой данных активно!" << std::endl;
                std::cout << "Статус: OK" << std::endl;
            }
            else {
                std::cout << "Соединение потеряно!" << std::endl;
            }
            break;
        }
        case 0:
            std::cout << "\nВыход из программы..." << std::endl;
            std::cout << "До свидания!" << std::endl;
            break;
        default:
            std::cout << "Неверный выбор! Пожалуйста, выберите пункт от 0 до 11." << std::endl;
        }
    } while (choice != 0);

    return 0;
}