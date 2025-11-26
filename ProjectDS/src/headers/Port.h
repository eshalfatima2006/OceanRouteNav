#ifndef PORT_H
#define PORT_H

#include <string>
using namespace std;

class Port {
private:
    string name;
    float dailyCharge;

public:
    Port(string n, float charge);
    Port();

    string getName() const;
    float getDailyCharge() const;

    void setName(string n);
    void setDailyCharge(float charge);
};

#endif