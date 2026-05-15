#include "PortLayout.h"
#include "MathUtilities.h"
#include <cstring>
#include <cctype>
#include <cmath>

using namespace std;

string toLowerStr(const string& s) {
	string out = s;
	for (int i = 0; i < (int)out.size(); ++i) {
		out[i] = tolower(static_cast<unsigned char>(out[i]));
	}
	return out;
}

// --- Function 1: Precise Port Coordinates (Calibrated from Percentage Map) ---
static PortCoordEntry* createPortCoordinates(int& count) {
	const int NUM_PORTS = 40;
	PortCoordEntry* coords = new PortCoordEntry[NUM_PORTS];

	
	const float W = 1200.0f;
	const float H = 800.0f;

	
	auto calc_x = [&](float p) { return (round(p * W)); };
	auto calc_y = [&](float p) { return (round(p * H)); };

	// ASIA / PACIFIC
	coords[0] = { "Karachi", calc_x(0.68f), calc_y(0.36f) }; // (816, 288)
	coords[1] = { "HongKong", calc_x(0.81f), calc_y(0.37f) }; // (972, 296)
	coords[2] = { "Mumbai", calc_x(0.70f), calc_y(0.39f) }; // (840, 312)
	coords[3] = { "Colombo", calc_x(0.72f), calc_y(0.46f) }; // (864, 368)
	coords[4] = { "Chittagong", calc_x(0.75f), calc_y(0.37f) }; // (900, 296)
	coords[5] = { "Jakarta", calc_x(0.79f), calc_y(0.53f) }; // (948, 424)
	coords[6] = { "Manila", calc_x(0.83f), calc_y(0.42f) }; // (996, 336)
	coords[7] = { "Singapore", calc_x(0.79f), calc_y(0.49f) }; // (948, 392)
	coords[8] = { "Shanghai", calc_x(0.84f), calc_y(0.33f) }; // (1008, 264)
	coords[9] = { "Tokyo", calc_x(0.88f), calc_y(0.30f) }; // (1056, 240)
	coords[10] = { "Osaka", calc_x(0.87f), calc_y(0.30f) }; // (1044, 240)
	coords[11] = { "Busan", calc_x(0.86f), calc_y(0.30f) }; // (1032, 240)

	// MIDDLE EAST
	coords[12] = { "Jeddah", calc_x(0.61f), calc_y(0.38f) }; // (732, 304)
	coords[13] = { "Dubai", calc_x(0.65f), calc_y(0.36f) }; // (780, 288)
	coords[14] = { "AbuDhabi", calc_x(0.65f), calc_y(0.36f) }; // (780, 288)
	coords[15] = { "Doha", calc_x(0.64f), calc_y(0.36f) }; // (768, 288)

	// EUROPE
	coords[16] = { "London", calc_x(0.50f), calc_y(0.21f) }; // (600, 168)
	coords[17] = { "Rotterdam", calc_x(0.51f), calc_y(0.21f) }; // (612, 168)
	coords[18] = { "Antwerp", calc_x(0.51f), calc_y(0.21f) }; // (612, 168)
	coords[19] = { "Hamburg", calc_x(0.53f), calc_y(0.20f) }; // (636, 160)
	coords[20] = { "Marseille", calc_x(0.51f), calc_y(0.26f) }; // (612, 208)
	coords[21] = { "Genoa", calc_x(0.52f), calc_y(0.25f) }; // (624, 200)
	coords[22] = { "Lisbon", calc_x(0.47f), calc_y(0.28f) }; // (564, 224)
	coords[23] = { "Dublin", calc_x(0.48f), calc_y(0.20f) }; // (576, 160)
	coords[24] = { "Helsinki", calc_x(0.57f), calc_y(0.16f) }; // (684, 128)
	coords[25] = { "Stockholm", calc_x(0.55f), calc_y(0.17f) }; // (660, 136)
	coords[26] = { "Oslo", calc_x(0.53f), calc_y(0.17f) }; // (636, 136)
	coords[27] = { "Copenhagen", calc_x(0.53f), calc_y(0.19f) }; // (636, 152)
	coords[28] = { "Istanbul", calc_x(0.58f), calc_y(0.27f) }; // (696, 216)
	coords[29] = { "Athens", calc_x(0.56f), calc_y(0.29f) }; // (672, 232)

	// AFRICA
	coords[30] = { "Alexandria", calc_x(0.58f), calc_y(0.33f) }; // (696, 264)
	coords[31] = { "CapeTown", calc_x(0.55f), calc_y(0.69f) }; // (660, 552)
	coords[32] = { "Durban", calc_x(0.58f), calc_y(0.66f) }; // (696, 528)
	coords[33] = { "PortLouis", calc_x(0.66f), calc_y(0.61f) }; // (792, 488)

	// AMERICAS
	coords[34] = { "NewYork", calc_x(0.29f), calc_y(0.27f) }; // (348, 216)
	coords[35] = { "Montreal", calc_x(0.29f), calc_y(0.25f) }; // (348, 200)
	coords[36] = { "Vancouver", calc_x(0.16f), calc_y(0.22f) }; // (192, 176)
	coords[37] = { "LosAngeles", calc_x(0.17f), calc_y(0.31f) }; // (204, 248)

	// OCEANIA
	coords[38] = { "Sydney", calc_x(0.92f), calc_y(0.69f) }; // (1104, 552)
	coords[39] = { "Melbourne", calc_x(0.90f), calc_y(0.71f) }; // (1080, 568)

	count = NUM_PORTS;
	return coords;
}

// --- Function 2: Assign Port Positions (Relaxation Disabled) ---
void assign_port_positions(Graph& g, int width, int height) {
	int coordCount = 0;
	PortCoordEntry* coordMap = createPortCoordinates(coordCount);

	int n = g.portsArr.size();
	if (n == 0) { delete[] coordMap; return; }

	// Initialize positions (uses the precise coordinates from above)
	for (int i = 0; i < n; ++i) {
		PortNode* p = g.portsArr[i];
		bool found = false;

		// 1. Match by exact name
		for (int j = 0; j < coordCount; j++) {
			if (strcmp(p->name.c_str(), coordMap[j].name) == 0) {
				p->x = coordMap[j].x;
				p->y = coordMap[j].y;
				found = true;
				break;
			}
		}

		// 2. Match by lowercase name
		if (!found) {
			string lowerName = toLowerStr(p->name);
			for (int j = 0; j < coordCount; j++) {
				string lowerMapName = toLowerStr(coordMap[j].name);
				if (lowerName == lowerMapName) {
					p->x = coordMap[j].x;
					p->y = coordMap[j].y;
					found = true;
					break;
				}
			}
		}

		// 3. If port not found, assign a generic grid position
		if (!found) {
			int cols = maxi(4, width / 160);
			int row = i / cols;
			int col = i % cols;
			float margin = 60.0f;
			float usableW = (float)width - margin * 2.0f;
			float usableH = (float)height - margin * 2.0f;
			p->x = margin + (usableW * (0.15f + 0.7f * (col + 0.5f) / cols));
			p->y = margin + (usableH * (0.08f + 0.84f * (row + 0.5f) / maxi(1, (n + cols - 1) / cols)));
		}
	}

	// Boundary Clamping
	for (int i = 0; i < n; ++i) {
		PortNode* p = g.portsArr[i];
		if (!p) continue;
		if (p->x < 40.0f) p->x = 40.0f;
		if (p->y < 40.0f) p->y = 40.0f;
		if (p->x > width - 40.0f) p->x = width - 40.0f;
		if (p->y > height - 40.0f) p->y = height - 40.0f;
	}

	delete[] coordMap;
}