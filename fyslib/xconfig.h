/*
 * xconfig.h
 *
 *  Created on: Mar 19, 2015
 *      Author: root
 */

#ifndef FYSLIB_XCONFIG_H_
#define FYSLIB_XCONFIG_H_

#include <map>
#include <vector>
#include <string>
#include "tthread.h"

using std::map;
using std::string;

namespace fyslib{

class XConfig
{
public:
	XConfig();
	virtual ~XConfig();

	bool LoadFromFile(const string &file);
	bool SaveToFile(const string &file);
	string Get(const string section,const string ident,string default_value);
	void Set(const string section,const string ident,const string value);
private:
	XConfig(const XConfig&);
	XConfig& operator = (const XConfig&);

	map<string,map<string,string>* > m_data;
	pthread_mutex_t *m_lock;
private:
	void Clear();
};

}

#endif /* FYSLIB_XCONFIG_H_ */
