#include "CCalendarUtil.h"

#include <calendar-backend/CMulticalendar.h>
#include <calendar-backend/CCalendar.h>
#include <calendar-backend/CTodo.h>
#include <calendar-backend/CEvent.h>
#include <calendar-backend/CalendarErrors.h>

#include "debug.h"
#define CALENDAR_NAME "eCoach activities"


int CCalendarUtil::addEvent(gchar *name,gchar *desc,gchar *location, time_t start,time_t end){
  DEBUG_BEGIN();
  int err;
  time_t t;
  time(&t);
  CEvent event(name,desc,location,start,end);
  CMulticalendar *multi = CMulticalendar::MCInstance();
  
  CCalendar *cal  = multi->getCalendarByName(CALENDAR_NAME,err);
  if(err == CALENDAR_DOESNOT_EXISTS)
  {
   int error;
   DEBUG("Calendar does not exist, creating a new one...");
   cal = multi->addCalendar(CALENDAR_NAME, COLOUR_VIOLET, 0, 1,LOCAL_CALENDAR,"","1.0",err);
   if(err != CALENDAR_OPERATION_SUCCESSFUL){
     return 1;
   }
   DEBUG("Calendar successfully created...");
  }
   cal->addEvent(&event,err); 
  
  
  DEBUG_END();
  return 0;
}
