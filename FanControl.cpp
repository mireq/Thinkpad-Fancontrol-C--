/*
 * =====================================================================
 *        Version:  1.0
 *        Created:  14.10.2012 10:41:49
 *         Author:  Miroslav Bendík
 *        Company:  LinuxOS.sk
 * =====================================================================
 */

#include <algorithm>
#include <cstdio>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <list>
#include <sstream>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include "FanControl.h"
#include "Logger.h"

#define IBM_ACPI "/proc/acpi/ibm"
#define INTERVAL 3
#define WATCHDOG_DELAY (3 * INTERVAL)
#define DISK_POOL_PERIOD 24
#define DISK_POOL_INTERVAL_COUNT (DISK_POOL_PERIOD / INTERVAL)
#define HITACHI_MODELS {"HTS4212..H9AT00", "HTS726060M9AT00", "HTS5410..G9AT00", "IC25N0..ATCS04", "IC25N0..ATCS05", "IC25T0..ATCS04", "IC25T0..ATCS05", "HTE541040G9AT00", "HTS5416..J9AT00", "HTS5416..J9SA00", "HTS54161"}

using namespace std;

FanControl::FanControl():
	m_dryRun(false),
	m_quiet(false),
	m_syslog(false),
	m_hddTemp(-128)
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

void FanControl::setDryRun(bool dryRun)
{
	m_dryRun = dryRun;
}

void FanControl::setQuiet(bool quiet)
{
	m_quiet = quiet;
}

void FanControl::setSyslog(bool syslog)
{
	m_syslog = syslog;
}

void FanControl::control()
{
	ostringstream watchdog_command;
	watchdog_command << "watchdog " << WATCHDOG_DELAY;
	string command = watchdog_command.str();
	sendIbmCommand("fan", command.c_str());

	Logger::instance().log("Starting dynamic fan control");

	int cycle = 0;
	while(1) {
		if (cycle % DISK_POOL_INTERVAL_COUNT == 0) {
			m_hddTemp = readDiskTemp("hda");
			if (m_hddTemp == -128) {
				m_hddTemp = readDiskTemp("sda");
			}
		}
		++cycle;
		sleep(INTERVAL);
	}
}

void FanControl::cleanup()
{
	sendIbmCommand("fan", "enable");
	sendIbmCommand("fan", "watchdog 0");
}

bool FanControl::sendIbmCommand(const std::string &device, const std::string &command)
{
	if (m_dryRun) {
		cout << "Command: " << device << ": " << command << endl;
		return true;
	}
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

int FanControl::readDiskTemp(const std::string &device)
{
	string deviceFileName = string("/dev/") + device;
	string modelFileName = string("/sys/block/") + device + string("/device/model");
	ifstream modelFile(modelFileName.c_str());
	if (!modelFile) {
		return -128;
	}

	string model((istreambuf_iterator<char>(modelFile)), istreambuf_iterator<char>());
	model.erase(remove(model.begin(), model.end(), '\n'), model.end());

	size_t pos = model.find_last_of(' ');
	if (pos != string::npos) {
		model = model.substr(pos + 1);
	}

	bool isHitachi = false;
	list<string> patterns(HITACHI_MODELS);
	for (auto patternIt = patterns.begin(); patternIt != patterns.end(); ++patternIt) {
		if (checkModelPattern(model, *patternIt)) {
			isHitachi = true;
			break;
		}
	}
	if (isHitachi) {
		return readHitachiTemp(deviceFileName);
	}

	return -128;
}

bool FanControl::checkModelPattern(const std::string &model, const std::string &pattern)
{
	if (model.length() != pattern.length()) {
		return false;
	}

	for (size_t i = 0; i < model.length(); ++i) {
		if (pattern[i] == '.') {
			continue;
		}
		if (model[i] != pattern[i]) {
			return false;
		}
	}
	return true;
}

int FanControl::readHitachiTemp(const std::string dev)
{
	int fd = open(dev.c_str(), O_RDONLY | O_NONBLOCK);
	if (fd < 0) {
		return -128;
	}
	unsigned char data[] = {0xf0, 0x00, 0x01, 0x00};
	if (ioctl(fd, 0x031f, data) >= 0) {
		close(fd);
		// underflow / overflow
		if (data[2] == 0x00 || data[2] == 0xff) {
			return -128;
		}
		if (data[2] == 0x01) { // Read not supported
			return -128;
		}
		return data[2] / 2 - 20;
	}
	else {
		close(fd);
		return -128;
	}
}

