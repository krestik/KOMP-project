#include "booking.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <chrono>
#include <ctime>

using namespace std;

std::chrono::system_clock::time_point parseDateTimeInternal(const std::string& dateStr, const Time& timeStruct) {
    std::tm tm = {};
    std::stringstream ss;
    ss << dateStr << " "
       << std::setw(2) << std::setfill('0') << timeStruct.hour << ":"
       << std::setw(2) << std::setfill('0') << timeStruct.minute << ":00";

    ss >> std::get_time(&tm, "%d-%m-%Y %H:%M:%S");

    if (ss.fail()) {
        cerr << "Предупреждение: Ошибка парсинга даты/времени: '" << ss.str() << "'" << endl;
        return std::chrono::system_clock::time_point::min();
    }

    tm.tm_isdst = -1;

    std::time_t tt = std::mktime(&tm);
    if (tt == -1) {
        cerr << "Предупреждение: Ошибка преобразования в time_t для '" << ss.str() << "'" << endl;
        return std::chrono::system_clock::time_point::min();
    }

    return std::chrono::system_clock::from_time_t(tt);
}


Booking::Booking(int _bookingId, int _workstationId, const std::string& _clientName, const std::string& _bookingDate, Time _start, Time _end)
    : bookingId(_bookingId), workstationId(_workstationId), clientName(_clientName), bookingDate(_bookingDate), startTime(_start), endTime(_end) {}

void Booking::display() const {
    std::cout << "ID бронирования: " << bookingId
         << ", ID рабочей станции: " << workstationId
         << ", Клиент: " << clientName
         << ", Дата: " << bookingDate
         << ", Время: "
         << std::setw(2) << std::setfill('0') << startTime.hour << ":"
         << std::setw(2) << std::setfill('0') << startTime.minute
         << " - "
         << std::setw(2) << std::setfill('0') << endTime.hour << ":"
         << std::setw(2) << std::setfill('0') << endTime.minute
         << std::endl;
}

void Booking::updateTime(Time newStart, Time newEnd) {
    startTime = newStart;
    endTime = newEnd;
}

void Booking::updateTime(int startHour, int startMinute, int endHour, int endMinute) {
    startTime.hour = startHour;
    startTime.minute = startMinute;
    endTime.hour = endHour;
    endTime.minute = endMinute;
}

std::chrono::system_clock::time_point Booking::getEndDateTime() const {
    return parseDateTimeInternal(bookingDate, endTime);
}


bool Booking::operator==(const Booking& other) const {
    return bookingId == other.bookingId;
}
