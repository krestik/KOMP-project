#include "workstation.h"
#include <iostream>

Workstation::Workstation(int _id, const std::string& _name, const std::string& _status)
    : id(_id), name(_name), status(_status) {}

void Workstation::display() const {
    std::cout << "ID: " << id << ", Название: " << name << ", Статус: " << status << std::endl;
}

void Workstation::updateStatus(const std::string& newStatus) {
    status = newStatus;
}

SpecialWorkstation::SpecialWorkstation(int _id, const std::string& _name, const std::string& _status, int _rating)
    : Workstation(_id, _name, _status), performanceRating(_rating) {}

void SpecialWorkstation::display() const {
    Workstation::display();
    std::cout << "Рейтинг производительности: " << performanceRating << std::endl;
}

std::ostream& operator<<(std::ostream& os, const Workstation& ws) {
    os << "Рабочая станция [ID: " << ws.getId() << ", Название: " << ws.getName() << ", Статус: " << ws.getStatus() << "]";
    return os;
}
