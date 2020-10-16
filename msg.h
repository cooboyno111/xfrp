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

/** @file msg.h
    @brief xfrp msg struct
    @author Copyright (C) 2016 Dengfeng Liu <liudengfeng@kunteng.org>
*/

#ifndef _MSG_H_
#define _MSG_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#include "client.h"
#include "common.h"

#define TYPE_LEN 1 //byte, char

#define MSG_TYPE_I 	0
#define MSG_LEN_I 	1
#define MSG_DATA_I	5

// msg_type match frp v0.10.0
enum msg_type {
	TypeLogin         = 'o',
	TypeLoginResp     = '1',
	TypeNewProxy      = 'p',
	TypeNewProxyResp  = '2',
	TypeNewWorkConn   = 'w',
	TypeReqWorkConn   = 'r',
	TypeStartWorkConn = 's',
	TypePing          = 'h',
	TypePong          = '4',
	TypeUdpPacket     = 'u',
};

struct general_response {
	int		code;
	char	*msg;
};

struct control_response {
	int		type;
	int		code;
	char	*msg;
};

struct new_proxy_response {
	char 	*run_id;
	char 	*proxy_name;
	char	*error;
	int		remote_port;
};

struct work_conn {
	char *run_id;
};

struct message {
	char 	type;
	char	*data_p;
	size_t	data_len;
};

struct start_work_conn_resp {
	char 	*proxy_name;
};

int new_proxy_service_marshal(const struct proxy_service *np_req, char **msg);
int msg_type_valid_check(char msg_type);
struct message *new_message();
char *calc_md5(const char *data, int datalen);
char *get_auth_key(const char *token, long int *timestamp);
size_t login_request_marshal(char **msg);
size_t pack(struct message *req_msg, unsigned char **ret_buf);
struct message *unpack(unsigned char *recv_msg, const ushort len);

// tranlate control request to json string
struct new_proxy_response *new_proxy_resp_unmarshal(const char *jres);
struct login_resp *login_resp_unmarshal(const char *jres);
struct start_work_conn_resp *start_work_conn_resp_unmarshal(const char *resp_msg);

// parse json string to control response
struct control_response *control_response_unmarshal(const char *jres);
struct work_conn *new_work_conn();
int new_work_conn_marshal(const struct work_conn *work_c, char **msg);

void control_response_free(struct control_response *res);

#endif //_MSG_H_