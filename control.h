/* vim: set et ts=4 sts=4 sw=4 : */
/********************************************************************\
 * This program is free software; you can redistribute it and/or    *
 * modify it under the terms of the GNU General Public License as   *
 * published by the Free Software Foundation; either version 2 of   *
 * the License, or (at your option) any later version.              *
 *                                                                  *
 * This program is distributed in the hope that it will be useful,  *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of   *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    *
 * GNU General Public License for more details.                     *
 *                                                                  *
 * You should have received a copy of the GNU General Public License*
 * along with this program; if not, contact:                        *
 *                                                                  *
 * Free Software Foundation           Voice:  +1-617-542-5942       *
 * 59 Temple Place - Suite 330        Fax:    +1-617-542-2652       *
 * Boston, MA  02111-1307,  USA       gnu@gnu.org                   *
 *                                                                  *
\********************************************************************/

/** @file control.h
    @brief control related
    @author Copyright (C) 2016 Dengfeng Liu <liudengfeng@kunteng.org>
*/

#ifndef	_CONTROL_H_
#define	_CONTROL_H_

#include "const.h"
#include "uthash.h"
#include "msg.h"

struct proxy_client;
struct bufferevent;
struct event_base;
enum msg_type;

struct control {
	struct event_base 	*connect_base;  	//main netevent 
	struct evdns_base  	*dnsbase;
    struct bufferevent  *connect_bev;    	//main io evet buf
    char                session_id;
    struct event		*ticker_ping;    	//heartbeat timer
};

void connect_eventcb(struct bufferevent *bev, short events, void *ptr);
void sync_iv(unsigned char *iv);
void start_base_connect();
void sync_session_id(uint32_t sid);
void init_main_control();
void run_control();
struct control *get_main_control();
void close_main_control();
void start_login_frp_server(struct event_base *base);
void send_login_frp_server(struct bufferevent *bev);
void login();
void free_control();
void sync_session_id(uint32_t sid);

void send_msg_frp_server(struct bufferevent *bev, 
					const enum msg_type type, 
					const char *msg, 
					const size_t msg_len, 
					uint32_t sid);

void control_process(struct proxy_client *client);
void send_new_proxy(struct proxy_service *ps);

struct bufferevent 
*connect_server(struct event_base *base, const char *name, const int port);

#endif //_CONTROL_H_