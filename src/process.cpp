#include <unistd.h>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

#include "process.h"
#include "linux_parser.h"

using std::string;
using std::to_string;
using std::vector;

Process::Process(int pid) : pid_(pid) {}

int Process::Pid() { return pid_; }

float Process::CpuUtilization() {
  long uptime = LinuxParser::UpTime();
  long starttime = uptime - LinuxParser::UpTime(pid_);
  long active = LinuxParser::ActiveJiffies(pid_);
  long seconds = starttime > 0 ? starttime : 1;
  return static_cast<float>(active) / sysconf(_SC_CLK_TCK) / static_cast<float>(seconds);
}

string Process::Command() { return LinuxParser::Command(pid_); }

string Process::Ram() { return LinuxParser::Ram(pid_); }

string Process::User() { return LinuxParser::User(pid_); }

long int Process::UpTime() { return LinuxParser::UpTime(pid_); }

bool Process::operator<(Process const& a) const {
  return this->cpuUtilization_ > a.cpuUtilization_;
}
