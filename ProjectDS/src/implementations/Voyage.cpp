#include "../headers/Voyage.h"
Voyage::Voyage(Port* orig, Port* dest, string d, string depTime, string arrTime, float c, string company) {
    origin = orig;
    destination = dest;
    date = d;
    departureTime = depTime;
    arrivalTime = arrTime;
    cost = c;
    shippingCompany = company;
}
Voyage::Voyage() {
    origin = nullptr;
    destination = nullptr;
    date = "";
    departureTime = "";
    arrivalTime = "";
    cost = 0.0;
    shippingCompany = "";
}
   
Port* Voyage::getOrigin() const {
    return origin;      
}
Port* Voyage::getDestination() const {
    return destination; 
}   
string Voyage::getDate() const {
    return date;        
}
string Voyage::getDepartureTime() const {
    return departureTime; 
}
string Voyage::getArrivalTime() const {
    return arrivalTime;   
}
float Voyage::getCost() const {
    return cost;        
}
string Voyage::getShippingCompany() const {
    return shippingCompany; 
}
void Voyage::setOrigin(Port* orig) {
    origin = orig;      
}
void Voyage::setDestination(Port* dest) {
    destination = dest; 
}
void Voyage::setDate(string d) {
    date = d;          
}
void Voyage::setDepartureTime(string time) {
    departureTime = time; 
}
void Voyage::setArrivalTime(string time) {
    arrivalTime = time;   
}
void Voyage::setCost(float c) {
    cost = c;          
}
void Voyage::setShippingCompany(string company) {
    shippingCompany = company; 
}
