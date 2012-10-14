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
#include <cstdio>
#include <fstream>
#include <getopt.h>
#include <iostream>
#include <signal.h>
#include <string>
#include <unistd.h>
#include <sys/resource.h>

#include "FanControl.h"
#include "Logger.h"

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

bool loggerExists()
{
	return ifstream(LOGGER);
}

void setPriority()
{
	setpriority(PRIO_PROCESS, getpid(), -10);
	if (errno != 0) {
		cerr << "Could not set priority" << endl;
	}
}

void cleanup(int signo = -1)
{
	FanControl::instance().cleanup();
	if (signo != SIGINT) {
		exit(1);
	}
	else {
		exit(0);
	}
}

void cleanupDaemon(int signo = -1)
{
	remove(PID_FILE);
	cleanup(signo);
}

void registerSignals(void (*cleanupHandler)(int))
{
	signal(SIGHUP, cleanupHandler);
	signal(SIGINT, cleanupHandler);
	signal(SIGABRT, cleanupHandler);
	signal(SIGQUIT, cleanupHandler);
	signal(SIGSEGV, cleanupHandler);
	signal(SIGTERM, cleanupHandler);
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

	if (!loggerExists()) {
		cout << "Logger " << LOGGER << " not found, disabling." << endl;
		syslog = false;
	}

	if (dryRun) {
		cout << argv[0] << ": Dry run, will not change fan state." << endl;
		quiet = false;
		daemonize = false;
	}

	FanControl::instance().setDryRun(dryRun);
	FanControl::instance().setQuiet(quiet);
	FanControl::instance().setSyslog(syslog);
	Logger::instance().setSyslog(syslog);
	Logger::instance().setQuiet(quiet);

	if (killDaemon || suspendDaemon) {
		ifstream pidFile(PID_FILE);
		if (!pidFile) {
			exit(0);
		}
		int pid;
		pidFile >> pid;

		if (killDaemon) {
			kill(pid, SIGINT);
		}
		else {
			kill(pid, SIGUSR1);
		}
	}
	else if (daemonize) {
		if (ifstream(PID_FILE)) {
			cout << argv[0] << ": File " << PID_FILE <<" already exists, refusing to run." << endl;
			exit(1);
		}
		else {
			ofstream pidFile(PID_FILE);
			if (!pidFile) {
				cerr << "Could not open " << PID_FILE << endl;
				exit(1);
			}
			int pid = fork();
			if (pid < 0) {
				cerr << "Could not daemonize." << endl;
				pidFile.close();
				exit(1);
			}

			if (pid > 0) {
				pidFile << pid;
				pidFile.close();
				exit(0);
			}
			pidFile.close();

			// Uzatvorenie file descriptorov
			close(STDIN_FILENO);
			close(STDOUT_FILENO);
			close(STDERR_FILENO);

			if (!dryRun) {
				setPriority();
			}
			registerSignals(cleanupDaemon);
			FanControl::instance().control();
		}
	}
	else {
		if (ifstream(PID_FILE)) {
			cout << argv[0] << ": WARNING: daemon already running" << endl;
		}
		if (!dryRun) {
			setPriority();
		}
		registerSignals(cleanup);
		FanControl::instance().control();
	}

	return 0;
}

