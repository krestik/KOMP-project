#ifndef WORKSTATION_H
#define WORKSTATION_H

#include <string>
#include <iostream>

class Workstation {
protected:
    int id;
    std::string name;
    std::string status;

public:
    Workstation(int _id, const std::string& _name, const std::string& _status = "available");
    virtual ~Workstation() = default;

    virtual void display() const;
    virtual void updateStatus(const std::string& newStatus);

    int getId() const { return id; }
    std::string getName() const { return name; }
    std::string getStatus() const { return status; }

    friend std::ostream& operator<<(std::ostream& os, const Workstation& ws);
};

class SpecialWorkstation : public Workstation {
private:
    int performanceRating;

public:
    SpecialWorkstation(int _id, const std::string& _name, const std::string& _status, int _rating);
    void display() const override;
};

std::ostream& operator<<(std::ostream& os, const Workstation& ws);

#endif // WORKSTATION_H
