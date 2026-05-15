#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <cctype>

using namespace std;

#define MAX_EXISTING 200
#define MAX_PORTS 1000
#define NAME_LEN 64
#define LINE_LEN 1024

// Safe C-string equality (case-sensitive as original)
static bool stringsEqual(const char* a, const char* b) {
    if (!a || !b) return false;
    while (*a && *b) {
        if (*a != *b) return false;
        ++a; ++b;
    }
    return (*a == '\0' && *b == '\0');
}

// Safe copy with guaranteed NUL termination
static void stringCopy(char* dest, const char* src, int destSize) {
    if (destSize <= 0) return;
    int i = 0;
    while (src[i] && i < destSize - 1) {
        dest[i] = src[i];
        ++i;
    }
    dest[i] = '\0';
}

// Return default charge groups (matches original behavior)
static int getDefaultCharge(const char* port) {
    if (!port) return 750;
    if (stringsEqual(port, "NewYork") || stringsEqual(port, "Singapore") ||
        stringsEqual(port, "Rotterdam") || stringsEqual(port, "Shanghai"))
        return 2000;
    if (stringsEqual(port, "HongKong") || stringsEqual(port, "Dubai") ||
        stringsEqual(port, "Busan") || stringsEqual(port, "LosAngeles"))
        return 1500;
    if (stringsEqual(port, "Tokyo") || stringsEqual(port, "Osaka") ||
        stringsEqual(port, "Sydney") || stringsEqual(port, "Melbourne"))
        return 1200;
    if (stringsEqual(port, "London") || stringsEqual(port, "Hamburg") ||
        stringsEqual(port, "Marseille") || stringsEqual(port, "Genoa"))
        return 1000;
    return 750;
}

// Find index of name in array, -1 if not found
static int findIndex(char arr[][NAME_LEN], int count, const char* name) {
    for (int i = 0; i < count; ++i) {
        if (stringsEqual(arr[i], name)) return i;
    }
    return -1;
}


int update_port_charges_tool_main() {
    ifstream routes("Routes.txt");
    if (!routes.is_open()) {
        cerr << "Failed to open Routes.txt\n";
        return 1;
    }

    ifstream existing("PortCharges.txt");
    if (!existing.is_open()) {
        cerr << "Failed to open PortCharges.txt\n";
        return 1;
    }

    ofstream updated("PortCharges_Updated.txt");
    if (!updated.is_open()) {
        cerr << "Failed to open PortCharges_Updated.txt for writing\n";
        return 1;
    }

    // Read existing charges into arrays
    char existingPorts[MAX_EXISTING][NAME_LEN];
    int existingCharges[MAX_EXISTING];
    int existingCount = 0;
    char line[LINE_LEN];

    while (existing.getline(line, LINE_LEN)) {
        // Skip empty lines and leading whitespace
        int len = (int)strlen(line);
        int pos = 0;
        while (pos < len && isspace((unsigned char)line[pos])) pos++;
        if (pos >= len) continue;

        // find first whitespace after name
        int spacePos = -1;
        for (int i = pos; i < len; ++i) {
            if (isspace((unsigned char)line[i])) { spacePos = i; break; }
        }

        if (spacePos == -1) continue; // malformed line

        // extract name safely
        int nameLen = spacePos - pos;
        if (nameLen >= NAME_LEN) nameLen = NAME_LEN - 1;
        char namebuf[NAME_LEN];
        for (int i = 0; i < nameLen; ++i) namebuf[i] = line[pos + i];
        namebuf[nameLen] = '\0';

        // parse charge (skip whitespace)
        int j = spacePos;
        while (j < len && isspace((unsigned char)line[j])) j++;
        if (j >= len) continue;
        char* endptr = nullptr;
        long charge = strtol(&line[j], &endptr, 10);
        if (&line[j] == endptr) continue; // no number parsed

        if (existingCount >= MAX_EXISTING) {
            cerr << "Warning: too many existing entries; skipping remainder\n";
            break;
        }
        stringCopy(existingPorts[existingCount], namebuf, NAME_LEN);
        existingCharges[existingCount] = (int)charge;
        ++existingCount;
    }
    existing.close();

    // Write existing entries first
    for (int i = 0; i < existingCount; ++i) {
        updated << existingPorts[i] << " " << existingCharges[i] << "\n";
    }

    // Collect ports from Routes.txt (first two words)
    char allPorts[MAX_PORTS][NAME_LEN];
    int portCount = 0;

    // Reset routes stream to start (file was opened earlier, ensure at beginning)
    routes.clear();
    routes.seekg(0);

    while (routes.getline(line, LINE_LEN)) {
        // skip empty/whitespace-only lines
        int len = (int)strlen(line);
        int pos = 0;
        while (pos < len && isspace((unsigned char)line[pos])) pos++;
        if (pos >= len) continue;

        // tokenize first two words manually (avoid strtok race issues)
        int idx = pos;
        // first word
        char word1[NAME_LEN] = { 0 };
        int w1 = 0;
        while (idx < len && !isspace((unsigned char)line[idx]) && w1 < NAME_LEN - 1) {
            word1[w1++] = line[idx++];
        }
        word1[w1] = '\0';
        // skip spaces
        while (idx < len && isspace((unsigned char)line[idx])) idx++;
        // second word
        char word2[NAME_LEN] = { 0 };
        int w2 = 0;
        while (idx < len && !isspace((unsigned char)line[idx]) && w2 < NAME_LEN - 1) {
            word2[w2++] = line[idx++];
        }
        word2[w2] = '\0';

        if (w1 > 0) {
            if (findIndex(allPorts, portCount, word1) == -1) {
                if (portCount >= MAX_PORTS) {
                    cerr << "Warning: too many ports discovered; some ports skipped\n";
                }
                else {
                    stringCopy(allPorts[portCount], word1, NAME_LEN);
                    ++portCount;
                }
            }
        }
        if (w2 > 0) {
            if (findIndex(allPorts, portCount, word2) == -1) {
                if (portCount >= MAX_PORTS) {
                    cerr << "Warning: too many ports discovered; some ports skipped\n";
                }
                else {
                    stringCopy(allPorts[portCount], word2, NAME_LEN);
                    ++portCount;
                }
            }
        }
    }
    routes.close();

    // Add missing ports with default charges
    for (int i = 0; i < portCount; ++i) {
        bool found = false;
        for (int j = 0; j < existingCount; ++j) {
            if (stringsEqual(allPorts[i], existingPorts[j])) { found = true; break; }
        }
        if (!found) {
            int charge = getDefaultCharge(allPorts[i]);
            updated << allPorts[i] << " " << charge << "\n";
            cout << "Added: " << allPorts[i] << " $" << charge << "\n";
        }
    }

    updated.close();
   
    return 0;
}

#ifdef UPDATE_PORT_CHARGES_TOOL
int main() {
    return update_port_charges_tool_main();
}
#endif