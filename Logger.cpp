/*
 * =====================================================================
 *        Version:  1.0
 *        Created:  14.10.2012 15:03:36
 *         Author:  Miroslav Bendík
 *        Company:  LinuxOS.sk
 * =====================================================================
 */

#include <iostream>
#include <sstream>
#include <unistd.h>
#include <wait.h>
#include "Logger.h"

using namespace std;

Logger::Logger():
	m_syslog(false),
	m_quiet(false)
{
}

Logger::~Logger()
{
}

Logger &Logger::instance()
{
	static Logger inst;
	return inst;
}

void Logger::log(const std::string &message)
{
	if (!m_quiet) {
		cout << message << endl;
	}
	if (m_syslog) {
		int pid = fork();
		if (pid == 0) {
			ostringstream commandStream;
			commandStream << "ibm-fancontrold[" << getpid() << "]";
			string command = commandStream.str();
			execl(LOGGER, LOGGER, "-t", command.c_str(), message.c_str(), (char *)0);
		}
		else {
			waitpid(pid, 0, 0);
		}
	}
}

void Logger::setSyslog(bool syslog)
{
	m_syslog = syslog;
}

void Logger::setQuiet(bool quiet)
{
	m_quiet = quiet;
}

