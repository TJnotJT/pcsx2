#include "GSRegressionTester.h"

bool regression_testing = true;
RegressionPacket regression_packets[n_regression_packets];

RegressionPacket* GetRegressionPacket()
{
	// Need to implement a ring buffer;
	static int i = 0;
	if (!regression_testing)
		return nullptr;
	i %= n_regression_packets;
	return &regression_packets[i++];
}