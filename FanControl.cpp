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
#include <sstream>
#include <unistd.h>
#include "FanControl.h"

#define IBM_ACPI "/proc/acpi/ibm"
#define INTERVAL 3
#define WATCHDOG_DELAY (3 * INTERVAL)

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
	ostringstream watchdog_command;
	watchdog_command << "watchdog " << WATCHDOG_DELAY;
	string command = watchdog_command.str();
	sendIbmCommand("fan", command.c_str());
	while(1) {
		sleep(INTERVAL);
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

