
#ifndef BG_PROCESS_SERVICE_H
#define BG_PROCESS_SERVICE_H

#include <string>
#include <time.h>
#include <map>


using namespace std;
struct BgProcessData
{
 std::string processName;
 std::string processDesc;
 time_t startTime;
 float percent; 
  /**
   * percent completed
   *  < 0 don't/cannot know the percentage
   *  0-100 normal percent meaning
   *  >100  process finished and to be removed from the Back Ground Process list
   */
};

#endif
