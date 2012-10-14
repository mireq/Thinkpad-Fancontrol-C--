/*
 * =====================================================================
 *        Version:  1.0
 *        Created:  14.10.2012 10:41:49
 *         Author:  Miroslav Bendík
 *        Company:  LinuxOS.sk
 * =====================================================================
 */

#include <unistd.h>
#include "FanControl.h"

FanControl::FanControl()
{
}

FanControl::~FanControl()
{
}

void FanControl::control()
{
	while(1) {
		sleep(2);
	}
}

