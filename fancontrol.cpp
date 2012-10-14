/*
 tp-fancontrol 0.3.02 (http://thinkwiki.org/wiki/ACPI_fan_control_script)
 Provided under the GNU General Public License version 2 or later or
 the GNU Free Documentation License version 1.2 or later, at your option.
 See http://www.gnu.org/copyleft/gpl.html for the Warranty Disclaimer.

 This program dynamically controls fan speed on some ThinkPad models
 according to user-defined temperature thresholds.  It implements its
 own decision algorithm, overriding the ThinkPad embedded
 controller. It also implements a workaround for the fan noise pulse
 experienced every few seconds on some ThinkPads.

 Run 'tp-fancontrol --help' for options.

 For optimal fan behavior during suspend and resume, invoke
 "tp-fancontrol -u" during the suspend process.

 WARNING: This script relies on undocumented hardware features and
 overrides nominal hardware behavior. It may thus cause arbitrary
 damage to your laptop or data. Watch your temperatures!
*/

#include <cstdlib>
#include <getopt.h>
#include <iostream>
#include <string>

#define PID_FILE "/var/run/tp-fancontrol.pid"

using namespace std;

void usage(const char *progName)
{
	cout << "Usage: " << progName << " [OPTION]..." << endl << endl;
	cout << "Available options:\n\
   -s N   Shift up the min temperature thresholds by N degrees\n\
          (positive for quieter, negative for cooler).\n\
          Max temperature thresholds are not affected.\n\
   -S N   Shift up the max temperature thresholds by N degrees\n\
          (positive for quieter, negative for cooler). DANGEROUS.\n\
   -t     Test mode\n\
   -q     Quiet mode\n\
   -d     Daemon mode, go into background (implies -q)\n\
   -l     Log to syslog\n\
   -k     Kill already-running daemon\n\
   -u     Tell already-running daemon that the system is being suspended\n\
   -p     Pid file location for daemon mode, default: " << PID_FILE << endl;
}

int main(int argc, char *argv[])
{
	string minThreshShift;
	string maxThreshShift;
	bool dryRun = false;
	bool quiet = false;
	bool daemonize = false;
	bool syslog = false;
	bool killDaemon = false;
	bool suspendDaemon = false;
	string pidFile = PID_FILE;

	int c = 0;
	while ((c = getopt(argc, argv, "s:S:qtdlp:kuh")) != -1) {
		switch (c) {
			case 's':
				minThreshShift = optarg;
				break;
			case 'S':
				maxThreshShift = optarg;
				break;
			case 't':
				dryRun = true;
				break;
			case 'q':
				quiet = true;
				break;
			case 'd':
				daemonize = true;
				break;
			case 'l':
				syslog = true;
				break;
			case 'k':
				killDaemon = true;
				break;
			case 'u':
				suspendDaemon = true;
				break;
			case 'p':
				pidFile = optarg;
				break;
			case 'h':
				usage(argv[0]);
				exit(0);
				break;
			case '?':
				usage(argv[0]);
				exit(0);
				break;
			default:
				usage(argv[0]);
				exit(-1);
		}
	}
	return 0;
}
