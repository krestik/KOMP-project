#include <iostream>
#include <string>
#include <stdexcept>
#include <sstream>
#include <vector>
#include <sqlite3.h>

using namespace std;

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef byte // Убираем неоднозначность с std::byte
#endif

// Структура для представления времени бронирования
struct Time {
    int hour;
    int minute;
};

// Класс для представления рабочей станции
class Workstation {
protected:
    int id;
    string name;
    string status; // "available" или "booked"
public:
    Workstation(int _id, const string& _name, const string& _status = "available")
        : id(_id), name(_name), status(_status) {}

    virtual void display() const {
        cout << "ID: " << id << ", Название: " << name << ", Статус: " << status << endl;
    }

    virtual void updateStatus(const string& newStatus) {
        status = newStatus;
    }

    int getId() const { return id; }
    string getName() const { return name; }
    string getStatus() const { return status; }

    // Перегрузка оператора вывода
    friend ostream& operator<<(ostream& os, const Workstation& ws) {
        os << "Рабочая станция [ID: " << ws.id << ", Название: " << ws.name << ", Статус: " << ws.status << "]";
        return os;
    }
};

// Производный класс с дополнительным полем (пример наследования)
class SpecialWorkstation : public Workstation {
private:
    int performanceRating;
public:
    SpecialWorkstation(int _id, const string& _name, const string& _status, int _rating)
        : Workstation(_id, _name, _status), performanceRating(_rating) {}

    void display() const override {
        Workstation::display();
        cout << "Рейтинг производительности: " << performanceRating << endl;
    }
};

// Класс для представления бронирования (дата – в формате "YYYY-MM-DD")
class Booking {
private:
    int bookingId;
    int workstationId;
    string clientName;
    string bookingDate; // дата бронирования
    Time startTime;
    Time endTime;
public:
    Booking(int _bookingId, int _workstationId, const string& _clientName, const string& _bookingDate, Time _start, Time _end)
        : bookingId(_bookingId), workstationId(_workstationId), clientName(_clientName), bookingDate(_bookingDate), startTime(_start), endTime(_end) {}

    void display() const {
        cout << "ID бронирования: " << bookingId
             << ", ID рабочей станции: " << workstationId
             << ", Клиент: " << clientName
             << ", Дата: " << bookingDate
             << ", Время: " << startTime.hour << ":" << (startTime.minute < 10 ? "0" : "") << startTime.minute
             << " - " << endTime.hour << ":" << (endTime.minute < 10 ? "0" : "") << endTime.minute
             << endl;
    }

    // Перегрузка методов для обновления времени бронирования (дата не меняется)
    void updateTime(Time newStart, Time newEnd) {
        startTime = newStart;
        endTime = newEnd;
    }

    void updateTime(int startHour, int startMinute, int endHour, int endMinute) {
        startTime.hour = startHour;
        startTime.minute = startMinute;
        endTime.hour = endHour;
        endTime.minute = endMinute;
    }

    int getBookingId() const { return bookingId; }
    int getWorkstationId() const { return workstationId; }
    string getClientName() const { return clientName; }
    string getBookingDate() const { return bookingDate; }
    Time getStartTime() const { return startTime; }
    Time getEndTime() const { return endTime; }

    bool operator==(const Booking& other) const {
        return bookingId == other.bookingId;
    }
};

// Класс для управления данными в базе данных SQLite
class BookingManager {
private:
    sqlite3* db;
public:
    BookingManager() : db(nullptr) {
        int rc = sqlite3_open("booking.db", &db);
        if (rc) {
            throw runtime_error("Не удалось открыть базу данных");
        }
        // Создание таблицы рабочих станций, если её ещё нет
        const char* sql1 = "CREATE TABLE IF NOT EXISTS Workstations (id INTEGER PRIMARY KEY, name TEXT, status TEXT);";
        // Создание таблицы бронирований с полем bookingDate, если её ещё нет
        const char* sql2 = "CREATE TABLE IF NOT EXISTS Bookings (bookingId INTEGER PRIMARY KEY, workstationId INTEGER, clientName TEXT, bookingDate TEXT, startHour INTEGER, startMinute INTEGER, endHour INTEGER, endMinute INTEGER);";
        char* errMsg = nullptr;
        rc = sqlite3_exec(db, sql1, 0, 0, &errMsg);
        if (rc != SQLITE_OK) {
            string err = errMsg;
            sqlite3_free(errMsg);
            throw runtime_error("Ошибка SQL: " + err);
        }
        rc = sqlite3_exec(db, sql2, 0, 0, &errMsg);
        if (rc != SQLITE_OK) {
            string err = errMsg;
            sqlite3_free(errMsg);
            throw runtime_error("Ошибка SQL: " + err);
        }
    }

    ~BookingManager() {
        if (db) sqlite3_close(db);
    }

    // Загрузка рабочих станций из БД в вектор
    vector<Workstation> loadWorkstations() {
        vector<Workstation> result;
        const char* sql = "SELECT id, name, status FROM Workstations;";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw runtime_error("Ошибка подготовки запроса для загрузки рабочих станций");
        }
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            const unsigned char* nameText = sqlite3_column_text(stmt, 1);
            const unsigned char* statusText = sqlite3_column_text(stmt, 2);
            string name = nameText ? reinterpret_cast<const char*>(nameText) : "";
            string status = statusText ? reinterpret_cast<const char*>(statusText) : "";
            result.push_back(Workstation(id, name, status));
        }
        sqlite3_finalize(stmt);
        return result;
    }

    // Загрузка бронирований из БД в вектор
    vector<Booking> loadBookings() {
        vector<Booking> result;
        const char* sql = "SELECT bookingId, workstationId, clientName, bookingDate, startHour, startMinute, endHour, endMinute FROM Bookings;";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw runtime_error("Ошибка подготовки запроса для загрузки бронирований");
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
            result.push_back(Booking(bookingId, workstationId, clientName, bookingDate, start, end));
        }
        sqlite3_finalize(stmt);
        return result;
    }

    // Добавление рабочей станции в БД (статус задаётся по умолчанию "available")
    void addWorkstation(const Workstation& ws) {
        stringstream ss;
        ss << "INSERT INTO Workstations (id, name, status) VALUES ("
           << ws.getId() << ", '" << ws.getName() << "', 'available');";
        string sql = ss.str();
        char* errMsg = nullptr;
        int rc = sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg);
        if (rc != SQLITE_OK) {
            string err = errMsg;
            sqlite3_free(errMsg);
            throw runtime_error("Ошибка SQL: " + err);
        }
    }

    // Удаление рабочей станции из БД по ID
    void deleteWorkstation(int id) {
        string sql = "DELETE FROM Workstations WHERE id = " + to_string(id) + ";";
        char* errMsg = nullptr;
        int rc = sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg);
        if (rc != SQLITE_OK) {
            string err = errMsg;
            sqlite3_free(errMsg);
            throw runtime_error("Ошибка SQL: " + err);
        }
    }

    // Обновление статуса рабочей станции в БД по ID
    void updateWorkstationStatus(int id, const string& newStatus) {
        string sql = "UPDATE Workstations SET status = '" + newStatus + "' WHERE id = " + to_string(id) + ";";
        char* errMsg = nullptr;
        int rc = sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg);
        if (rc != SQLITE_OK) {
            string err = errMsg;
            sqlite3_free(errMsg);
            throw runtime_error("Ошибка SQL: " + err);
        }
    }

    // Добавление бронирования в БД
    void addBooking(const Booking& b) {
        stringstream ss;
        ss << "INSERT INTO Bookings (bookingId, workstationId, clientName, bookingDate, startHour, startMinute, endHour, endMinute) VALUES ("
           << b.getBookingId() << ", " << b.getWorkstationId() << ", '" << b.getClientName() << "', '" << b.getBookingDate() << "', "
           << b.getStartTime().hour << ", " << b.getStartTime().minute << ", "
           << b.getEndTime().hour << ", " << b.getEndTime().minute << ");";
        string sql = ss.str();
        char* errMsg = nullptr;
        int rc = sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg);
        if (rc != SQLITE_OK) {
            string err = errMsg;
            sqlite3_free(errMsg);
            throw runtime_error("Ошибка SQL: " + err);
        }
    }

    // Удаление бронирования из БД по bookingId
    void deleteBooking(int bookingId) {
        string sql = "DELETE FROM Bookings WHERE bookingId = " + to_string(bookingId) + ";";
        char* errMsg = nullptr;
        int rc = sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg);
        if (rc != SQLITE_OK) {
            string err = errMsg;
            sqlite3_free(errMsg);
            throw runtime_error("Ошибка SQL: " + err);
        }
    }

    // Обновление бронирования в БД по bookingId
    void updateBooking(int bookingId, const Booking& b) {
        stringstream ss;
        ss << "UPDATE Bookings SET workstationId = " << b.getWorkstationId()
           << ", clientName = '" << b.getClientName() << "', bookingDate = '" << b.getBookingDate() << "', startHour = " << b.getStartTime().hour
           << ", startMinute = " << b.getStartTime().minute << ", endHour = " << b.getEndTime().hour
           << ", endMinute = " << b.getEndTime().minute << " WHERE bookingId = " << bookingId << ";";
        string sql = ss.str();
        char* errMsg = nullptr;
        int rc = sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg);
        if (rc != SQLITE_OK) {
            string err = errMsg;
            sqlite3_free(errMsg);
            throw runtime_error("Ошибка SQL: " + err);
        }
    }
};

// Функция для управления рабочими станциями и бронированиями (синхронизация массива и БД)
void manageData(BookingManager &manager) {
    // Загружаем данные из БД в in-memory массивы
    vector<Workstation> wsArray = manager.loadWorkstations();
    vector<Booking> bookingArray = manager.loadBookings();
    int choice;
    while (true) {
        cout << "\n===== Главное меню =====\n";
        cout << "1. Управление рабочими станциями\n";
        cout << "2. Управление бронированиями\n";
        cout << "0. Выход\n";
        cout << "Выберите действие: ";
        cin >> choice;
        if (cin.fail()) {
            cin.clear();
            cin.ignore(256, '\n');
            cout << "Неверный ввод. Повторите." << endl;
            continue;
        }
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
                    cin >> wsChoice;
                    if (cin.fail()) {
                        cin.clear();
                        cin.ignore(256, '\n');
                        cout << "Неверный ввод. Повторите." << endl;
                        continue;
                    }
                    if (wsChoice == 0) break;
                    switch (wsChoice) {
                        case 1: {
                            cout << "\n--- Список рабочих станций ---\n";
                            for (const auto &ws : wsArray) {
                                ws.display();
                            }
                            break;
                        }
                        case 2: {
                            int id;
                            string name;
                            cout << "Введите ID станции: ";
                            cin >> id;
                            cout << "Введите название станции: ";
                            cin.ignore();
                            getline(cin, name);
                            Workstation ws(id, name); // статус автоматически "available"
                            manager.addWorkstation(ws);
                            wsArray.push_back(ws);
                            cout << "Рабочая станция добавлена." << endl;
                            break;
                        }
                        case 3: {
                            int id;
                            cout << "Введите ID станции для удаления: ";
                            cin >> id;
                            bool found = false;
                            for (auto it = wsArray.begin(); it != wsArray.end(); ++it) {
                                if (it->getId() == id) {
                                    wsArray.erase(it);
                                    found = true;
                                    break;
                                }
                            }
                            if (found) {
                                manager.deleteWorkstation(id);
                                cout << "Рабочая станция удалена." << endl;
                            } else {
                                cout << "Станция с таким ID не найдена." << endl;
                            }
                            break;
                        }
                        case 4: {
                            int id;
                            string newStatus;
                            cout << "Введите ID станции для обновления: ";
                            cin >> id;
                            cout << "Введите новый статус: ";
                            cin >> newStatus;
                            bool found = false;
                            for (auto &ws : wsArray) {
                                if (ws.getId() == id) {
                                    ws.updateStatus(newStatus);
                                    found = true;
                                    break;
                                }
                            }
                            if (found) {
                                manager.updateWorkstationStatus(id, newStatus);
                                cout << "Статус обновлён." << endl;
                            } else {
                                cout << "Станция с таким ID не найдена." << endl;
                            }
                            break;
                        }
                        default:
                            cout << "Неверный выбор. Попробуйте снова." << endl;
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
                    cin >> bookChoice;
                    if (cin.fail()) {
                        cin.clear();
                        cin.ignore(256, '\n');
                        cout << "Неверный ввод. Повторите." << endl;
                        continue;
                    }
                    if (bookChoice == 0) break;
                    switch (bookChoice) {
                        case 1: {
                            cout << "\n--- Список бронирований ---\n";
                            for (const auto &b : bookingArray) {
                                b.display();
                            }
                            break;
                        }
                        case 2: {
                            int bookingId, workstationId;
                            string clientName, bookingDate;
                            Time start, end;
                            cout << "Введите ID бронирования: ";
                            cin >> bookingId;
                            cout << "Введите ID рабочей станции: ";
                            cin >> workstationId;
                            cout << "Введите имя клиента: ";
                            cin.ignore();
                            getline(cin, clientName);
                            cout << "Введите дату бронирования (YYYY-MM-DD): ";
                            getline(cin, bookingDate);
                            cout << "Введите время начала (час минута): ";
                            cin >> start.hour >> start.minute;
                            cout << "Введите время окончания (час минута): ";
                            cin >> end.hour >> end.minute;
                            Booking b(bookingId, workstationId, clientName, bookingDate, start, end);
                            manager.addBooking(b);
                            bookingArray.push_back(b);
                            // При создании бронирования статус рабочей станции меняется на "booked"
                            manager.updateWorkstationStatus(workstationId, "booked");
                            // Обновляем статус в массиве рабочих станций
                            for (auto &ws : wsArray) {
                                if (ws.getId() == workstationId) {
                                    ws.updateStatus("booked");
                                    break;
                                }
                            }
                            cout << "Бронирование добавлено, статус станции изменён на 'booked'." << endl;
                            break;
                        }
                        case 3: {
                            int bookingId;
                            cout << "Введите ID бронирования для удаления: ";
                            cin >> bookingId;
                            bool found = false;
                            for (auto it = bookingArray.begin(); it != bookingArray.end(); ++it) {
                                if (it->getBookingId() == bookingId) {
                                    // При удалении бронирования можно, при необходимости, обновлять статус станции
                                    bookingArray.erase(it);
                                    found = true;
                                    break;
                                }
                            }
                            if (found) {
                                manager.deleteBooking(bookingId);
                                cout << "Бронирование удалено." << endl;
                            } else {
                                cout << "Бронирование с таким ID не найдено." << endl;
                            }
                            break;
                        }
                        case 4: {
                            int bookingId, workstationId;
                            string clientName, bookingDate;
                            Time start, end;
                            cout << "Введите ID бронирования для обновления: ";
                            cin >> bookingId;
                            cout << "Введите новый ID рабочей станции: ";
                            cin >> workstationId;
                            cout << "Введите новое имя клиента: ";
                            cin.ignore();
                            getline(cin, clientName);
                            cout << "Введите новую дату бронирования (YYYY-MM-DD): ";
                            getline(cin, bookingDate);
                            cout << "Введите новое время начала (час минута): ";
                            cin >> start.hour >> start.minute;
                            cout << "Введите новое время окончания (час минута): ";
                            cin >> end.hour >> end.minute;
                            Booking b(bookingId, workstationId, clientName, bookingDate, start, end);
                            bool found = false;
                            for (auto &bk : bookingArray) {
                                if (bk.getBookingId() == bookingId) {
                                    bk = b;
                                    found = true;
                                    break;
                                }
                            }
                            if (found) {
                                manager.updateBooking(bookingId, b);
                                cout << "Бронирование обновлено." << endl;
                            } else {
                                cout << "Бронирование с таким ID не найдено." << endl;
                            }
                            break;
                        }
                        default:
                            cout << "Неверный выбор. Попробуйте снова." << endl;
                    }
                }
                break;
            }
            default:
                cout << "Неверный выбор. Попробуйте снова." << endl;
        }
    }
}

int main() {
#ifdef _WIN32
    // Установка кодовой страницы консоли на UTF-8 для корректного отображения русских символов
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif

    try {
        manageData(*(new BookingManager()));
    } catch (exception &ex) {
        cerr << "Ошибка: " << ex.what() << endl;
    }

    return 0;
}
