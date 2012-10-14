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
#include <vector>
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
#define HDAPS_TEMP "/sys/bus/platform/drivers/hdaps/hdaps/temp1"
#define SUSPEND_TIME 60
#define OFF_THRESH_DELTA 3
#define MIN_WAIT 180
#define WAIT_COUNT (MIN_WAIT / INTERVAL)
#define LEVEL_COUNT 4

using namespace std;

/*
 WARNING: The list of temperature ranges used below is much more liberal
 than the rules used by the embedded controller firmware, and is
 derived mostly from anecdotal evidence, hunches and wishful thinking.
 It is also model-specific (see http://thinkwiki.org/wiki/Thermal_sensors).

 Temperature ranges, per sensor:
 (min temperature: when to step up from 0-th fan level,
  max temperature: when to step up to maximum fan level)
*/

const list<int> THRESHOLDS({
             //  Sensor     ThinkPad model
             //             R51     T41/2  Z60t   T43-26xx
//min  max   //  ---------- ------- -----  -----  ---------------------------
  50,  70,   //  EC 0x78    CPU     CPU    ?      CPU
  47,  60,   //  EC 0x79    miniPCI ?      ?      Between CPU and PCMCIA slot
  43,  55,   //  EC 0x7A    HDD     ?      ?      PCMCIA slot
  49,  68,   //  EC 0x7B    GPU     GPU    ?      GPU
  40,  50,   //  EC 0x7C    BAT     BAT    BAT    Sys BAT (front left of battery)
  40,  50,   //  EC 0x7D    n/a     n/a    n/a    UltraBay BAT
  37,  47,   //  EC 0x7E    BAT     BAT    BAT    Sys BAT (rear right of battery)
  37,  47,   //  EC 0x7F    n/a     n/a    n/a    UltraBay BAT

  45,  60,   //  EC 0xC0    ?       n/a    ?      Between northbridge and DRAM
  48,  62,   //  EC 0xC1    ?       n/a    ?      Southbridge (under miniPCI)
  50,  65,   //  EC 0xC2    ?       n/a    ?      Power circuitry (under CDC)

  47,  58,   //  HDD        ->      ->     ->     Hard disk internal sensor
  47,  60,   //  HDAPS      ->      ->     ->     HDAPS readout (same as EC 0x79)
});

const vector<int> LEVELS({0, 2, 4, 7});

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
	int idx = 0;
	int newIdx = 0;
	int startTime = 0;
	int maxIdx = LEVEL_COUNT - 1;
	int oldLevel = 0;
	bool first = true;
	while(1) {
		if (cycle % DISK_POOL_INTERVAL_COUNT == 0) {
			m_hddTemp = readDiskTemp("hda");
			if (m_hddTemp == -128) {
				m_hddTemp = readDiskTemp("sda");
			}
		}
		istringstream temperaturesReadStream(readIbmProperty("thermal", "temperatures"));
		list<int> temperatures;
		for (int i = 0; i < 11; ++i) {
			int temp;
			temperaturesReadStream >> temp;
			temperatures.push_back(temp);
		}
		temperatures.push_back(m_hddTemp);
		temperatures.push_back(readHdapsTemp());

		int now = time(0);
		int maxZ = (idx > 0 ? (now > startTime + MIN_WAIT ? 2 * (idx - 1) : 2 * idx) : 0 );
		auto thresholdIt = THRESHOLDS.begin();
		for (auto tempIt = temperatures.begin(); tempIt != temperatures.end(); ++tempIt) {
			int min = *thresholdIt;
			++thresholdIt;
			int max = *thresholdIt;
			++thresholdIt;
			if (*tempIt == 128 || *tempIt == -128) {
				continue;
			}

			int z = 0;
			if (*tempIt < min - OFF_THRESH_DELTA) {
				z = 0;
			}
			else {
				z = ( (2 * (*tempIt - min) * (maxIdx - 1)) / (max - min)) + 2;
				if (z < 1) {
					z = 1;
				}
				if (z > 2 * maxIdx) {
					z = 2 * maxIdx;
				}
				if (z > maxZ) {
					maxZ = z;
				}
			}
		}

		int hys = (maxZ == 2 * idx - 1);
		if (!hys) {
			maxZ++;
		}
		newIdx = maxZ / 2;

		if (!first) {
			oldLevel = LEVELS[idx];
		}
		int newLevel = LEVELS[newIdx];
		if (oldLevel != newLevel) {
			startTime = time(0);
		}

		setLevel(newLevel);

		sleep(INTERVAL);
		++cycle;
		first = false;
		idx = newIdx;
	}
}

void FanControl::cleanup()
{
	sendIbmCommand("fan", "enable");
	sendIbmCommand("fan", "watchdog 0");
}

void FanControl::suspend()
{
	setLevel(0);
	sleep(SUSPEND_TIME);
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

std::string FanControl::readIbmProperty(const std::string &device, const std::string &property)
{
	string search = property + ":";
	string fileName = string(IBM_ACPI) + "/" + device;
	ifstream dev(fileName.c_str());
	string line;
	while (dev.good()) {
		getline(dev, line);
		if (line.length()) {
			line.erase(line.length() - 1);
		}
		if (line.find_first_of(search) == 0) {
			line = line.substr(search.length());
			while (line.length() && (line[0] == ' ' || line[0] == '\t')) {
				line = line.substr(1);
			}
			return line;
		}
	}
	return string();
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

int FanControl::readHdapsTemp()
{
	ifstream in(HDAPS_TEMP);
	if (!in) {
		return -128;
	}
	int temp;
	in >> temp;
	return temp;
}

void FanControl::setLevel(int level)
{
	ostringstream level_command;
	level_command << "level " << level;
	string command = level_command.str();
	sendIbmCommand("fan", command.c_str());
}

