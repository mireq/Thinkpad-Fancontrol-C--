/*
 * =====================================================================
 *        Version:  1.0
 *        Created:  14.10.2012 10:41:53
 *         Author:  Miroslav Bendík
 *        Company:  LinuxOS.sk
 * =====================================================================
 */

#ifndef FANCONTROL_H_BPJQMR65
#define FANCONTROL_H_BPJQMR65

#include <string>

class FanControl
{
private:
	FanControl();

public:
	~FanControl();
	static FanControl &instance();

	void setDryRun(bool dryRun);
	void setQuiet(bool quiet);
	void setSyslog(bool syslog);

	void control();
	void cleanup();

private:
	bool sendIbmCommand(const std::string &device, const std::string &command);
	int readDiskTemp(const std::string &device);
	bool checkModelPattern(const std::string &model, const std::string &pattern);
	int readHitachiTemp(const std::string dev);

private:
	bool m_dryRun;
	bool m_quiet;
	bool m_syslog;

	int m_hddTemp;
}; /* -----  end of class FanControl  ----- */

#endif /* end of include guard: FANCONTROL_H_BPJQMR65 */

