#include "../headers/Port.h"
Port::Port(string n, float charge) 
{
    name = n;
    dailyCharge = charge;
}
Port::Port() 
{
    name = "";
    dailyCharge = 0.0;
}
string Port::getName() const {
    return name;
}
float Port::getDailyCharge() const {
    return dailyCharge;
}
void Port::setName(string n) {
    name = n;
}
void Port::setDailyCharge(float charge) {
    dailyCharge = charge;
}
