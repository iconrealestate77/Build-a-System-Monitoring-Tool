#include "processor.h"
#include "linux_parser.h"

float Processor::Utilization() {
  static long prevActive = 0, prevIdle = 0;
  long active = LinuxParser::ActiveJiffies();
  long idle = LinuxParser::IdleJiffies();
  long totalDelta = (active + idle) - (prevActive + prevIdle);
  long activeDelta = active - prevActive;
  prevActive = active;
  prevIdle = idle;
  if (totalDelta <= 0) return 0.0;
  return static_cast<float>(activeDelta) / static_cast<float>(totalDelta);
}
