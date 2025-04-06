#ifndef BOOKING_MANAGER_H
#define BOOKING_MANAGER_H

#include <vector>
#include <string>

class Workstation;
class Booking;
struct sqlite3;

class BookingManager {
private:
    sqlite3* db;
    void initializeDatabase();

public:
    BookingManager();
    ~BookingManager();
    BookingManager(const BookingManager&) = delete;
    BookingManager& operator=(const BookingManager&) = delete;

    std::vector<Workstation> loadWorkstations();
    std::vector<Booking> loadBookings();
    void addWorkstation(const Workstation& ws);
    void deleteWorkstation(int id);
    void updateWorkstationStatus(int id, const std::string& newStatus);
    void addBooking(const Booking& b);
    void deleteBooking(int bookingId);
    void updateBooking(int bookingId, const Booking& b);
};

#endif // BOOKING_MANAGER_H
