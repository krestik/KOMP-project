#include "booking_manager.h"
#include "workstation.h"
#include "booking.h"
#include <sqlite3.h>
#include <stdexcept>
#include <sstream>
#include <vector>
#include <string>
#include <iostream>

using namespace std;

BookingManager::BookingManager() : db(nullptr) {
    int rc = sqlite3_open("booking.db", &db);
    if (rc) {
        string errMsgStr = db ? sqlite3_errmsg(db) : "не удалось получить сообщение об ошибке sqlite";
        if (db) sqlite3_close(db);
        db = nullptr;
        throw runtime_error("Не удалось открыть базу данных: " + errMsgStr);
    }
    try {
        initializeDatabase();
    } catch (...) {
        sqlite3_close(db);
        db = nullptr;
        throw;
    }
}

BookingManager::~BookingManager() {
    if (db) {
        sqlite3_close(db);
    }
}

void BookingManager::initializeDatabase() {
    const char* sql1 = "CREATE TABLE IF NOT EXISTS Workstations (id INTEGER PRIMARY KEY, name TEXT, status TEXT);";
    const char* sql2 = "CREATE TABLE IF NOT EXISTS Bookings (bookingId INTEGER PRIMARY KEY, workstationId INTEGER, clientName TEXT, bookingDate TEXT, startHour INTEGER, startMinute INTEGER, endHour INTEGER, endMinute INTEGER);";
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql1, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        string err = errMsg ? errMsg : "неизвестная ошибка sqlite";
        sqlite3_free(errMsg);
        throw runtime_error("Ошибка SQL при создании таблицы Workstations: " + err);
    }
    rc = sqlite3_exec(db, sql2, 0, 0, &errMsg);
    if (rc != SQLITE_OK) {
        string err = errMsg ? errMsg : "неизвестная ошибка sqlite";
        sqlite3_free(errMsg);
        throw runtime_error("Ошибка SQL при создании таблицы Bookings: " + err);
    }
}

vector<Workstation> BookingManager::loadWorkstations() {
    vector<Workstation> result;
    const char* sql = "SELECT id, name, status FROM Workstations;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        string errMsgStr = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        throw runtime_error("Ошибка подготовки запроса для загрузки рабочих станций: " + errMsgStr);
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        const unsigned char* nameText = sqlite3_column_text(stmt, 1);
        const unsigned char* statusText = sqlite3_column_text(stmt, 2);
        string name = nameText ? reinterpret_cast<const char*>(nameText) : "";
        string status = statusText ? reinterpret_cast<const char*>(statusText) : "";
        result.emplace_back(id, name, status);
    }
    sqlite3_finalize(stmt);
    return result;
}

vector<Booking> BookingManager::loadBookings() {
    vector<Booking> result;
    const char* sql = "SELECT bookingId, workstationId, clientName, bookingDate, startHour, startMinute, endHour, endMinute FROM Bookings;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        string errMsgStr = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        throw runtime_error("Ошибка подготовки запроса для загрузки бронирований: " + errMsgStr);
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int bookingId = sqlite3_column_int(stmt, 0);
        int workstationId = sqlite3_column_int(stmt, 1);
        const unsigned char* clientText = sqlite3_column_text(stmt, 2);
        const unsigned char* dateText = sqlite3_column_text(stmt, 3);
        int startHour = sqlite3_column_int(stmt, 4);
        int startMinute = sqlite3_column_int(stmt, 5);
        int endHour = sqlite3_column_int(stmt, 6);
        int endMinute = sqlite3_column_int(stmt, 7);
        string clientName = clientText ? reinterpret_cast<const char*>(clientText) : "";
        string bookingDate = dateText ? reinterpret_cast<const char*>(dateText) : "";
        Time start = { startHour, startMinute };
        Time end = { endHour, endMinute };
        result.emplace_back(bookingId, workstationId, clientName, bookingDate, start, end);
    }
    sqlite3_finalize(stmt);
    return result;
}

void BookingManager::addWorkstation(const Workstation& ws) {
    const char* sql = "INSERT INTO Workstations (id, name, status) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        string errMsgStr = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        throw runtime_error("Ошибка подготовки запроса для добавления станции: " + errMsgStr);
    }
    sqlite3_bind_int(stmt, 1, ws.getId());
    sqlite3_bind_text(stmt, 2, ws.getName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, "available", -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        string errMsgStr = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        throw runtime_error("Ошибка выполнения запроса для добавления станции: " + errMsgStr);
    }
    sqlite3_finalize(stmt);
}

void BookingManager::deleteWorkstation(int id) {
    const char* sql = "DELETE FROM Workstations WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
         string errMsgStr = sqlite3_errmsg(db);
         sqlite3_finalize(stmt);
         throw runtime_error("Ошибка подготовки запроса для удаления станции: " + errMsgStr);
    }
    sqlite3_bind_int(stmt, 1, id);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        string errMsgStr = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        throw runtime_error("Ошибка выполнения запроса для удаления станции: " + errMsgStr);
    }
    sqlite3_finalize(stmt);
}

void BookingManager::updateWorkstationStatus(int id, const string& newStatus) {
    const char* sql = "UPDATE Workstations SET status = ? WHERE id = ?;";
     sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
         string errMsgStr = sqlite3_errmsg(db);
         sqlite3_finalize(stmt);
         throw runtime_error("Ошибка подготовки запроса для обновления статуса станции: " + errMsgStr);
    }
    sqlite3_bind_text(stmt, 1, newStatus.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, id);
     if (sqlite3_step(stmt) != SQLITE_DONE) {
        string errMsgStr = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        throw runtime_error("Ошибка выполнения запроса для обновления статуса станции: " + errMsgStr);
    }
    sqlite3_finalize(stmt);
}

void BookingManager::addBooking(const Booking& b) {
    const char* sql = "INSERT INTO Bookings (bookingId, workstationId, clientName, bookingDate, startHour, startMinute, endHour, endMinute) VALUES (?, ?, ?, ?, ?, ?, ?, ?);";
     sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
         string errMsgStr = sqlite3_errmsg(db);
         sqlite3_finalize(stmt);
         throw runtime_error("Ошибка подготовки запроса для добавления брони: " + errMsgStr);
    }
    sqlite3_bind_int(stmt, 1, b.getBookingId());
    sqlite3_bind_int(stmt, 2, b.getWorkstationId());
    sqlite3_bind_text(stmt, 3, b.getClientName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, b.getBookingDate().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, b.getStartTime().hour);
    sqlite3_bind_int(stmt, 6, b.getStartTime().minute);
    sqlite3_bind_int(stmt, 7, b.getEndTime().hour);
    sqlite3_bind_int(stmt, 8, b.getEndTime().minute);

     if (sqlite3_step(stmt) != SQLITE_DONE) {
        string errMsgStr = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        throw runtime_error("Ошибка выполнения запроса для добавления брони: " + errMsgStr);
    }
    sqlite3_finalize(stmt);
}

void BookingManager::deleteBooking(int bookingId) {
    const char* sql = "DELETE FROM Bookings WHERE bookingId = ?;";
     sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
         string errMsgStr = sqlite3_errmsg(db);
         sqlite3_finalize(stmt);
         throw runtime_error("Ошибка подготовки запроса для удаления брони: " + errMsgStr);
    }
     sqlite3_bind_int(stmt, 1, bookingId);
     if (sqlite3_step(stmt) != SQLITE_DONE) {
        string errMsgStr = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        throw runtime_error("Ошибка выполнения запроса для удаления брони: " + errMsgStr);
    }
    sqlite3_finalize(stmt);
}

void BookingManager::updateBooking(int bookingId, const Booking& b) {
    const char* sql = "UPDATE Bookings SET workstationId = ?, clientName = ?, bookingDate = ?, startHour = ?, startMinute = ?, endHour = ?, endMinute = ? WHERE bookingId = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
         string errMsgStr = sqlite3_errmsg(db);
         sqlite3_finalize(stmt);
         throw runtime_error("Ошибка подготовки запроса для обновления брони: " + errMsgStr);
    }
    sqlite3_bind_int(stmt, 1, b.getWorkstationId());
    sqlite3_bind_text(stmt, 2, b.getClientName().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, b.getBookingDate().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, b.getStartTime().hour);
    sqlite3_bind_int(stmt, 5, b.getStartTime().minute);
    sqlite3_bind_int(stmt, 6, b.getEndTime().hour);
    sqlite3_bind_int(stmt, 7, b.getEndTime().minute);
    sqlite3_bind_int(stmt, 8, bookingId);

     if (sqlite3_step(stmt) != SQLITE_DONE) {
        string errMsgStr = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        throw runtime_error("Ошибка выполнения запроса для обновления брони: " + errMsgStr);
    }
    sqlite3_finalize(stmt);
}
