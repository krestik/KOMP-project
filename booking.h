#ifndef BOOKING_H
#define BOOKING_H

#include <string>
#include <iostream>
#include <chrono>
#include "time.h"

class Booking {
private:
    int bookingId;
    int workstationId;
    std::string clientName;
    std::string bookingDate; // DD-MM-YYYY
    Time startTime;
    Time endTime;

public:
    Booking(int _bookingId, int _workstationId, const std::string& _clientName, const std::string& _bookingDate, Time _start, Time _end);

    void display() const;

    void updateTime(Time newStart, Time newEnd);
    void updateTime(int startHour, int startMinute, int endHour, int endMinute);

    int getBookingId() const { return bookingId; }
    int getWorkstationId() const { return workstationId; }
    std::string getClientName() const { return clientName; }
    std::string getBookingDate() const { return bookingDate; }
    Time getStartTime() const { return startTime; }
    Time getEndTime() const { return endTime; }

    std::chrono::system_clock::time_point getEndDateTime() const;

    bool operator==(const Booking& other) const;
};

#endif // BOOKING_H
