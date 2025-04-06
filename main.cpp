#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <limits>
#include <ios>
#include <chrono>
#include <algorithm>
#include <vector>
#include <regex>
#include <iomanip>

#include "time.h"
#include "workstation.h"
#include "booking.h"
#include "booking_manager.h"

#define NOMINMAX
#include <windows.h>

using namespace std;

bool parseTimeHHMM(const string& timeStr, Time& resultTime) {
    regex time_regex("^([01]?[0-9]|2[0-3]):([0-5][0-9])$");
    smatch match;
    if (regex_match(timeStr, match, time_regex) && match.size() == 3) {
        try {
            resultTime.hour = stoi(match[1].str());
            resultTime.minute = stoi(match[2].str());
            return true;
        } catch (const std::exception& e) {
            cerr << "Ошибка преобразования времени: " << e.what() << endl;
            return false;
        }
    }
    return false;
}

bool isValidDateFormat(const string& dateStr) {
    regex date_regex("^([0-2][0-9]|3[01])-(0[1-9]|1[0-2])-([0-9]{4})$");
    return regex_match(dateStr, date_regex);
}

void checkAndRemoveExpiredBookings(BookingManager& manager, vector<Booking>& bookingArray, vector<Workstation>& wsArray) {
    auto now = chrono::system_clock::now();
    vector<int> expiredBookingIds;
    vector<int> affectedWorkstationIds;
    bool expiredFound = false;

    cout << "\nПроверка просроченных бронирований..." << endl;

    for (const auto& booking : bookingArray) {
        auto endTimePoint = booking.getEndDateTime();
        if (endTimePoint != chrono::system_clock::time_point::min() && endTimePoint < now) {
            expiredBookingIds.push_back(booking.getBookingId());
            affectedWorkstationIds.push_back(booking.getWorkstationId());
            cout << "Обнаружено просроченное бронирование ID: " << booking.getBookingId() << " для станции " << booking.getWorkstationId() << endl;
            expiredFound = true;
        }
    }

    if (!expiredFound) {
        cout << "Просроченных бронирований не найдено." << endl;
        return;
    }

    for (int expiredId : expiredBookingIds) {
        try {
            manager.deleteBooking(expiredId);
            bookingArray.erase(remove_if(bookingArray.begin(), bookingArray.end(),
                                         [expiredId](const Booking& b){ return b.getBookingId() == expiredId; }),
                               bookingArray.end());
            cout << "Бронирование ID " << expiredId << " удалено (просрочено)." << endl;
        } catch (const exception& e) {
            cerr << "Ошибка при удалении просроченного бронирования ID " << expiredId << ": " << e.what() << endl;
        }
    }

    sort(affectedWorkstationIds.begin(), affectedWorkstationIds.end());
    affectedWorkstationIds.erase(unique(affectedWorkstationIds.begin(), affectedWorkstationIds.end()), affectedWorkstationIds.end());

    for (int wsId : affectedWorkstationIds) {
        bool hasActiveOrFutureBookings = false;
        auto checkTime = chrono::system_clock::now();
        for (const auto& booking : bookingArray) {
            if (booking.getWorkstationId() == wsId) {
                auto bookingEndTime = booking.getEndDateTime();
                if (bookingEndTime != chrono::system_clock::time_point::min() && bookingEndTime >= checkTime) {
                    hasActiveOrFutureBookings = true;
                    break;
                }
            }
        }

        if (!hasActiveOrFutureBookings) {
            for (auto& ws : wsArray) {
                if (ws.getId() == wsId && ws.getStatus() == "booked") {
                    try {
                        manager.updateWorkstationStatus(wsId, "available");
                        ws.updateStatus("available");
                        cout << "Статус станции ID " << wsId << " изменен на 'available' (нет активных броней)." << endl;
                    } catch (const exception& e) {
                         cerr << "Ошибка при обновлении статуса станции ID " << wsId << ": " << e.what() << endl;
                    }
                    break;
                }
            }
        }
    }
     cout << "Проверка просроченных бронирований завершена." << endl;
}

void manageData(BookingManager &manager) {
    vector<Workstation> wsArray;
    vector<Booking> bookingArray;

    try {
        wsArray = manager.loadWorkstations();
        bookingArray = manager.loadBookings();
        cout << "Данные успешно загружены из booking.db." << endl;
    } catch (const exception& e) {
        cerr << "Критическая ошибка при загрузке данных: " << e.what() << endl;
        return;
    }

    checkAndRemoveExpiredBookings(manager, bookingArray, wsArray);

    int choice;
    while (true) {
        cout << "\n===== Главное меню =====\n";
        cout << "1. Управление рабочими станциями\n";
        cout << "2. Управление бронированиями\n";
        cout << "0. Выход\n";
        cout << "Выберите действие: ";

        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Неверный ввод. Пожалуйста, введите число." << endl;
            continue;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        if (choice == 0) {
            cout << "Выход из программы." << endl;
            break;
        }

        switch (choice) {
            case 1: {
                int wsChoice;
                while (true) {
                    cout << "\n===== Меню рабочих станций =====\n";
                    cout << "1. Показать все рабочие станции\n";
                    cout << "2. Добавить рабочую станцию\n";
                    cout << "3. Удалить рабочую станцию\n";
                    cout << "4. Обновить статус рабочей станции\n";
                    cout << "0. Вернуться в главное меню\n";
                    cout << "Выберите действие: ";

                    if (!(cin >> wsChoice)) {
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        cout << "Неверный ввод. Пожалуйста, введите число." << endl;
                        continue;
                    }
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');

                    if (wsChoice == 0) break;

                    try {
                        switch (wsChoice) {
                            case 1: {
                                cout << "\n--- Список рабочих станций ---\n";
                                if (wsArray.empty()) {
                                    cout << "Рабочие станции не найдены." << endl;
                                } else {
                                    for (const auto &ws : wsArray) {
                                        ws.display();
                                    }
                                }
                                break;
                            }
                            case 2: {
                                int id;
                                string name;
                                cout << "Введите ID новой станции: ";
                                if (!(cin >> id)) {
                                    cin.clear();
                                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                                    cout << "Неверный ID.\n";
                                    continue;
                                }
                                cin.ignore(numeric_limits<streamsize>::max(), '\n');

                                cout << "Введите название станции: ";
                                getline(cin, name);
                                if(name.empty()) {
                                    cout << "Название не может быть пустым.\n";
                                    continue;
                                }

                                bool id_exists = false;
                                for(const auto& ws : wsArray) {
                                    if(ws.getId() == id) {
                                        id_exists = true;
                                        break;
                                    }
                                }

                                if(id_exists) {
                                    cout << "Ошибка: Станция с ID " << id << " уже существует." << endl;
                                    continue;
                                }

                                Workstation new_ws(id, name);
                                manager.addWorkstation(new_ws);
                                wsArray.push_back(new_ws);
                                cout << "Рабочая станция добавлена." << endl;
                                break;
                            }
                            case 3: {
                                int id_to_delete;
                                cout << "Введите ID станции для удаления: ";
                                if (!(cin >> id_to_delete)) {
                                    cin.clear();
                                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                                    cout << "Неверный ID.\n";
                                    continue;
                                }
                                cin.ignore(numeric_limits<streamsize>::max(), '\n');

                                size_t initial_size = wsArray.size();
                                wsArray.erase(remove_if(wsArray.begin(), wsArray.end(),
                                    [id_to_delete](const Workstation& ws){
                                        return ws.getId() == id_to_delete;
                                    }), wsArray.end());

                                if (wsArray.size() < initial_size) {
                                    manager.deleteWorkstation(id_to_delete);
                                    cout << "Рабочая станция удалена." << endl;

                                    bookingArray.erase(remove_if(bookingArray.begin(), bookingArray.end(),
                                        [id_to_delete](const Booking& b){
                                            return b.getWorkstationId() == id_to_delete;
                                        }), bookingArray.end());
                                    cout << "Связанные бронирования также удалены." << endl;
                                } else {
                                    cout << "Станция с таким ID не найдена." << endl;
                                }
                                break;
                            }
                            case 4: {
                                int id_to_update;
                                string newStatus;
                                cout << "Введите ID станции для обновления: ";
                                if (!(cin >> id_to_update)) {
                                    cin.clear();
                                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                                    cout << "Неверный ID.\n";
                                    continue;
                                }
                                cin.ignore(numeric_limits<streamsize>::max(), '\n');

                                cout << "Введите новый статус (available/booked/maintenance): ";
                                getline(cin, newStatus);

                                if (newStatus != "available" && newStatus != "booked" && newStatus != "maintenance") {
                                    cout << "Недопустимый статус. Используйте 'available', 'booked' или 'maintenance'." << endl;
                                    continue;
                                }

                                bool found = false;
                                for (auto &ws : wsArray) {
                                    if (ws.getId() == id_to_update) {
                                        manager.updateWorkstationStatus(id_to_update, newStatus);
                                        ws.updateStatus(newStatus);
                                        found = true;
                                        break;
                                    }
                                }

                                if (found) {
                                    cout << "Статус обновлён." << endl;
                                } else {
                                    cout << "Станция с таким ID не найдена." << endl;
                                }
                                break;
                            }
                            default:
                                cout << "Неверный выбор. Попробуйте снова." << endl;
                        }
                    } catch (const exception& e) {
                        cerr << "Произошла ошибка при работе со станциями: " << e.what() << endl;
                    }
                }
                break;
            }

            case 2: {
                int bookChoice;
                while (true) {
                    cout << "\n===== Меню бронирований =====\n";
                    cout << "1. Показать все бронирования\n";
                    cout << "2. Добавить бронирование\n";
                    cout << "3. Удалить бронирование\n";
                    cout << "4. Обновить бронирование\n";
                    cout << "0. Вернуться в главное меню\n";
                    cout << "Выберите действие: ";

                    if (!(cin >> bookChoice)) {
                        cin.clear();
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                        cout << "Неверный ввод. Пожалуйста, введите число." << endl;
                        continue;
                    }
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');

                    if (bookChoice == 0) break;

                    try {
                        switch (bookChoice) {
                            case 1: {
                                cout << "\n--- Список бронирований ---\n";
                                checkAndRemoveExpiredBookings(manager, bookingArray, wsArray);
                                if (bookingArray.empty()) {
                                    cout << "Актуальные бронирования не найдены." << endl;
                                } else {
                                    for (const auto &b : bookingArray) {
                                        b.display();
                                    }
                                }
                                break;
                            }
                            case 2: {
                                int bookingId, workstationId;
                                string clientName, bookingDateStr, startTimeStr, endTimeStr;
                                Time start = {0, 0}, end = {0, 0};

                                cout << "Введите ID нового бронирования: ";
                                if (!(cin >> bookingId)) {
                                    cin.clear();
                                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                                    cout << "Неверный ID.\n";
                                    continue;
                                }
                                cin.ignore(numeric_limits<streamsize>::max(), '\n');

                                bool book_id_exists = false;
                                for(const auto& b : bookingArray) {
                                    if(b.getBookingId() == bookingId) {
                                        book_id_exists = true;
                                        break;
                                    }
                                }

                                if(book_id_exists) {
                                    cout << "Ошибка: Бронирование с ID " << bookingId << " уже существует.\n";
                                    continue;
                                }

                                cout << "Введите ID рабочей станции для бронирования: ";
                                if (!(cin >> workstationId)) {
                                    cin.clear();
                                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                                    cout << "Неверный ID станции.\n";
                                    continue;
                                }
                                cin.ignore(numeric_limits<streamsize>::max(), '\n');

                                bool ws_exists = false;
                                for(const auto& ws : wsArray) {
                                    if(ws.getId() == workstationId) {
                                        ws_exists = true;
                                        break;
                                    }
                                }

                                if(!ws_exists) {
                                    cout << "Ошибка: Станция с ID " << workstationId << " не найдена.\n";
                                    continue;
                                }

                                cout << "Введите имя клиента: ";
                                getline(cin, clientName);
                                if(clientName.empty()) {
                                    cout << "Имя клиента не может быть пустым.\n";
                                    continue;
                                }

                                cout << "Введите дату бронирования (формат DD-MM-YYYY): ";
                                getline(cin, bookingDateStr);
                                if (!isValidDateFormat(bookingDateStr)) {
                                    cout << "Ошибка: Неверный формат даты. Используйте DD-MM-YYYY." << endl;
                                    continue;
                                }

                                cout << "Введите время начала (формат HH:MM): ";
                                getline(cin, startTimeStr);
                                if (!parseTimeHHMM(startTimeStr, start)) {
                                    cout << "Ошибка: Неверный формат времени начала. Используйте HH:MM." << endl;
                                    continue;
                                }

                                cout << "Введите время окончания (формат HH:MM): ";
                                getline(cin, endTimeStr);
                                if (!parseTimeHHMM(endTimeStr, end)) {
                                    cout << "Ошибка: Неверный формат времени окончания. Используйте HH:MM." << endl;
                                    continue;
                                }

                                if (start.hour * 60 + start.minute >= end.hour * 60 + end.minute) {
                                    cout << "Ошибка: Время начала должно быть раньше времени окончания.\n";
                                    continue;
                                }

                                Booking new_b(bookingId, workstationId, clientName, bookingDateStr, start, end);
                                auto endTimePoint = new_b.getEndDateTime();

                                if (endTimePoint != chrono::system_clock::time_point::min() &&
                                    endTimePoint < chrono::system_clock::now()) {
                                    cout << "Ошибка: Нельзя добавить бронирование на уже прошедшее время." << endl;
                                    continue;
                                }

                                bool conflictDetected = false;
                                int new_start_minutes = new_b.getStartTime().hour * 60 + new_b.getStartTime().minute;
                                int new_end_minutes = new_b.getEndTime().hour * 60 + new_b.getEndTime().minute;

                                for (const auto& existing_booking : bookingArray) {
                                    if (existing_booking.getWorkstationId() == new_b.getWorkstationId() &&
                                        existing_booking.getBookingDate() == new_b.getBookingDate()) {
                                        int existing_start_minutes = existing_booking.getStartTime().hour * 60 +
                                                                   existing_booking.getStartTime().minute;
                                        int existing_end_minutes = existing_booking.getEndTime().hour * 60 +
                                                                 existing_booking.getEndTime().minute;

                                        if (new_start_minutes < existing_end_minutes &&
                                            existing_start_minutes < new_end_minutes) {
                                            conflictDetected = true;
                                            cerr << "Ошибка: Конфликт времени! Станция " << new_b.getWorkstationId()
                                                 << " уже забронирована в это время (ID существующей брони: "
                                                 << existing_booking.getBookingId() << ")." << endl;
                                            break;
                                        }
                                    }
                                }

                                if (conflictDetected) {
                                    continue;
                                }

                                manager.addBooking(new_b);
                                bookingArray.push_back(new_b);

                                bool status_updated = false;
                                for (auto &ws : wsArray) {
                                    if (ws.getId() == workstationId) {
                                        if (ws.getStatus() != "booked") {
                                            manager.updateWorkstationStatus(workstationId, "booked");
                                            ws.updateStatus("booked");
                                            status_updated = true;
                                        }
                                        break;
                                    }
                                }

                                cout << "Бронирование добавлено."
                                     << (status_updated ? " Статус станции обновлен на 'booked'." : "") << endl;
                                break;
                            }
                            case 3: {
                                int bookingId_to_delete;
                                cout << "Введите ID бронирования для удаления: ";
                                if (!(cin >> bookingId_to_delete)) {
                                    cin.clear();
                                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                                    cout << "Неверный ID.\n";
                                    continue;
                                }
                                cin.ignore(numeric_limits<streamsize>::max(), '\n');

                                int wsId_of_deleted_booking = -1;
                                size_t initial_size = bookingArray.size();

                                bookingArray.erase(
                                    remove_if(bookingArray.begin(), bookingArray.end(),
                                        [&](const Booking& b){
                                            if (b.getBookingId() == bookingId_to_delete) {
                                                wsId_of_deleted_booking = b.getWorkstationId();
                                                return true;
                                            }
                                            return false;
                                        }
                                    ),
                                    bookingArray.end()
                                );

                                if (bookingArray.size() < initial_size) {
                                    manager.deleteBooking(bookingId_to_delete);
                                    cout << "Бронирование удалено." << endl;

                                    bool other_active_bookings_exist = false;
                                    if (wsId_of_deleted_booking != -1) {
                                        auto checkTime = chrono::system_clock::now();
                                        for(const auto& b : bookingArray) {
                                            if (b.getWorkstationId() == wsId_of_deleted_booking) {
                                                auto bookingEndTime = b.getEndDateTime();
                                                if(bookingEndTime != chrono::system_clock::time_point::min() &&
                                                   bookingEndTime >= checkTime) {
                                                    other_active_bookings_exist = true;
                                                    break;
                                                }
                                            }
                                        }

                                        if (!other_active_bookings_exist) {
                                            for(auto& ws : wsArray) {
                                                if(ws.getId() == wsId_of_deleted_booking &&
                                                   ws.getStatus() == "booked") {
                                                    manager.updateWorkstationStatus(wsId_of_deleted_booking, "available");
                                                    ws.updateStatus("available");
                                                    cout << "Статус станции " << wsId_of_deleted_booking
                                                         << " изменен на 'available', так как других активных броней нет."
                                                         << endl;
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                } else {
                                    cout << "Бронирование с таким ID не найдено." << endl;
                                }
                                break;
                            }
                            case 4: {
                                int bookingId_to_update;
                                cout << "Введите ID бронирования для обновления: ";
                                if (!(cin >> bookingId_to_update)) {
                                    cin.clear();
                                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                                    cout << "Неверный ID.\n";
                                    continue;
                                }
                                cin.ignore(numeric_limits<streamsize>::max(), '\n');

                                Booking* booking_ptr = nullptr;
                                for(auto& b : bookingArray) {
                                    if(b.getBookingId() == bookingId_to_update) {
                                        booking_ptr = &b;
                                        break;
                                    }
                                }

                                if (!booking_ptr) {
                                    cout << "Бронирование с ID " << bookingId_to_update << " не найдено." << endl;
                                    continue;
                                }

                                int new_workstationId;
                                string new_clientName, new_bookingDateStr, new_startTimeStr, new_endTimeStr;
                                Time new_start, new_end;

                                cout << "Введите новый ID рабочей станции (текущий: "
                                     << booking_ptr->getWorkstationId() << "): ";
                                if (!(cin >> new_workstationId)) {
                                    cin.clear();
                                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                                    cout << "Неверный ID станции.\n";
                                    continue;
                                }
                                cin.ignore(numeric_limits<streamsize>::max(), '\n');

                                bool new_ws_exists = false;
                                for(const auto& ws : wsArray) {
                                    if(ws.getId() == new_workstationId) {
                                        new_ws_exists = true;
                                        break;
                                    }
                                }

                                if(!new_ws_exists) {
                                    cout << "Ошибка: Станция с ID " << new_workstationId << " не найдена.\n";
                                    continue;
                                }

                                cout << "Введите новое имя клиента (текущее: " << booking_ptr->getClientName()
                                     << ", Enter чтобы оставить): ";
                                getline(cin, new_clientName);
                                if(new_clientName.empty()) {
                                    new_clientName = booking_ptr->getClientName();
                                }

                                cout << "Введите новую дату (DD-MM-YYYY) (текущая: " << booking_ptr->getBookingDate()
                                     << ", Enter чтобы оставить): ";
                                getline(cin, new_bookingDateStr);
                                if(new_bookingDateStr.empty()) {
                                    new_bookingDateStr = booking_ptr->getBookingDate();
                                } else if (!isValidDateFormat(new_bookingDateStr)) {
                                    cout << "Ошибка: Неверный формат даты. Используйте DD-MM-YYYY." << endl;
                                    continue;
                                }

                                cout << "Введите новое время начала (HH:MM) (текущее: " << setfill('0') << setw(2)
                                     << booking_ptr->getStartTime().hour << ":" << setw(2)
                                     << booking_ptr->getStartTime().minute << ", Enter чтобы оставить): ";
                                getline(cin, new_startTimeStr);
                                if(new_startTimeStr.empty()) {
                                    new_start = booking_ptr->getStartTime();
                                } else if (!parseTimeHHMM(new_startTimeStr, new_start)) {
                                    cout << "Ошибка: Неверный формат времени начала. Используйте HH:MM." << endl;
                                    continue;
                                }

                                cout << "Введите новое время окончания (HH:MM) (текущее: " << setfill('0') << setw(2)
                                     << booking_ptr->getEndTime().hour << ":" << setw(2)
                                     << booking_ptr->getEndTime().minute << ", Enter чтобы оставить): ";
                                getline(cin, new_endTimeStr);
                                if(new_endTimeStr.empty()) {
                                    new_end = booking_ptr->getEndTime();
                                } else if (!parseTimeHHMM(new_endTimeStr, new_end)) {
                                    cout << "Ошибка: Неверный формат времени окончания. Используйте HH:MM." << endl;
                                    continue;
                                }

                                if (new_start.hour * 60 + new_start.minute >= new_end.hour * 60 + new_end.minute) {
                                    cout << "Ошибка: Время начала должно быть раньше времени окончания.\n";
                                    continue;
                                }

                                Booking updated_b(bookingId_to_update, new_workstationId, new_clientName,
                                                new_bookingDateStr, new_start, new_end);
                                auto endTimePoint = updated_b.getEndDateTime();

                                if (endTimePoint != chrono::system_clock::time_point::min() &&
                                    endTimePoint < chrono::system_clock::now()) {
                                    cout << "Ошибка: Нельзя обновить бронирование на уже прошедшее время." << endl;
                                    continue;
                                }

                                bool conflictDetected = false;
                                int new_start_minutes = updated_b.getStartTime().hour * 60 +
                                                      updated_b.getStartTime().minute;
                                int new_end_minutes = updated_b.getEndTime().hour * 60 +
                                                    updated_b.getEndTime().minute;

                                for (const auto& existing_booking : bookingArray) {
                                    if (existing_booking.getBookingId() == updated_b.getBookingId()) {
                                        continue;
                                    }

                                    if (existing_booking.getWorkstationId() == updated_b.getWorkstationId() &&
                                        existing_booking.getBookingDate() == updated_b.getBookingDate()) {
                                        int existing_start_minutes = existing_booking.getStartTime().hour * 60 +
                                                                   existing_booking.getStartTime().minute;
                                        int existing_end_minutes = existing_booking.getEndTime().hour * 60 +
                                                                 existing_booking.getEndTime().minute;

                                        if (new_start_minutes < existing_end_minutes &&
                                            existing_start_minutes < new_end_minutes) {
                                            conflictDetected = true;
                                            cerr << "Ошибка: Конфликт времени при обновлении! Станция "
                                                 << updated_b.getWorkstationId()
                                                 << " уже забронирована в это время (ID существующей брони: "
                                                 << existing_booking.getBookingId() << ")." << endl;
                                            break;
                                        }
                                    }
                                }

                                if (conflictDetected) {
                                    continue;
                                }

                                manager.updateBooking(bookingId_to_update, updated_b);
                                *booking_ptr = updated_b;
                                cout << "Бронирование обновлено." << endl;
                                break;
                            }
                            default:
                                cout << "Неверный выбор. Попробуйте снова." << endl;
                        }
                    } catch (const exception& e) {
                        cerr << "Произошла ошибка при работе с бронированиями: " << e.what() << endl;
                    }
                }
                break;
            }
            default:
                cout << "Неверный выбор. Пожалуйста, попробуйте снова." << endl;
        }
    }
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    cout.sync_with_stdio(false);
    cin.tie(nullptr);

    unique_ptr<BookingManager> manager_ptr;
    try {
        manager_ptr = make_unique<BookingManager>();
        manageData(*manager_ptr);
    } catch (const exception &ex) {
        cerr << "Критическая ошибка программы: " << ex.what() << endl;
        return 1;
    }
    return 0;
}
