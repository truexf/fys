/*
 * objectpool.hpp
 *
 *  Created on: Apr 3, 2015
 *      Author: root
 */

#ifndef HELLOLINUX_FYSLIB_OBJECTPOOL_HPP_
#define HELLOLINUX_FYSLIB_OBJECTPOOL_HPP_

#include <vector>
#include "AutoObject.hpp"
#include "tthread.h"

using std::vector;

namespace fyslib{

template<typename T>
class ObjectPool
{
public:
	ObjectPool()
	{
		m_lock = CreateMutex(false);
		m_capacity = 1024;
	}
	~ObjectPool()
	{
		{
			AutoMutex auto1(m_lock);
			for(size_t i=0;i<m_pool.size();++i)
			{
				delete m_pool[i];
			}
			m_pool.clear();
		}
		DestroyMutex(m_lock);
	}
	void SetCapacity(int v)
	{
		if (v > 0)
			m_capacity = v;
	}
	void PushObject(T* obj)
	{
		AutoMutex auto1(m_lock);
		if(m_pool.size() < m_capacity)
			m_pool.push_back(obj);
		else
			delete obj;
	}
	T* PullObject()
	{
		AutoMutex auto1(m_lock);
		if(m_pool.empty())
			m_pool.push_back(new T);
		T* ret = m_pool.back();
		m_pool.pop_back();
		return ret;
	}
private:
	ObjectPool(const ObjectPool&);
	ObjectPool& operator=(const ObjectPool&);
private:
	vector<T*> m_pool;
	pthread_mutex_t *m_lock;
	int m_capacity;
};


}//endof namespace

#endif /* HELLOLINUX_FYSLIB_OBJECTPOOL_HPP_ */
