/*
 * xlog.h
 *
 *  Created on: Mar 10, 2015
 *      Author: root
 */

#ifndef FYSLIB_XLOG_H_
#define FYSLIB_XLOG_H_

#include "tthread.h"
#include <string.h>
#include <string>
#include <pthread.h>
#include "systemtime.h"

using namespace std;

namespace fyslib{

class XLog: public TThread
{
public:
  XLog(const char *p_logfilenamepath=NULL,int p_logqueue_size = 1024*1024,int p_flush_interval = 1000);
  virtual ~XLog();
  void Log(const void *data,int datasize);
  void LogWarn(const char *data);
  void LogError(const char *data);
  void LogInfo(const char *data);
  void LogDebug(const char *data);
protected:
  void Flush();
  void SwitchQueue();
  void Run(); //override;
private:
  //cannot clone
  XLog(const XLog &);
  XLog& operator=(const XLog &);

  //structures
  void *m_prequeue_logical;
  void *m_savingqueue_logical;
  void *m_prequeue_real;
  void *m_savingqueue_real;
  pthread_mutex_t * m_swaplock;
  pthread_mutex_t * m_prequeuelock;
  int m_prequeue_size;
  int m_savingqueue_size;
  int m_file_handle;
  PSYSTEMTIME m_file_date;
  PSYSTEMTIME m_file_date_tmp;
  char *m_computer_name;

  //parameters
  string m_logfilepath;
  int m_logqueue_size;
  int m_flush_interval;
};

}

#endif /* FYSLIB_XLOG_H_ */
