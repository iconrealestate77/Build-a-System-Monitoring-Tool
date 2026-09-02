#include <dirent.h>
#include <unistd.h>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

#include "linux_parser.h"

using std::stof;
using std::string;
using std::to_string;
using std::vector;

string LinuxParser::OperatingSystem() {
  string line;
  string key;
  string value;
  std::ifstream filestream(kOSPath);
  if (filestream.is_open()) {
    while (std::getline(filestream, line)) {
      std::replace(line.begin(), line.end(), ' ', '_');
      std::replace(line.begin(), line.end(), '=', ' ');
      std::replace(line.begin(), line.end(), '"', ' ');
      std::istringstream linestream(line);
      while (linestream >> key >> value) {
        if (key == "PRETTY_NAME") {
          std::replace(value.begin(), value.end(), '_', ' ');
          return value;
        }
      }
    }
  }
  return value;
}

string LinuxParser::Kernel() {
  string os, kernel, version;
  string line;
  std::ifstream stream(kProcDirectory + kVersionFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    linestream >> os >> version >> kernel;
  }
  return kernel;
}

vector<int> LinuxParser::Pids() {
  vector<int> pids;
  DIR* directory = opendir(kProcDirectory.c_str());
  struct dirent* file;
  while ((file = readdir(directory)) != nullptr) {
    if (file->d_type == DT_DIR) {
      string filename(file->d_name);
      if (std::all_of(filename.begin(), filename.end(), isdigit)) {
        int pid = stoi(filename);
        pids.push_back(pid);
      }
    }
  }
  closedir(directory);
  return pids;
}

float LinuxParser::MemoryUtilization() {
  string line, key, unit;
  long value;
  long memTotal = 0, memFree = 0;
  std::ifstream stream(kProcDirectory + kMeminfoFilename);
  if (stream.is_open()) {
    while (std::getline(stream, line)) {
      std::istringstream linestream(line);
      linestream >> key >> value >> unit;
      if (key == "MemTotal:") memTotal = value;
      else if (key == "MemFree:") memFree = value;
    }
  }
  if (memTotal == 0) return 0.0;
  return static_cast<float>(memTotal - memFree) / static_cast<float>(memTotal);
}

long LinuxParser::UpTime() {
  string line;
  double uptime = 0;
  std::ifstream stream(kProcDirectory + kUptimeFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    linestream >> uptime;
  }
  return static_cast<long>(uptime);
}

vector<string> LinuxParser::CpuUtilization() {
  vector<string> values;
  string line, cpu, value;
  std::ifstream stream(kProcDirectory + kStatFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    linestream >> cpu;
    while (linestream >> value) values.push_back(value);
  }
  return values;
}

long LinuxParser::Jiffies() { return ActiveJiffies() + IdleJiffies(); }

long LinuxParser::ActiveJiffies() {
  vector<string> cpu = CpuUtilization();
  if (cpu.size() < 10) return 0;
  long user = stol(cpu[0]), nice = stol(cpu[1]), system = stol(cpu[2]);
  long irq = stol(cpu[5]), softirq = stol(cpu[6]), steal = stol(cpu[7]);
  return user + nice + system + irq + softirq + steal;
}

long LinuxParser::IdleJiffies() {
  vector<string> cpu = CpuUtilization();
  if (cpu.size() < 10) return 0;
  long idle = stol(cpu[3]), iowait = stol(cpu[4]);
  return idle + iowait;
}

long LinuxParser::ActiveJiffies(int pid) {
  string line, value;
  vector<string> fields;
  std::ifstream stream(kProcDirectory + to_string(pid) + kStatFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    while (linestream >> value) fields.push_back(value);
  }
  if (fields.size() < 17) return 0;
  long utime = stol(fields[13]), stime = stol(fields[14]);
  long cutime = stol(fields[15]), cstime = stol(fields[16]);
  return utime + stime + cutime + cstime;
}

int LinuxParser::TotalProcesses() {
  string line, key;
  int value;
  std::ifstream stream(kProcDirectory + kStatFilename);
  if (stream.is_open()) {
    while (std::getline(stream, line)) {
      std::istringstream linestream(line);
      linestream >> key >> value;
      if (key == "processes") return value;
    }
  }
  return 0;
}

int LinuxParser::RunningProcesses() {
  string line, key;
  int value;
  std::ifstream stream(kProcDirectory + kStatFilename);
  if (stream.is_open()) {
    while (std::getline(stream, line)) {
      std::istringstream linestream(line);
      linestream >> key >> value;
      if (key == "procs_running") return value;
    }
  }
  return 0;
}

string LinuxParser::Command(int pid) {
  string line;
  std::ifstream stream(kProcDirectory + to_string(pid) + kCmdlineFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
  }
  return line;
}

string LinuxParser::Ram(int pid) {
  string line, key, value;
  std::ifstream stream(kProcDirectory + to_string(pid) + kStatusFilename);
  if (stream.is_open()) {
    while (std::getline(stream, line)) {
      std::istringstream linestream(line);
      linestream >> key >> value;
      if (key == "VmSize:") {
        long mb = stol(value) / 1024;
        return to_string(mb);
      }
    }
  }
  return string("0");
}

string LinuxParser::Uid(int pid) {
  string line, key, value;
  std::ifstream stream(kProcDirectory + to_string(pid) + kStatusFilename);
  if (stream.is_open()) {
    while (std::getline(stream, line)) {
      std::istringstream linestream(line);
      linestream >> key >> value;
      if (key == "Uid:") return value;
    }
  }
  return string();
}

string LinuxParser::User(int pid) {
  string uid = Uid(pid);
  string line, user, x, id;
  std::ifstream stream(kPasswordPath);
  if (stream.is_open()) {
    while (std::getline(stream, line)) {
      std::replace(line.begin(), line.end(), ':', ' ');
      std::istringstream linestream(line);
      linestream >> user >> x >> id;
      if (id == uid) return user;
    }
  }
  return string();
}

long LinuxParser::UpTime(int pid) {
  string line, value;
  vector<string> fields;
  std::ifstream stream(kProcDirectory + to_string(pid) + kStatFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    while (linestream >> value) fields.push_back(value);
  }
  if (fields.size() < 22) return 0;
  long starttime = stol(fields[21]) / sysconf(_SC_CLK_TCK);
  return UpTime() - starttime;
}
