/*
 * =====================================================================
 *        Version:  1.0
 *        Created:  14.10.2012 15:03:39
 *         Author:  Miroslav Bendík
 *        Company:  LinuxOS.sk
 * =====================================================================
 */

#ifndef LOGGER_H_LHMFJF1H
#define LOGGER_H_LHMFJF1H

#define LOGGER "/usr/bin/logger"

#include <string>

class Logger
{
private:
	Logger();

public:
	~Logger();
	static Logger &instance();
	void log(const std::string &message);

	void setSyslog(bool syslog);
	void setQuiet(bool quiet);

private:
	bool m_syslog;
	bool m_quiet;
}; /* -----  end of class Logger  ----- */

#endif /* end of include guard: LOGGER_H_LHMFJF1H */

