/*
 * xprotocol.h
 *
 *  Created on: Mar 30, 2015
 *      Author: root
 */

#ifndef HELLOLINUX_FYSHTTP_XIO_H_
#define HELLOLINUX_FYSHTTP_XIO_H_

#include <string>
#include <vector>
#include <map>
#include <pthread.h>
#include "../fyslib/sysutils.h"
#include "../fyslib/AutoObject.hpp"
#include "../fyslib/tthread.h"

using std::string;
using std::vector;
using std::map;
using namespace fyslib;

namespace fyshttp
{

class XSocket;
class XServer;
class XHttpHandleManager;
class XHttpResponse;
enum XRequestState
{
	rsNotHttp, //不是一个HTTP请求
	rsHeadUnCompleted, //请求头尚未接收完成
	rsBodyUncompleted, //请求头已经接收完成，请求体尚未接收完成
	rsCompleted //请求头已经接收完成
};

//enum XResponseAction
//{
//	raPending, //响应未决，还需要更多的输入
//	raSuccess, //响应处理完成，可以发送响应并断开连接
//	raSuccessAndKeepAlive //响应处理完成，可以发送响应并保持连接
//};

struct XHttpHeadLine
{
	XHttpHeadLine(const string k,const string v):key(k),value(v){}
	string key;
	string value;
};

class XHttpRequest
{
public:
	XHttpRequest(XSocket *skt)
	{
		m_data = new MemoryStream;
		m_state = rsCompleted;
		m_body_pos = 0;
		m_content_length = 0;
		m_socket = skt;
	}
	~XHttpRequest()
	{
		delete m_data;
	}
	XRequestState State()
	{
		return m_state;
	}
	void Head(/*out*/vector<XHttpHeadLine> &head)
	{
		head.assign(m_head.begin(),m_head.end());
	}
	void Body(/*out*/MemoryStream &body)
	{
		void *p = m_data->GetBuffer();
		IncPtr(&p,m_body_pos);
		body.Empty();
		body.Write(p,m_data->GetSize() - m_body_pos);
	}
	string Method()
	{
		return m_method;
	}
	string Path()
	{
		return m_path;
	}
	string UrlParams()
	{
		return m_url_params;
	}
	int ContentLen()
	{
		return m_content_length;
	}
	XSocket *Socket()
	{
		return m_socket;
	}
	XRequestState DataIn(/*in*/void *buf,/*in*/int buf_size);
private:
	XHttpRequest(const XHttpRequest&);
	XHttpRequest& operator=(const XHttpRequest&);

	vector<XHttpHeadLine > m_head;
	MemoryStream *m_data;
	XRequestState m_state;
	int m_body_pos;
	string m_method;
	string m_path;
	string m_url_params;
	int m_content_length;
	XSocket *m_socket;
private:
	XHttpRequest *Clone()
	{
		XHttpRequest *ret = new XHttpRequest(m_socket);
		ret->m_head.assign(m_head.begin(),m_head.end());
		ret->m_data->Write(m_data->GetBuffer(),m_data->GetSize());
		ret->m_state = m_state;
		ret->m_body_pos = m_body_pos;
		ret->m_method = m_method;
		ret->m_path = m_path;
		ret->m_url_params = m_url_params;
		ret->m_content_length = m_content_length;
		return ret;
	}
	void Clear(bool emptyData)
	{
		if(emptyData)
			m_data->Empty();
		m_state = rsCompleted;
		m_body_pos = 0;
		m_method.clear();
		m_path.clear();
		m_url_params.clear();
		m_content_length = 0;
	}
};

class XHttpResponse
{
public:
	XHttpResponse()
	{
		m_body = new MemoryStream;
		m_statu_code = 200;
	}
	void SetSocket(XSocket *skt){
		m_socket = skt;
	}
	~XHttpResponse(){
		delete m_body;
	}

	void SetStatuCode(int c)
	{
		m_statu_code = c;
	}
	void WriteHead(const string key,const string value)
	{
		for(size_t i=0;i<m_head.size();++i)
		{
			if(SameText(key,m_head[i].key))
			{
				m_head[i].value = value;
				return;
			}
		}
		m_head.push_back(XHttpHeadLine(key,value));
	}
	void WriteBody(void *buf, int buf_size)
	{
		m_body->Write(buf,buf_size);
	}
	void DataOut(MemoryStream *out)
	{
		string s = FormatString("HTTP/1.1 %d\r\n",m_statu_code);
		out->Write(s.c_str(),s.length());
		WriteHead("Content-Length",Int2Str(m_body->GetSize()));
		for(size_t i=0;i<m_head.size();++i)
		{
			s = FormatString("%s: %s\r\n",m_head[i].key.c_str(),m_head[i].value.c_str());
			out->Write(s.c_str(),s.length());
		}
		out->Write("\r\n",2);
		if(m_body->GetSize() > 0)
			out->Write(m_body->GetBuffer(),m_body->GetSize());
	}
	void Clear()
	{
		m_statu_code = 200;
		m_head.clear();
		m_body->Empty();
	}
private:
	int m_statu_code;
	vector<XHttpHeadLine> m_head;
	MemoryStream *m_body;
	XSocket *m_socket;
};

typedef void (*HttpHandleFunc)(/*in*/const XHttpRequest *req,/*out*/XHttpResponse *res);

class XHttpHandleManager
{
public:
	XHttpHandleManager(){
		m_lock = CreateMutex(false);
	}
	~XHttpHandleManager(){
		DestroyMutex(m_lock);
	}

	void RegisterHandle(const string path,HttpHandleFunc func){
		AutoMutex auto1(m_lock);
		m_handles[path] = func;
	}
	HttpHandleFunc GetHandleFunc(const string path){
		map<string,HttpHandleFunc>::iterator it;
		{
			AutoMutex auto1(m_lock);
			it = m_handles.find(path);
		}
		if(m_handles.end() == it)
			return NULL;
		else
			return it->second;
	}
private:
	XHttpHandleManager(const XHttpHandleManager&);
	XHttpHandleManager& operator=(const XHttpHandleManager&);

	map<string,HttpHandleFunc> m_handles;
	pthread_mutex_t *m_lock;
};





}//end of namespace




#endif /* HELLOLINUX_FYSHTTP_XIO_H_ */
