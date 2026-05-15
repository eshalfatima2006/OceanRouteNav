#include "DataLoader.h"
#include "RouteUtilities.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>

using namespace std;

bool load_port_charges(Graph& g, const string& filepath) {
    ifstream in(filepath.c_str());
    if (!in.is_open()) {
        cerr << "Failed to open PortCharges file: " << filepath << "\n";
        return false;
    }
    string line;
    while (getline(in, line)) {
        if (line.empty()) continue;
        istringstream ss(line);
        string pname;
        int charge = 0;
        if (!(ss >> pname >> charge)) continue;
        int id = g.get_port_id(pname);
        if (id == -1) id = g.add_port(pname);
        PortNode* p = g.get_port_by_id(id);
        if (p) p->portCharge = charge;
    }
    in.close();
    return true;
}

bool load_routes(Graph& g, const string& filepath) {
    ifstream in(filepath.c_str());
    if (!in.is_open()) {
        cerr << "Failed to open Routes file: " << filepath << "\n";
        return false;
    }
    string line;
    while (getline(in, line)) {
        if (line.empty()) continue;

        istringstream ss(line);
        string origin, dest, date, depart, arrive, costStr;
        if (!(ss >> origin >> dest >> date >> depart >> arrive >> costStr)) continue;

        string company;
        getline(ss, company);
        if (!company.empty() && company[0] == ' ') company.erase(0, 1);

        int cost = atoi(costStr.c_str());
        int oid = g.get_port_id(origin);
        if (oid == -1) oid = g.add_port(origin);
        int did = g.get_port_id(dest);
        if (did == -1) did = g.add_port(dest);

        RouteEdge* e = new RouteEdge();
        e->destPortID = did;
        e->dateStr = date;
        e->departStr = depart;
        e->arriveStr = arrive;
        e->cost = cost;
        e->company = company;
        fillEdgeTimestamps(e);

        g.add_edge(oid, e);

        PortNode* port = g.get_port_by_id(oid);
        if (port) {
            CompanyPortStorage* storage = port->getCompanyStorage(company);
            storage->companyRoutes.push(e);
        }
    }
    in.close();
    return true;
}