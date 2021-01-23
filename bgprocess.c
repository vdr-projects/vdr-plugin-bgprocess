/*
 * bgprocess.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <vdr/plugin.h>
#include "service.h"
#include <map>
#include <time.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include "i18n.h"


using namespace std;
std::map <string, BgProcessData> bgProcessList; // list of all BackGround Process running
std::map <string, BgProcessData> CompletedList; // list of all BackGround Process finished
std::map <string, time_t> CompletedTimeList; // list of all BackGround Process finished

static const char *VERSION        = "0.1.0";
static const char *DESCRIPTION    = "Collects and lists background processes and their status";
static char MAINMENUENTRY[32]     = "Activity";

int nProcess=0;
float avgPercent=0.0;


class BgProcessMenu:public cOsdMenu
{
 enum {showCompleted, showRunning};;
 int showMode;
 public:
        BgProcessMenu();
        void ShowBgProcesses();
        void ShowCompleted();
        eOSState ProcessKey(eKeys k);
};

class cPluginBgprocess : public cPlugin {
private:
  bool AddProcess(BgProcessData pData);
  bool RemoveProcess(BgProcessData pData);
  string GenKey(BgProcessData pData);
public:
  cPluginBgprocess(void);
  virtual ~cPluginBgprocess();
  virtual const char *Version(void) { return VERSION; }
  virtual const char *Description(void) { return DESCRIPTION; }
  virtual const char *CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char *argv[]);
  virtual bool Initialize(void);
  virtual bool Start(void);
  virtual void Stop(void);
  virtual void Housekeeping(void);
  virtual void MainThreadHook(void);
  virtual cString Active(void);
  virtual const char *MainMenuEntry(void) {

          nProcess = bgProcessList.size();
	  avgPercent = 0.0;
	  std::map <string, BgProcessData>::iterator it;
	  for (it = bgProcessList.begin(); it!= bgProcessList.end(); it++)
	  if (it->second.percent>0)
	   avgPercent +=it->second.percent ;

          if(nProcess>0)
	  {
	      avgPercent /= nProcess;
              //sprintf(MAINMENUENTRY, tr("Activity (%d%% - %d process%s)"), (int)avgPercent, nProcess, nProcess>1?"es":" ");
	      sprintf(MAINMENUENTRY, tr("%d background process (%d%%)"), nProcess, (int)avgPercent);
	  }
	  else
	      //sprintf(MAINMENUENTRY,tr("Activity"));
	      return NULL;

	  return MAINMENUENTRY;
	  }
  virtual cOsdObject *MainMenuAction(void);
  virtual cMenuSetupPage *SetupMenu(void);
  virtual bool SetupParse(const char *Name, const char *Value);
  virtual bool HasSetupOptions(void) { return false; };
  virtual bool Service(const char *Id, void *Data = NULL);
  virtual const char **SVDRPHelpPages(void);
  virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
  };

bool cPluginBgprocess::AddProcess(BgProcessData pData)
{
  string key = GenKey(pData);
  /*map<string, BgProcessData>::iterator iter = bgProcessList.find(key);
  if (iter == bgProcessList.end()) // not found
   return false;*/
  bgProcessList[key] = pData;


  return true;
}
bool cPluginBgprocess::RemoveProcess(BgProcessData pData)
{
  string key = GenKey(pData);
  map<string, BgProcessData>::iterator iter = bgProcessList.find(key);
  if (iter == bgProcessList.end()) // not found
   return false;
  bgProcessList.erase(iter);
  // add to completed list
  CompletedList[key] = pData;
  CompletedTimeList[key] = time(NULL);

  // compute new average for Display
  return true;
}

string cPluginBgprocess::GenKey(BgProcessData pData)
{
  std::ostringstream oss;
  oss << pData.processName<<" "<<pData.startTime;  // converts name + time to a string
  std::string key(oss.str());
  return key;
}

cPluginBgprocess::cPluginBgprocess(void)
{
  // Initialize any member variables here.
  // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
  // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
  bgProcessList.clear(); // start fresh
}

cPluginBgprocess::~cPluginBgprocess()
{
  // Clean up after yourself!
  bgProcessList.clear();
}

const char *cPluginBgprocess::CommandLineHelp(void)
{
  // Return a string that describes all known command line options.
  return NULL;
}

bool cPluginBgprocess::ProcessArgs(int argc, char *argv[])
{
  // Implement command line argument processing here if applicable.
  return true;
}

bool cPluginBgprocess::Initialize(void)
{
  // Initialize any background activities the plugin shall perform.
#if VDRVERSNUM < 10507
RegisterI18n(tlPhrases);
#endif
  return true;
}

bool cPluginBgprocess::Start(void)
{
  // Start any background activities the plugin shall perform.
  return true;
}

void cPluginBgprocess::Stop(void)
{
  // Stop any background activities the plugin shall perform.
}

void cPluginBgprocess::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

void cPluginBgprocess::MainThreadHook(void)
{
  // Perform actions in the context of the main program thread.
  // WARNING: Use with great care - see PLUGINS.html!
}

cString cPluginBgprocess::Active(void)
{
  // Return a message string if shutdown should be postponed
  return NULL;
}

cOsdObject *cPluginBgprocess::MainMenuAction(void)
{
  // Perform the action when selected from the main VDR menu.
  //return NULL;
  return new BgProcessMenu;
}

cMenuSetupPage *cPluginBgprocess::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return NULL;
}

bool cPluginBgprocess::SetupParse(const char *Name, const char *Value)
{
  // Parse your own setup parameters and store their values.
  return false;
}

bool cPluginBgprocess::Service(const char *Id, void *Data)
{
  // Handle custom service requests from other plugins
  if (strcmp(Id,"bgprocess-data") == 0)
  {
   if ( !Data ) return true;

  BgProcessData* newBgProcess = new BgProcessData;
  BgProcessData* t = (BgProcessData*) Data;

  *newBgProcess = *t;

  if (newBgProcess->percent > 100 ) // process to be removed
      RemoveProcess(*newBgProcess);

  else if (newBgProcess->percent <= 100 )
      AddProcess(*newBgProcess); // should also update process info if 0 <= percent <= 100

 // debugging START
  ofstream of;
  of.open("/tmp/bglist", ios::app);
  of <<"Name:"<< newBgProcess->processName<< " Desc:"<< newBgProcess->processDesc
     <<" %:" <<newBgProcess->percent<<" startTime:"<<newBgProcess->startTime<<endl;
  of.close();
 // debugging END


  delete newBgProcess;
  return true;
  }
  return false;
}

const char **cPluginBgprocess::SVDRPHelpPages(void)
{
  // Return help text for SVDRP commands this plugin implements
  static const char* HelpPages[] = {
  "PROCESS\n",
  NULL
  };
  return HelpPages;
}

cString cPluginBgprocess::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
  // Process SVDRP commands this plugin implements
  if( strcasecmp(Command,"PROCESS") != 0 ) return NULL;

  char name[512]; // plugin/process name
  char desc[512]; // description
  long long int t; // time
  int perCent;
  //sprintf(a, "(%s:%d) %s \n\t\t %s \n\t\t %s", __FILE__,__LINE__,__PRETTY_FUNCTION__, Command, Option);
  int isSuccess = 1;
  int flag = sscanf(Option, "%s %lld %d %[^\n] ", name, &t, &perCent, desc) ;
  if(flag != 4 ) isSuccess = 0;
  name[511] = 0;
  desc[511] = 0;
  
  BgProcessData* newBgProcess = new BgProcessData;
  newBgProcess->processName = name;
  newBgProcess->processDesc = desc;
  newBgProcess->percent = perCent;
  newBgProcess->startTime = t;
 
  if(isSuccess)
	  isSuccess &= Service("bgprocess-data", newBgProcess);
  
  delete(newBgProcess);
  return isSuccess ? "OK" : "FAILED" ;
  //return cString::sprintf("%s %lld %d %s (%d)", name, t, perCent, desc, flag) ;
}


// ---- BgProcessMenu --------------------------------------

BgProcessMenu::BgProcessMenu() : cOsdMenu(tr("Background processes"),4,10,13)
{
 ShowBgProcesses();
 // Change this to last added
 //if (bgProcessList.size()) showMode = showRunning;
 //else showMode = showCompleted;
}

void BgProcessMenu::ShowCompleted()
{
  SetCols(4,10,13);
  std::map<string,BgProcessData>::iterator iter = CompletedList.begin();
  std::map<string,time_t>::iterator iterTime = CompletedTimeList.begin();
  int i=0;
  Clear();
  if (iter == CompletedList.end())
   {
    Add(new cOsdItem(" ",osUnknown,false));
    Add(new cOsdItem(tr(" No process completed"),osUnknown,false));
   }
 for (; iter != CompletedList.end(); iterTime++, iter++)
 {
  i++;
  std::ostringstream oss;
  struct tm *time_tm = localtime(&iter->second.startTime);
  oss<< i <<"\t"<<iter->second.processName<<"\t"<<time_tm->tm_hour<<":"<<time_tm->tm_min;
  time_tm = localtime(&iterTime->second);
  oss<<" - "<<time_tm->tm_hour<<":"<<time_tm->tm_min<<"\t"<<iter->second.processDesc;

  Add(new cOsdItem(oss.str().c_str(),osUnknown,false));
 }
 SetTitle(tr("Completed processes"));

 if (!CompletedList.empty())
     cOsdMenu::SetHelp(tr("Clear list"), NULL, NULL , tr("Running"));
 else
     cOsdMenu::SetHelp(NULL, NULL, NULL , tr("Running"));

 Display();
}


void BgProcessMenu::ShowBgProcesses()
{
  SetCols(4,10,13,7);
 // loop through the list and display
 std::map<string,BgProcessData>::iterator iter = bgProcessList.begin();
 int i=0;
 Clear();
 if (iter == bgProcessList.end() && CompletedList.size()==0)
   {
    Add(new cOsdItem(" "),osUnknown,false);
    Add(new cOsdItem(tr(" No background process running"),osUnknown,false));
   }
 for (; iter != bgProcessList.end(); iter++)
 {
  i++;
  if( iter->second.percent < 0 ) // donot show process with -ve percentage_done
  continue;

  // make the progress bar string [|||||    ]
  std::ostringstream oss;
  char per_str[100+3];
  per_str[102]='\0'; per_str[0]='['; per_str[101]=']';
  for (int j=0; j<100; j++) per_str[1+j]=' ';
  for (int j=0; j<iter->second.percent; j++) per_str[1+j] = '|';

  // calc hour and minute
  //struct tm *time_tm = localtime(&iter->second.startTime);

  //  oss<< i <<"\t"<<iter->second.processName<<"\t"<<per_str<<"\t"<<time_tm->tm_hour<<":"<<time_tm->tm_min<<"\t"<<iter->second.processDesc;
  oss<< i <<"\t"<<iter->second.processName<<"\t"<<per_str<<"\t"<<iter->second.percent<<"%\t"<<iter->second.processDesc;

  Add(new cOsdItem(oss.str().c_str(),osUnknown,false));
 }
 SetTitle(tr("Running processes"));

/** Completed process */
 iter = CompletedList.begin();
 std::map<string,time_t>::iterator iterTime = CompletedTimeList.begin();

 for (; iter != CompletedList.end(); iter++,iterTime++)
 {
	 i++;
	 std::ostringstream oss;
	 struct tm *time_tm = localtime(&iter->second.startTime);
	 oss<< i <<"\t"<<iter->second.processName<<"\t"<<time_tm->tm_hour<<":"<<setw(2)<<setfill('0')<<time_tm->tm_min;
	 time_tm = localtime(&iterTime->second);
	 oss<<" - "<<time_tm->tm_hour<<":"<<setw(2)<<setfill('0')<<time_tm->tm_min<<"\t\t"<<tr("Completed");

	 Add(new cOsdItem(oss.str().c_str(),osUnknown,false));
 }


 //cOsdMenu::SetHelp(NULL,tr("Completed"), NULL , NULL);
 cOsdMenu::SetHelp(NULL,NULL, NULL , NULL);
 Display();
}

eOSState BgProcessMenu::ProcessKey(eKeys k)
{
 // if (showMode == showRunning) ShowBgProcesses();
  //else ShowCompleted();
  ShowBgProcesses();
  eOSState state = cOsdMenu::ProcessKey(k);
//  switch (k)
//  {
//   case kBlue: showMode = showRunning; break;
//   case kGreen: showMode = showCompleted; break;
//   case kRed: if(showMode == showCompleted) CompletedList.clear(); break;
//   default: break;
//  }
  return state;
}

VDRPLUGINCREATOR(cPluginBgprocess); // Don't touch this!

