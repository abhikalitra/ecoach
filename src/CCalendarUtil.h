#ifndef __CCALENDAR_UTIL_H__
#define __CCALENDAR_UTIL_H__

#include <glib.h>
class CCalendarUtil {

  public:
   
  int addEvent(gchar *name,gchar *desc,gchar *location, time_t start,time_t end);
  
 
  private:
   
};
   
#endif