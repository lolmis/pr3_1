-- Создание базы данных
CREATE DATABASE ByteKeeperDB;
GO

USE ByteKeeperDB;
GO

-- Таблица категорий
CREATE TABLE Categories (
    CategoryID INT IDENTITY(1,1) PRIMARY KEY,
    CategoryName NVARCHAR(100) NOT NULL UNIQUE
);

-- Таблица пользователей
CREATE TABLE Users (
    UserID INT IDENTITY(1,1) PRIMARY KEY,
    UserName NVARCHAR(100) NOT NULL UNIQUE
);

-- Таблица ресурсов (с поддержкой Soft Delete)
CREATE TABLE Resources (
    ResourceID INT IDENTITY(1,1) PRIMARY KEY,
    Name NVARCHAR(255) NOT NULL,
    Size BIGINT NOT NULL,
    CategoryID INT NOT NULL,
    OwnerID INT NOT NULL,
    CreatedDate DATETIME DEFAULT GETDATE(),
    isDeleted BIT DEFAULT 0,
    FOREIGN KEY (CategoryID) REFERENCES Categories(CategoryID),
    FOREIGN KEY (OwnerID) REFERENCES Users(UserID)
);

-- Таблица для логирования действий
CREATE TABLE Logs (
    LogID INT IDENTITY(1,1) PRIMARY KEY,
    ActionDescription NVARCHAR(500) NOT NULL,
    ActionDate DATETIME DEFAULT GETDATE()
);

-- Добавление начальных данных (опционально)
INSERT INTO Categories (CategoryName) VALUES ('Документы'), ('Медиа'), ('Архивы');
INSERT INTO Users (UserName) VALUES ('Иванов И.И.'), ('Петров П.П.');
GO