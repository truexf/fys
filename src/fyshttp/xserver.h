/*
 * httpserver.h
 *
 *  Created on: Mar 25, 2015
 *      Author: fangyousong@qq.com
 *
 *工作流程
 * 1. 服务器启动，加载配置，创建epoll,日志器，创建epoll等待线程，创建task线程，侦听，利用select接受连接，连接成功以后设置非阻塞，创建一个包装socket的xsocket对象，并将其加入到epoll的监测中
 * 2. epoll线程启动，循环epoll_wait，收到EPOLL_IN接收数据，并将接收到的完整逻辑包封装未httprequest结构放入服务器的请求队列，收到EPOLL_OUT，发送写缓存中的数据，收到错误，写日志，关闭socket,或者退出循环
 * 3. task线程启动，循环从服务器的请求队列拉取请求，根据请求的路径信息获取相关的处理函数指针，处理函数处理完请求，将结果写入response参数，task线程将response写入该socket的写缓存中。
 */

#ifndef FYSHTTP_XSERVER_H_
#define FYSHTTP_XSERVER_H_

#include <string>
#include <vector>
#include <sys/epoll.h>
#include "xsocketutils.h"
#include "../fyslib/tthread.h"
#include "../fyslib/xtimer.h"
#include "../fyslib/sysutils.h"
#include "../fyslib/pandc.hpp"
#include "../fyslib/threadpool.h"
#include "../fyslib/systemtime.h"
#include "../fyslib/AutoObject.hpp"
#include "../fyslib/xconfig.h"
#include "../fyslib/xlog.h"
#include "../fyslib/objectpool.hpp"
#include "xsocketutils.h"
#include "xio.h"

using std::string;
using std::vector;
using namespace fyslib;

namespace fyshttp{

#define RECV_BUF_SIZE	8192
#define SEND_BUF_SIZE	8192

enum XSocketState
{
	ssUnaccepted,
	ssAccepted,
	ssRecving,
	ssSending,
	ssShutdown_read,
	ssShutdown_writ,
	ssShutdown_rdwr,
	ssClosed
};

//typedef void (*XHandleFunction)(const void *request,int request_size,void **buf,int *buf)

class XSocket;
class XServer;
class XEpollThread;
class XTaskThread;

struct XBuf
{
	XSocket *skt;
	void *ptr;
	int buf_size;
};

class XSocket
{
	friend class XServer;
	friend class XHttpRequest;
	friend class XHttpResponse;
	friend class XTaskThread;
	friend class XEpollThread;
public:
	XSocket();
	~XSocket()
	{
		delete m_request;
		delete m_remote_addr;
		delete m_send_buf;
		DestroyMutex(m_recv_lock);
		DestroyMutex(m_send_lock);
	}
private:
	XSocket(const XSocket&);
	XSocket& operator = (const XSocket&);
private:
	void Accept(int sockfd,sockaddr_in *addr_remote)
	{
		m_fd = sockfd;
		memcpy(m_remote_addr,addr_remote,sizeof(sockaddr_in));
		m_state = ssAccepted;
	}
	ssize_t Recv();
	ssize_t Send();
	void Shutdown(int how)
	{
		if (0 == how)
			return;
		switch(how)
		{
		case SHUT_RD:
			shutdown(m_fd,how);
			m_state = ssShutdown_read;
			break;
		case SHUT_WR:
			shutdown(m_fd,how);
			m_state = ssShutdown_writ;
			break;
		case SHUT_RDWR:
			shutdown(m_fd,how);
			m_state = ssShutdown_rdwr;
			break;
		}

	}
	void Close()
	{
		close(m_fd);
	}
	void SetServer(XServer *svr)
	{
		m_server = svr;
	}
private:
	int m_fd;
	sockaddr_in *m_remote_addr;
	XSocketState m_state;
	XServer *m_server;
	pthread_mutex_t *m_recv_lock;
	pthread_mutex_t *m_send_lock;
	unsigned char m_recv_buf[RECV_BUF_SIZE];
	XHttpRequest *m_request;
	MemoryStream *m_send_buf;
};

class XEpollThread: public TThread
{
	friend class XServer;
protected:
	void Run(); //override;
private:
	XServer *m_server;
	struct epoll_event m_evlist[88];
};


class XTaskThread: public TThread
{
	friend class XServer;
protected:
	void Run(); //override;
private:
	XServer *m_server;
};

struct XServerConfig
{
	//common
	unsigned short listen_port;
	int max_connection;
	string server_info;
	int epoll_thread_count; //epoll_wait的线程数
	int recv_queue_size;  //全局的请求队列长度，逻辑完整的请求包放入这个队列
	int task_thread_count; //请求处理线程数，从请求队列取出请求包，进行处理，将响应写入具体的socket的响应缓冲区

	//socket option
	int keep_alive;
	int keep_alive_timeout;
	int disable_nagle;
	int linger_onoff;
	int linger;
	int reuse_addr;
	int recv_buf_size;
	int send_buf_size;

	//log option
	string log_path;
	int log_queue_len;
	int log_flush_interval;
};

class XServer: public TThread
{
	friend class XSocket;
	friend class XEpollThread;
	friend class XTaskThread;
	friend class XHttpRequest;
	friend class XHttpResponse;
public:
	XServer(const char *cfgfile);
	virtual ~XServer();

	inline void RegisterHandler(const string path,HttpHandleFunc func){
		m_handle_mgr->RegisterHandle(path,func);
		return;
	}
private:
	XServer(const XServer&);
	XServer& operator = (const XServer&);

protected:
	void Run(); //override;
private:
	LimitedPandc *m_recved_queue;
	LimitedPandc *m_send_queue;
	vector<XEpollThread*> m_epoll_threads;
	vector<XTaskThread*> m_workers;
	int m_epoll_handle;
	int m_listener;
	XLog *m_log;
	XServerConfig *m_config;
	XHttpHandleManager *m_handle_mgr;
	ObjectPool<XHttpResponse> *m_response_obj_pool;
};




}
#endif /* FYSHTTP_XSERVER_H_ */
