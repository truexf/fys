/*
 * xserver_consts.h
 *
 *  Created on: Mar 29, 2015
 *      Author: root
 */

#ifndef HELLOLINUX_FYSHTTP_XSERVER_CONSTS_H_
#define HELLOLINUX_FYSHTTP_XSERVER_CONSTS_H_

namespace fyshttp{
//server config
const char * const CFG_SEC_NORMAL	= "normal";
const char * const CFG_NAME_LISTEN_PORT = "listen_port";
const char * const CFG_NAME_MAX_CONNECTION = "max_connection";
const char * const CFG_NAME_SERVER_INFO = "server_info";
const char * const CFG_NAME_EPOLL_THREAD_COUNT = "epoll_thread_count";
const char * const CFG_NAME_TASK_THREAD_COUNT = "task_thread_count";
const char * const CFG_NAME_RECV_QUEUE_SIZE = "recv_queue_size";
const char * const CFG_NAME_PER_SOCKET_SEND_QUEUE_SIZE = "per_socket_send_queue_size";

const char * const CFG_SEC_SOCKET	= "socket";
const char * const CFG_NAME_KEEPALIVE = "keep_alive";
const char * const CFG_NAME_KEEPALIVE_TIMEOUT = "keep_alive_timeout";
const char * const CFG_NAME_DISABLE_NAGLE = "disable_nagle";
const char * const CFG_NAME_LINGER_ONOFF = "linger_onoff";
const char * const CFG_NAME_LINGERF = "linger";
const char * const CFG_NAME_REUSE_ADDR = "reuse_addr";
const char * const CFG_NAME_RECV_BUF_SIZE = "recv_buf_size";
const char * const CFG_NAME_SEND_BUF_SIZE = "send_buf_size";

const char * const CFG_SEC_LOG	= "log";
const char * const CFG_NAME_LOG_PATH = "log_path";
const char * const CFG_NAME_LOG_QUEUE_LEN = "log_queue_len";
const char * const CFG_NAME_LOG_FLUSH_INTERVAL = "log_flush_interval";




}


#endif /* HELLOLINUX_FYSHTTP_XSERVER_CONSTS_H_ */
