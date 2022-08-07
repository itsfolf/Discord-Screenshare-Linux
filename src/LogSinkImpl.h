#include "rtc_base/logging.h"
#include <fstream>

struct FilePath {
	std::string data;
};

class LogSinkImpl final : public rtc::LogSink {
public:
	LogSinkImpl(const FilePath &logPath);

	void OnLogMessage(const std::string &msg, rtc::LoggingSeverity severity, const char *tag) override;
	void OnLogMessage(const std::string &message, rtc::LoggingSeverity severity) override;
	void OnLogMessage(const std::string &message) override;

	std::string result() const {
		return _data.str();
	}

private:
	std::ofstream _file;
	std::ostringstream _data;

};


