/*
 * xio.cpp
 *
 *  Created on: Mar 30, 2015
 *      Author: root
 */
#include "xio.h"
#include "xserver.h"
#include <string.h>


namespace fyshttp
{


XRequestState XHttpRequest::DataIn(/*in*/void *buf,/*in*/int buf_size)
{
	if(NULL == buf || 0 == buf_size)
		return m_state;
	m_data->Write(buf,buf_size);
lblcase:
	switch(m_state)
	{
	case rsNotHttp:
		break;
	case rsHeadUnCompleted:
	{
		void *p = AddPtr(m_data->GetBuffer(),4);
		for(int i=0;i<m_data->GetSize()-4;++i)
		{
			IncPtr(&p,1);
			if(memcmp(p,"\r\n\r\n",4) == 0)
			{
				m_state = rsBodyUncompleted;
				m_body_pos = i+4;
				string shead((char*)(m_data->GetBuffer()),m_body_pos+1);
				vector<string> vhead;
				vector<string> vhead_1;
				vector<string> vhead_2;
				SplitString(shead,"\r\n",vhead);
				SplitString(vhead[0]," ",vhead_1);
				m_method = vhead_1[0];
				SplitString(vhead_1[1],"?",vhead_2);
				m_path = vhead_2[0];
				if (vhead_2.size() > 1)
					m_url_params = vhead_2[1];
				else
					m_url_params.clear();
				m_head.clear();
				m_content_length = 0;
				if(vhead.size() > 1)
				{
					for(size_t ihead=1;ihead<vhead.size();++ihead)
					{
						string sk,sv;
						SplitString(vhead[ihead],":",sk,sv);
						sk = trim(sk);
						sv = trim(sv);
						if(SameText(sk,"Content-Length"))
						{
							m_content_length = atoi(sv.c_str());
						}
						m_head.push_back(XHttpHeadLine(sk,sv));
					}
				}
				if(m_content_length <= 0)
				{
					m_state = rsCompleted;
					//post packet to recv queue
					m_socket->m_server->m_recved_queue->P(Clone());
					Clear(true);
				}

				goto lblcase;
			}
		}
		break;
	}
	case rsBodyUncompleted:
	{
		int iretain = m_data->GetSize() - m_body_pos - m_content_length;
		if(iretain >= 0)
		{
			m_state = rsCompleted;
			//post packet to recv queue
			m_socket->m_server->m_recved_queue->P(this->Clone());
			if(iretain > 0)
			{
				void *p = m_data->GetBuffer();
				IncPtr(&p,m_body_pos + m_content_length);
				memmove(m_data->GetBuffer(),p,iretain); //maybe overlap
				m_data->Shrink(iretain+1);
				m_data->Seek(MemoryStream::soEnd,1);
				Clear(false);
			}
			else
				Clear(true);
			goto lblcase;
		}
		break;
	}
	case rsCompleted:
	{
		if(buf_size >= 4)
		{
			string smtd((char*)buf,4);
			if(!SameText(smtd,"GET ") && !SameText(smtd,"POST"))
			{
				m_state = rsNotHttp;
				goto lblcase;
			}
		}
		m_state = rsHeadUnCompleted;
		goto lblcase;
		break;
	}
	}

	return m_state;
}


}//end of namespace fyshttp


