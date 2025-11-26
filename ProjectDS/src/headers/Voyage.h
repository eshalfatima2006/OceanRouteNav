#ifndef VOYAGE_H
#define VOYAGE_H
#include <string>
#include "Port.h"
using namespace std;

class Voyage {
private:
    Port* origin;
    Port* destination;
    string date;
    string departureTime;
    string arrivalTime;
    float cost;
    string shippingCompany;

public:
    Voyage(Port* orig , Port* dest, string d , string depTime, string arrTime , float c , string company);
    Voyage();
    Port* getOrigin() const;
    Port* getDestination() const;
    string getDate() const;
    string getDepartureTime() const;
    string getArrivalTime() const;
    float getCost() const;
    string getShippingCompany() const;

    void setOrigin(Port* orig);
    void setDestination(Port* dest);
    void setDate(string d);
    void setDepartureTime(string time);
    void setArrivalTime(string time);
    void setCost(float c);
    void setShippingCompany(string company);
};

#endif