#include "LogSinkImpl.h"
#include <sys/time.h>

LogSinkImpl::LogSinkImpl(const FilePath &logPath) {
	if (!logPath.data.empty()) {
		_file.open(logPath.data);
	}
}

void LogSinkImpl::OnLogMessage(const std::string &msg, rtc::LoggingSeverity severity, const char *tag) {
	OnLogMessage(std::string(tag) + ": " + msg);
}

void LogSinkImpl::OnLogMessage(const std::string &message, rtc::LoggingSeverity severity) {
	OnLogMessage(message);
}

void LogSinkImpl::OnLogMessage(const std::string &message) {
	time_t rawTime;
	time(&rawTime);
	struct tm timeinfo;

	timeval curTime = { 0 };
	localtime_r(&rawTime, &timeinfo);
	gettimeofday(&curTime, nullptr);
	int32_t milliseconds = curTime.tv_usec / 1000;

	auto &stream = _file.is_open() ? (std::ostream&)_file : _data;
	stream
		<< (timeinfo.tm_year + 1900)
		<< "-" << (timeinfo.tm_mon + 1)
		<< "-" << (timeinfo.tm_mday)
		<< " " << timeinfo.tm_hour
		<< ":" << timeinfo.tm_min
		<< ":" << timeinfo.tm_sec
		<< ":" << milliseconds
		<< " " << message;

//#if DEBUG
    printf("%d-%d-%d %d:%d:%d:%d %s\n", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, milliseconds, message.c_str());
//#endif
}
