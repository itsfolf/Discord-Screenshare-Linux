#pragma once

#include "rtc_base/thread.h"

namespace LinuxFix
{
    rtc::Thread *GlobalLoopThread();
    rtc::Thread *GlobalCaptureThread();
}