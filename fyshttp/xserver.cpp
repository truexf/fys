/*
 * xserver.cpp
 *
 *  Created on: Mar 29, 2015
 *      Author: root
 */

#include "xserver.h"
#include <stdlib.h>
#include "xserver_consts.h"
#include "../fyslib/xconfig.h"
#include <iostream>
#include <string.h>
#include <sys/epoll.h>
#include <errno.h>
#include "xsocketutils.h"
#include "xio.h"
#include "../fyslib/xlog.h"
#include "../fyslib/sysutils.h"
#include "../fyslib/tthread.h"
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <unistd.h>

using namespace std;
using namespace fyslib;

namespace fyshttp{

XSocket::XSocket()
{
	m_fd = 0;
	m_remote_addr = new sockaddr_in;
	m_state = ssUnaccepted;
	m_recv_lock = CreateMutex(false);
	m_send_lock = CreateMutex(true);
	m_request = new XHttpRequest(this);
	m_send_buf = new MemoryStream;
}
ssize_t XSocket::Recv()
{
	AutoMutex auto1(m_recv_lock);
	ssize_t ret = 0;
	ssize_t sz;
	while(true)
	{
		sz = recv(m_fd,m_recv_buf,RECV_BUF_SIZE,0);
		if(-1 == sz)
		{
			switch(errno)
			{
			case EWOULDBLOCK: //equal to EAGAIN
				goto lblret;
				break;
			default:
				m_server->m_log->LogError(FormatString("socket recv() fail,errno: %d",errno).c_str());
				//todo: close socket
				return 0;
			}
		}
		else if(0==sz)
		{
			//todo: peer shutdown close socket
			return 0;
		}
		m_request->DataIn(m_recv_buf,sz);
		ret += sz;
		if(sz < RECV_BUF_SIZE)
			goto lblret;
	}

	lblret:
	return ret;
}


ssize_t XSocket::Send()
{
	AutoMutex auto1(m_send_lock);
	if(0 == m_send_buf->GetSize())
		return 0;

	void *buf = m_send_buf->GetBuffer();
	int buf_size = m_send_buf->GetSize();
	ssize_t ret = 0;
	ssize_t sz = 0;
	int send_size;
	void *p = buf;
	while(true)
	{
		IncPtr(&p,sz);
		int retain = buf_size - ret;
		if (retain <= SEND_BUF_SIZE)
			send_size = retain;
		else
			send_size = SEND_BUF_SIZE;
		sz = send(m_fd,p,send_size,0);
		if(-1 == sz)
		{
			switch(errno)
			{
			case EAGAIN:
				goto lblret;
			default:
				m_server->m_log->LogError(FormatString("socket send() fail,errno: %d",errno).c_str());
				//todo: close socket
				goto lblret;
			}
		}
		ret += sz;

		if(sz <= send_size)
			goto lblret;
		if(ret>=buf_size)
			goto lblret;
	}

	lblret:
	if(ret > 0)
	{
		if(ret < buf_size)
		{
			p = m_send_buf->GetBuffer();
			IncPtr(&p,ret);
			int iretain = m_send_buf->GetSize() - ret;
			memcpy(m_send_buf->GetBuffer(),p,iretain);
			m_send_buf->Shrink(iretain);
		}
		else
		{
			if(m_send_buf->GetCapacity() > 8192)
				m_send_buf->Clear();
		}
	}
	return ret;
}

XServer::XServer(const char *cfgfile)
{
	//1. load config
	XConfig *cfg = new XConfig;
	if(!cfg->LoadFromFile(cfgfile))
	{
		cout<<"load config from file <"<<cfgfile<<"> failed,process is terminated."<<endl;
		_exit(1);
	}
	m_config = new XServerConfig;
	m_config->listen_port = atoi(cfg->Get(CFG_SEC_NORMAL,CFG_NAME_LISTEN_PORT,"8888").c_str());
	m_config->max_connection = atoi(cfg->Get(CFG_SEC_NORMAL,CFG_NAME_MAX_CONNECTION,"65535").c_str());
	m_config->server_info = cfg->Get(CFG_SEC_NORMAL,CFG_NAME_SERVER_INFO,"XServer");
	m_config->epoll_thread_count = atoi(cfg->Get(CFG_SEC_NORMAL,CFG_NAME_EPOLL_THREAD_COUNT,"3").c_str());
	m_config->task_thread_count = atoi(cfg->Get(CFG_SEC_NORMAL,CFG_NAME_TASK_THREAD_COUNT,"3").c_str());
	m_config->recv_queue_size = atoi(cfg->Get(CFG_SEC_NORMAL,CFG_NAME_RECV_QUEUE_SIZE,"1000").c_str());

	m_config->keep_alive = atoi(cfg->Get(CFG_SEC_SOCKET,CFG_NAME_KEEPALIVE,"1").c_str());
	m_config->keep_alive_timeout = atoi(cfg->Get(CFG_SEC_SOCKET,CFG_NAME_KEEPALIVE_TIMEOUT,"1800").c_str());
	m_config->disable_nagle = atoi(cfg->Get(CFG_SEC_SOCKET,CFG_NAME_DISABLE_NAGLE,"1").c_str());
	m_config->reuse_addr = atoi(cfg->Get(CFG_SEC_SOCKET,CFG_NAME_REUSE_ADDR,"1").c_str());
	m_config->recv_buf_size = atoi(cfg->Get(CFG_SEC_SOCKET,CFG_NAME_RECV_BUF_SIZE,"8192").c_str());
	m_config->send_buf_size = atoi(cfg->Get(CFG_SEC_SOCKET,CFG_NAME_SEND_BUF_SIZE,"8192").c_str());

	m_config->log_flush_interval = atoi(cfg->Get(CFG_SEC_NORMAL,CFG_NAME_LOG_FLUSH_INTERVAL,"1000").c_str());
	m_config->log_queue_len = atoi(cfg->Get(CFG_SEC_NORMAL,CFG_NAME_LOG_QUEUE_LEN,"8192").c_str());
	m_config->log_path = cfg->Get(CFG_SEC_NORMAL,CFG_NAME_LOG_PATH,"");
	m_handle_mgr = new XHttpHandleManager;
	m_response_obj_pool = new ObjectPool<XHttpResponse>;
}

XServer::~XServer()
{
	delete m_response_obj_pool;
	delete m_handle_mgr;
	delete m_config;
}

void XServer::Run()
{
	SetFreeOnTerminate(true);
	//create queue
	m_recved_queue = new LimitedPandc(m_config->recv_queue_size);

	//create logger
	m_log = new XLog(m_config->log_path.c_str(),m_config->log_queue_len,m_config->log_flush_interval);
	m_log->Start();

	//create epoll
	m_epoll_handle = epoll_create(1000);
	if(-1 == m_epoll_handle)
	{
		cout<<"create epoll failed,process is terminated.errno:"<<errno<<endl;
		_exit(1);
	}

	//create epoll threads
	for(int i = 0;i<m_config->epoll_thread_count;++i)
	{
		XEpollThread *thrd = new XEpollThread;
		thrd->m_server = this;
		m_epoll_threads.push_back(thrd);
		thrd->Start();
	}

	//create task threads
	for(int i = 0;i<m_config->task_thread_count;++i)
	{
		XTaskThread *thrd = new XTaskThread;
		thrd->m_server = this;
		m_workers.push_back(thrd);
		thrd->Start();
	}

	//do listen
	m_listener = CreateTCPSocket();
	if(-1 == m_listener)
	{
		cout<<"create listener failed,process is terminated.errno:"<<errno<<endl;
		_exit(1);
	}
	if (-1 == SetSocketNonblock(m_listener))
	{
		cout<<"set listen socket nonblock failed,process is terminated.errno:"<<errno<<endl;
		_exit(1);
	}
	SetSocketReuseAddr(m_listener,m_config->reuse_addr);
	if(-1 == TcpBind(m_listener,"0.0.0.0",m_config->listen_port))
	{
		cout<<"bind listen socket failed,process is terminated.errno:"<<errno<<endl;
		_exit(1);
	}
	if(-1 == listen(m_listener,100))
	{
		cout<<"listen failed,process is terminated.errno:"<<errno<<endl;
		_exit(1);
	}
	m_log->LogInfo(FormatString("xserver is running, listening port: %d",m_config->listen_port).c_str());
	while (!GetTerminated())
	{
		timeval tv;
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		sockaddr_in addr;
		socklen_t addr_len = sizeof(addr);
		int skt = AcceptTimeout(m_listener,&tv,(sockaddr*)&addr,&addr_len);
		if(-1 == skt)
			continue;
		XSocket *clt = new XSocket;
		clt->Accept(skt,&addr);
		clt->SetServer(this);
		SetSocketKeepalive(skt,m_config->keep_alive,m_config->keep_alive_timeout);
		SetSocketNagle(skt,m_config->disable_nagle);
		SetSocketNonblock(skt);
		SetSocketSendbufSize(skt,m_config->send_buf_size);
		SetSocketRecvbufSize(skt,m_config->recv_buf_size);
		struct epoll_event evt;
		evt.data.ptr = (void*)clt;
		evt.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET;
		if(-1 == epoll_ctl(m_epoll_handle,EPOLL_CTL_ADD,skt,&evt))
		{
			m_log->LogError(FormatString("epll_clt failed,errno:%d",errno).c_str());
			continue;
		}
	}


	//9. clean
	//todo ...
}

void XEpollThread::Run()
{
	SetFreeOnTerminate(true);

	int ready;
	XSocket *skt = NULL;
	while (!GetTerminated())
	{
		ready = epoll_wait(m_server->m_epoll_handle,m_evlist,sizeof(m_evlist)/sizeof(m_evlist[0]),1000);
		if(-1 == ready)
		{
			if(EINTR == errno)
				continue;
			else
			{
				m_server->m_log->LogError(FormatString("epoll_wait fail,errno: %d",errno).c_str());
				break;
			}
		}
		if(0 == ready)
			continue;
		for(int i=0;i<ready;++i)
		{
			skt = (XSocket*)(m_evlist[i].data.ptr);
			if(m_evlist[i].events & EPOLLIN)
			{
				skt->Recv();
			}
			if(m_evlist[i].events & EPOLLOUT)
			{
				skt->Send();
			}
			if(m_evlist[i].events & EPOLLRDHUP)
			{
				//todo: peer shutdown close socket epoll_ctl_del
				skt->Close();
			}
			if(m_evlist[i].events & (EPOLLHUP | EPOLLERR))
			{
				//todo: close socket epoll_ctl_del
				skt->Close();
			}
		}
	}

}

void XTaskThread::Run()
{
	SetFreeOnTerminate(false);
	void *req = NULL;
	timespec ts;
	ts.tv_sec = 1;
	ts.tv_nsec = 0;
	HttpHandleFunc handle = NULL;
	XHttpRequest *request;
	while(!GetTerminated())
	{
		if(m_server->m_recved_queue->TimedC(&ts,&req))
		{
			request = (XHttpRequest*)req;
			handle = m_server->m_handle_mgr->GetHandleFunc(request->Path());
			if(handle)
			{
				XHttpResponse *res = m_server->m_response_obj_pool->PullObject();
				res->Clear();
				res->SetSocket(request->Socket());
				handle(request,res);
				{
					AutoMutex auto1(request->Socket()->m_send_lock);
					request->Socket()->m_send_buf->Empty();
					res->DataOut(request->Socket()->m_send_buf);
					request->Socket()->Send();
				}
				m_server->m_response_obj_pool->PushObject(res);
			}
			else  //no handler,reset peer
			{
				SetSocketLinger(request->Socket()->m_fd,1,0);
				close(request->Socket()->m_fd);
			}
		}
	}
}


}//end of namespace fyshttp
