#pragma once

struct alignas(32) RegressionPacket
{
	char name[4096];
	int size, w, h, pitch, bytes_per_pixel;
	u8 data[1024 * 1024 * 4];
};

extern bool regression_testing;
constexpr int n_regression_packets = 10;
extern RegressionPacket regression_packets[n_regression_packets];

RegressionPacket* GetRegressionPacket();