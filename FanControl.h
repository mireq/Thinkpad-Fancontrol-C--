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

class FanControl
{
private:
	FanControl();

public:
	~FanControl();
	static FanControl &instance();
	void control();
	void cleanup();

private:
	bool sendIbmCommand(const char *device, const char *command);
}; /* -----  end of class FanControl  ----- */

#endif /* end of include guard: FANCONTROL_H_BPJQMR65 */

