/*
 * =====================================================================
 *        Version:  1.0
 *        Created:  14.10.2012 10:41:49
 *         Author:  Miroslav Bendík
 *        Company:  LinuxOS.sk
 * =====================================================================
 */

#include <fstream>
#include <iostream>
#include <unistd.h>
#include "FanControl.h"

#define IBM_ACPI "/proc/acpi/ibm"

using namespace std;

FanControl::FanControl()
{
}

FanControl::~FanControl()
{
}

FanControl &FanControl::instance()
{
	static FanControl inst;
	return inst;
}

void FanControl::control()
{
	while(1) {
		sleep(2);
	}
}

void FanControl::cleanup()
{
	sendIbmCommand("fan", "enable");
	sendIbmCommand("fan", "watchdog 0");
}

bool FanControl::sendIbmCommand(const char *device, const char *command)
{
	string fileName = string(IBM_ACPI) + "/" + device;
	ofstream dev(fileName.c_str());
	if (!dev) {
		cerr << "Could not open device " << device << endl;
		return false;
	}
	dev << command;
	if (dev.fail()) {
		cerr << "Could not write " << command << " to device " << device << endl;
		return false;
	}
	return true;
}

