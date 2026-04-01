#pragma once
#include <string>

struct Category {
    int id;
    std::string name;

    Category() : id(0), name("") {}
    Category(int _id, const std::string& _name) : id(_id), name(_name) {}
};

struct User {
    int id;
    std::string name;

    User() : id(0), name("") {}
    User(int _id, const std::string& _name) : id(_id), name(_name) {}
};

struct Resource {
    int id;
    std::string name;
    long long size;
    int categoryId;
    std::string categoryName;
    int ownerId;
    std::string ownerName;
    std::string createdDate;
    bool isDeleted;

    Resource() : id(0), size(0), categoryId(0), ownerId(0), isDeleted(false) {}
};