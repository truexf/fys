//============================================================================
// Name        : helloworld.cpp
// Author      : fangys
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
using namespace std;
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "../fyslib/sysutils.h"
#include <unistd.h>
#include "../fyslib/tthread.h"
#include "../fyslib/AutoObject.hpp"
#include "../fyslib/systemtime.h"
#include <time.h>
#include "../fyslib/pandc.hpp"
#include "../fyslib/xtimer.h"
#include "../fyslib/xconfig.h"
#include "../fyslib/threadpool.h"
#include "../fyslib/xlog.h"
#include "../fyslib/xdaemon.h"
#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <memory>
#include <semaphore.h>
#include "../fyshttp/xio.h"
#include "../fyshttp/xsocketutils.h"
#include "../fyshttp/xserver.h"
#include "../fyshttp/xserver_consts.h"
#include "../fyshttp/xio.h"

#ifndef _GLIBCXX_USE_WCHAR_T
#define _GLIBCXX_USE_WCHAR_T
#endif

using namespace fyslib;
using namespace fyshttp;
using namespace std;

class MyThrd: public TThread
{
public:
	void Set(Pandc<int> *p)
	{
		m_p = p;
	}
protected:
	Pandc<int> *m_p;
	void Run()
	{
		for(int i=0;i<10;++i){
			m_p->P(i);
		}

	}
};

class MyThrd1: public TThread
{
public:
	void Set(Pandc<int> *p)
	{
		m_p = p;
	}
protected:
	Pandc<int> *m_p;
	void Run()
	{
		for(int i=0;i<10;++i){
			cout<<m_p->C()<<endl;
		}

	}
};


void func1()
{
	cout<<"timer proc"<<endl;
}

void func2(const string s)
{
	cout<<s<<endl;
}


class Pthrd: public TThread
{
public:
	void Set(LimitedPandc *queue,int cnt,pthread_mutex_t *lock)
	{
		m_queue = queue;
		m_cnt = cnt;
		m_lock = lock;
	}
protected:
	void Run()
	{
		AutoMutex auto1(m_lock);
		SetFreeOnTerminate(true);
		for (int i = 0;i<m_cnt;++i)
		{
			m_queue->P(NULL);
		}
	}
private:
	LimitedPandc *m_queue;
	int m_cnt;
	pthread_mutex_t *m_lock;
};

class Cthrd: public TThread
{
public:
	void Set(LimitedPandc *queue,int cnt,pthread_mutex_t *lock)
	{
		m_queue = queue;
		m_cnt = cnt;
		m_lock = lock;
	}
protected:
	void Run()
	{
		AutoMutex auto1(m_lock);
		SetFreeOnTerminate(true);
		for (int i = 0;i<m_cnt;++i)
		{
			void *a = m_queue->C();
			if (NULL != a)
			{
				cout<<"not null"<<endl;
			}
		}
	}
private:
	LimitedPandc *m_queue;
	int m_cnt;
	pthread_mutex_t *m_lock;
};

#define startTimeTick() 		struct timeval tv;\
memset(&tv,0,sizeof(tv));\
gettimeofday(&tv,NULL);\
string s__ = FormatString("start time: %d-%d",tv.tv_sec,tv.tv_usec);\
cout<<s__<<endl;

#define endTimeTick() 		memset(&tv,0,sizeof(tv));\
gettimeofday(&tv,NULL);\
s__ = FormatString("stop  time: %d-%d",tv.tv_sec,tv.tv_usec);\
cout<<s__<<endl;

#define waitForExit() 		cout<<"press Q to continue ...";\
cin>>s__;\

class RwlockThrd: public TThread
{
public:
	void Set(pthread_rwlock_t *rw,pthread_mutex_t *mtx)
	{
		m_rw = rw;
		m_mtx = mtx;
	}
protected:
	pthread_rwlock_t *m_rw;
	pthread_mutex_t *m_mtx;
	void Run()
	{
		LockMutex(m_mtx);
		for(int i = 0;i<10000000;++i)
		{
			pthread_rwlock_rdlock(m_rw);
			pthread_rwlock_unlock(m_rw);
		}
		UnlockMutex(m_mtx);
	}
};

void myHttpHandle(const XHttpRequest *req,/*out*/XHttpResponse *res)
{
	res->SetStatuCode(200);
	char * ret = "return from fyshttpsvr.";
	res->WriteHead("Content-Type","text/plain; charset=utf-8");
	res->WriteHead("Connection","Keep-Alive");
	res->WriteHead("Server","fyshttp");
	//res->WriteHead("Connection","close");
	res->WriteBody(ret,strlen(ret));
	return;
}

int main(int argc,char *argv[])
{

	//xserver
	{
		daemonize();
		XServer *svr = new XServer("/root/http.cfg");
		svr->RegisterHandler("/",myHttpHandle);
		svr->Start();
		for(;;)
		{
			sleep(1);
		}
		return 0;
	}

	//LimitedPandc
	{
		LimitedPandc *q = new LimitedPandc(100);
		timespec ts;
		ts.tv_sec = 1;
		ts.tv_nsec = 0;
		void * ret = NULL;
		while(1)
		{
			q->TimedC(&ts,&ret);
			cout<<"loop"<<endl;
		}
		string x;
		cin>>x;
		return 0;
	}


	//daemon
	{
		daemonize();
		for(;;)
		{
			sleep(1);
		}
		return 0;
	}

	//99乘法表
	{
		for(int i=1;i<10;++i)
		{
			for (int j=1;j<=i;++j)
			{
				printf("%d * %d = %-2d   ",j,i,i*j);
			}
			printf("\n");
		}
		return 0;
	}

	{
		sql::Driver *driver = get_driver_instance();
		sql::Connection *conn = driver->connect("localhost","root","");
		auto_ptr<sql::Connection> auto1(conn);
		conn->setSchema("fys");
		sql::Statement *stmt = conn->createStatement();
		auto_ptr<sql::Statement> auto2(stmt);
		conn->setAutoCommit(true);
		stmt->execute("select * from fys");
		sql::ResultSet *ret = stmt->getResultSet();
		auto_ptr<sql::ResultSet> auto3(ret);
		while(ret->next())
		{
			//cout<<"id:"<<ret->getString("id")<<endl;
			cout<<"name:"<<ret->getString("name")<<endl;
//			cout<<"birthday:"<<ret->getString("birthday")<<endl;
			cout<<"======================="<<endl;
		}
		conn->setAutoCommit(false);
		stmt->execute("insert into fys values ('阿布菜单')");
		//conn->commit();

		//stmt->execute("insert into fys(id,name,sex,birthday) values ('12345','方友松',1,'1983-05-12')");
		return 0;
	}
	//mutex test
	{
		startTimeTick();
		pthread_mutex_t *mtx = CreateMutex(false);
		for(int i = 0;i<20000000;++i)
		{
			LockMutex(mtx);
			UnlockMutex(mtx);
		}
		endTimeTick();
		return 0;
	}
	//rwlock test
	{
		startTimeTick();
		pthread_mutex_t *mtx = CreateMutex(false);
		pthread_rwlock_t rw;
		pthread_rwlock_init(&rw,NULL);
		RwlockThrd *thrd = new RwlockThrd;
		thrd->Set(&rw,mtx);
		thrd->Start();
		for(int i = 0;i<10000000;++i)
		{
			pthread_rwlock_rdlock(&rw);
			pthread_rwlock_unlock(&rw);
		}
		LockMutex(mtx);
		UnlockMutex(mtx);
		DestroyMutex(mtx);
		pthread_rwlock_destroy(&rw);
		endTimeTick();
		return 0;
	}
	//semaphore test
	{
		startTimeTick()
		sem_t mysem;
		sem_init(&mysem,0,0);
		for(int i=0;i<10000000;i++)
		{
			sem_post(&mysem);
		}
		endTimeTick();
		//waitForExit();
		return 0;
	}


	{
		cout<<"press Q to continue ...";
		string s;
		cin>>s;
		//cout<<s<<endl;
		//pause();
		return 0;
	}
	{
		cout<<sizeof(void*)<<endl;
		char * a = NULL;
		char * b = NULL;
		memcpy(a,b,3);
		cout<<a<<endl;
		return 0;
	}

	//LimitedPandc
	{
		struct timeval tv;
		memset(&tv,0,sizeof(tv));
		gettimeofday(&tv,NULL);
		string s = FormatString("start time: %d-%d",tv.tv_sec,tv.tv_usec);
		cout<<s<<endl;
		LimitedPandc queue(10000);
		vector<pthread_mutex_t*> locks;
		int thrd_count = 10;
		int pcount = 10000000;
		for(int i=0;i<thrd_count;++i)
		{
			pthread_mutex_t *plock = CreateMutex(false);
			pthread_mutex_t *clock = CreateMutex(false);
			Pthrd *thrdp = new Pthrd();
			thrdp->Set(&queue,pcount,plock);
			Cthrd *thrdc = new Cthrd();
			thrdc->Set(&queue,pcount,clock);
			locks.push_back(plock);
			locks.push_back(clock);
			thrdc->Start();
			thrdp->Start();
		}

		sleep(1);
		for(int i=0;i<thrd_count*2;++i)
		{
			AutoMutex auto1(locks[i]);
		}

		memset(&tv,0,sizeof(tv));
		gettimeofday(&tv,NULL);
		s = FormatString("stop  time: %d-%d",tv.tv_sec,tv.tv_usec);
		cout<<s<<endl;
		return 0;

	}

	//xlog
	{
		XLog log("/usr/fys/");
		log.LogError("logerror");
		log.LogInfo("logInfo");
		log.LogWarn("logWarn");
		sleep(3);
		return 0;
	}

	//thread pool
	{
		ThreadPool *pool = new ThreadPool(1);
		pool->PushTask(AutoFunc(func1));
		pool->PushTask(AutoFunc(func1));
		pool->PushTask(AutoFunc(func1));
		pool->PushTask(AutoFunc(func2,"abc"));
		pool->PushTask(AutoFunc(func2,"efg"));
		sleep(3);
		delete pool;
		sleep(2);
		return 0;
	}

	//xconfig
	{
		XConfig cfg;
		cfg.Set("sec","name","value");
		cfg.Set("sec","k","v");
		cfg.Set("data","1","2");
		cfg.SaveToFile("/usr/fys/cfg.ini");
		return 0;
	}
	//timespec
	{
		timespec ts;
		ts.tv_sec = 10;
		ts.tv_nsec = 0;
		timespec ts1;
		ts1.tv_sec = 1;
		ts1.tv_nsec = 0;
		TThread *thrd = StartAsyncTimer(AutoFunc(func1),true,&ts1);
		SleepExactly(&ts);
		cout<<"stop"<<endl;
		StopAsyncTimer(thrd);
		return 0;
	}
	{
		timespec ts;
		ts.tv_sec = 5;
		ts.tv_nsec = 0;
		SleepExactly(&ts);
		cout<<"ok"<<endl;
	}
	{
		Pandc<int> p1;
		MyThrd *t1 = new MyThrd;
		t1->Set(&p1);
		MyThrd1 *t2 = new MyThrd1;
		t2->Set(&p1);
		t2->Start();
		t1->Start();
		sleep(5);
		return 0;
	}
	{
		struct timespec t;
		clock_gettime(CLOCK_REALTIME,&t);
		cout<<t.tv_sec<<endl;
		cout<<t.tv_nsec<<endl;
		return 0;
	}

	//systemtime
	{
		SYSTEMTIME st;
		GetLocalTime(&st);
		cout<<FormatDatetime(st)<<endl;
		return 0;
	}
//	{
//		int i=0;
//		cout<<__sync_add_and_fetch(&i,1)<<endl;
//		cout<<__sync_val_compare_and_swap(&i,i,100)<<endl;
//		cout<<i<<endl;
//		cout<<__sync_sub_and_fetch(&i,2)<<endl;
//		return  0;
//	}

	//memory stream
	{
		MemoryStream stm;
		string s("hello world!");
		cout<<s<<endl;
		stm.Write(s.c_str(),s.length());
		stm.SaveToFile("/usr/fys/1");
		string s1;
		stm.Clear();
		stm.LoadFromFile("/usr/fys/1");
		s1.assign((char*)stm.GetBuffer(),stm.GetSize());
		cout<<s1<<endl;
		return 0;
	}

	//TThread
	{
		MyThrd *thrd = new MyThrd;
		thrd->SetFreeOnTerminate(true);
		thrd->Start();
		sleep(1);
		thrd->Stop();
		//pause();
		return 0;
	}

	//sysutils
	{
		wcout<<UpperCaseW(L"Hello World")<<endl;
		wcout<<LowerCaseW(L"Hello World")<< endl;
		return 0;
	}
	{
		vector<string> v1;
		GetCommandLineList(v1);
		for(size_t i=0; i<v1.size(); ++i)
			cout<<v1[i]<<endl;
		return 0;
	}
	if(FileExists("/proc/10957/cmdline"))
		cout<<"yes,exists"<<endl;
	cout<<Int2Str(1024)<<endl;
	int fd = open("/proc/10957/cmdline", O_RDONLY);
	if(-1 == fd)
		cout<<"open fail:"<<errno<<endl;
	else
	{
		unsigned char fv[500] = {0};
		read(fd,fv,500);
		close(fd);
	}
	cout<<getpid()<<endl;
	for(int i=0; i<argc; ++i)
		cout<<argv[i]<<endl;
//	{
//		char s[100] = {0};
//		gets(s);
//	}
	return 0;
	if(SameText("Hello workd","hello worKD"))
		cout<<"same text"<<endl;
	cout<<UpperCase("Hello World")<<endl;
	cout<<LowerCase("Hello World")<<endl;
	string s = trim(" asdf ");
	cout<<s<<endl;
	cout<<ExtractFileName("/abc/asdf")<<endl;
	cout<<ExtractFilePath("/abc/asdf",true)<<endl;
	ForceDirectory("/abc/adf");
	if(DirectoryExists("/abc/adf"))
		cout<<"dir created"<<endl;
	if(0 == access("/etc",R_OK))
		cout<<"file exists"<<endl;
	else
		cout<<"file not exists"<<endl;
	cout << "!!!Hello World!!!" << endl; // prints !!!Hello World!!!
	return 0;
}
