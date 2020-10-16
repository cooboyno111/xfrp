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

/** @file client.c
    @brief client surrogate for local service
    @author Copyright (C) 2016 Dengfeng Liu <liudengfeng@kunteng.org>
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#include <syslog.h>

#include <zlib.h>

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>

#include "debug.h"
#include "client.h"
#include "uthash.h"
#include "control.h"
#include "config.h"
#include "const.h"
#include "uthash.h"
#include "zip.h"
#include "common.h"
#include "proxy.h"
#include "utils.h"

#define MAX_OUTPUT (512*1024)

static void drained_writecb(struct bufferevent *bev, void *ctx);
static void xfrp_event_cb(struct bufferevent *bev, short what, void *ctx);

static void xfrp_read_cb(struct bufferevent *bev, void *ctx)
{
	struct proxy *p = (struct proxy *)ctx;
	struct bufferevent *partner = p?p->bev:NULL;
	struct evbuffer *src, *dst;
	size_t len;
	src = bufferevent_get_input(bev);
	len = evbuffer_get_length(src);
	if (!partner) {
		evbuffer_drain(src, len);
		return;
	}
	dst = bufferevent_get_output(partner);
	evbuffer_add_buffer(dst, src);
	struct proxy *p_l = new_proxy_buf(bev);

	if (evbuffer_get_length(dst) >= MAX_OUTPUT) {
		/* We're giving the other side data faster than it can
		 * pass it on.  Stop reading here until we have drained the
		 * other side to MAX_OUTPUT/2 bytes. */
		bufferevent_setcb(partner, xfrp_read_cb, drained_writecb,
		    xfrp_event_cb, p_l);
		bufferevent_setwatermark(partner, EV_WRITE, MAX_OUTPUT/2,
		    MAX_OUTPUT);
		bufferevent_disable(bev, EV_READ);
	}
}

static void drained_writecb(struct bufferevent *bev, void *ctx)
{
	struct proxy *p = (struct proxy *)ctx;
	struct bufferevent *partner = p?p->bev:NULL;

	/* We were choking the other side until we drained our outbuf a bit.
	 * Now it seems drained. */
	bufferevent_setcb(bev, xfrp_read_cb, NULL, xfrp_event_cb, p);
	bufferevent_setwatermark(bev, EV_WRITE, 0, 0);
	if (partner)
		bufferevent_enable(partner, EV_READ);
}

static void close_on_finished_writecb(struct bufferevent *bev, void *ctx)
{
	struct evbuffer *b = bufferevent_get_output(bev);

	if (evbuffer_get_length(b) == 0) {
		bufferevent_free(bev);
	}
}

static void xfrp_event_cb(struct bufferevent *bev, short what, void *ctx)
{
	struct proxy *p = (struct proxy *)ctx;
	struct bufferevent *partner = p?p->bev:NULL;

	if (what & (BEV_EVENT_EOF|BEV_EVENT_ERROR)) {
		debug(LOG_DEBUG, "working connection closed!");
		if (partner) {
			/* Flush all pending data */
			xfrp_read_cb(bev, p);

			if (evbuffer_get_length(bufferevent_get_output(partner))) {
				/* We still have to flush data from the other
				 * side, but when that's done, close the other
				 * side. */
				bufferevent_setcb(partner,
				    NULL, close_on_finished_writecb,
				    xfrp_event_cb, NULL);

				bufferevent_disable(partner, EV_READ);
			} else {
				/* We have nothing left to say to the other
				 * side; close it. */
				bufferevent_free(partner);
				free_proxy(p);
			}
		}
		bufferevent_free(bev);
	}
}

// return: 0: init succeed; 1: init failed
// static int init_ftp_data_port(struct proxy_service *ps)
// {
// 	struct mycurl_string ret_buf;
// 	if ( ! mycurl_string_init(&ret_buf)) {
// 		debug(LOG_ERR, "error: ftp data port buffer init failed!");
// 		return 1;
// 	}

// 	char url[1024] = {0};
// 	struct common_conf *c_conf = get_common_config();
	

// 	int state_code = 0;
// 	double down_size = 0;
// 	int ret = net_visit(url, 
// 			&ret_buf,
// 			HTTP_GET,
// 			NULL,
// 			60l, 
// 			&state_code,
// 			&down_size);

// 	if (ret) {
// 		debug(LOG_ERR, "error: ftp remote data port init failed by HTTP GET");
// 		mycurl_string_free(&ret_buf);
// 		return 1;
// 	}

// 	return 0;
// }

int is_ftp_proxy(const struct proxy_service *ps)
{
	if (! ps || ! ps->proxy_type)
		return 0;

	if (0 == strcmp(ps->proxy_type, "ftp") && ps->remote_data_port > 0)
		return 1;

	return 0;
}

// create frp tunnel for service
void start_xfrp_tunnel(struct proxy_client *client)
{
	if (! client->ctl_bev) {
		debug(LOG_ERR, "proxy client control bev is invalid!");
		return;
	}

	struct event_base *base = client->base;
	struct common_conf *c_conf = get_common_config();
	struct proxy_service *ps = client->ps;

	if (! base) {
		debug(LOG_ERR, "service event base get failed");
		return;
	}

	if (! ps) {
		debug(LOG_ERR, "service tunnel started failed, no proxy service resource.");
		return;
	}

	if (! ps->local_port) {
		debug(LOG_ERR, "service tunnel started failed, proxy service resource unvalid.");
		return;
	}

	client->local_proxy_bev = connect_server(base, ps->local_ip, ps->local_port);
	if (!client->local_proxy_bev) {
		debug(LOG_ERR, "frpc tunnel connect local proxy port [%d] failed!", ps->local_port);
		bufferevent_free(client->ctl_bev);
		return;
	}
	
	debug(LOG_DEBUG, "proxy server [%s:%d] <---> client [%s:%d]", 
		  c_conf->server_addr, 
		  ps->remote_port, 
		  ps->local_ip ? ps->local_ip:"::1",
		  ps->local_port);

	struct proxy *ctl_prox = new_proxy_buf(client->ctl_bev);
	struct proxy *local_prox = new_proxy_buf(client->local_proxy_bev);
	bufferevent_data_cb proxy_s2c_cb, proxy_c2s_cb;
	if (is_ftp_proxy(client->ps)) {
		proxy_c2s_cb = ftp_proxy_c2s_cb;
		proxy_s2c_cb = ftp_proxy_s2c_cb;
		ctl_prox->remote_data_port = client->ps->remote_data_port;
		ctl_prox->proxy_name = strdup(ps->proxy_name);
	} else {
		proxy_c2s_cb = tcp_proxy_c2s_cb;
		proxy_s2c_cb = tcp_proxy_s2c_cb;
	}

	bufferevent_setcb(client->ctl_bev, 
						proxy_s2c_cb, 
						NULL, 
						xfrp_event_cb, 
						local_prox);

	bufferevent_setcb(client->local_proxy_bev, 
						proxy_c2s_cb, 
						NULL, 
						xfrp_event_cb, 
						ctl_prox);
						
	bufferevent_enable(client->ctl_bev, EV_READ|EV_WRITE);
	bufferevent_enable(client->local_proxy_bev, EV_READ|EV_WRITE);
}

int send_client_data_tail(struct proxy_client *client)
{
	int send_l = 0;
	if (client->data_tail && client->data_tail_size && client->local_proxy_bev) {
		send_l = bufferevent_write(client->local_proxy_bev, client->data_tail, client->data_tail_size);
	}

	return send_l;
}

void free_proxy_client(struct proxy_client *client)
{
	if (client->local_ip) free(client->local_ip);
	
	free_base_config(client->bconf);
	
	evtimer_del(client->ev_timeout);
}

void del_proxy_client(struct proxy_client *client)
{
	struct proxy_client *all_pc = get_all_pc();
	if (!client || !all_pc ) {
		debug(LOG_INFO, "Error: all_pc or client is NULL");
		return;
	}
	
	HASH_DEL(all_pc, client);
	
	free_proxy_client(client);
}

// Return NULL if proxy service not found with proxy_name
struct proxy_service *get_proxy_service(const char *proxy_name)
{
	struct proxy_service *ps = NULL;
	struct proxy_service *all_ps = get_all_proxy_services();
	HASH_FIND_STR(all_ps, proxy_name, ps);
	return ps;
}

struct proxy_client *new_proxy_client()
{
	struct proxy_client *client = calloc(1, sizeof(struct proxy_client));
	assert(client);
	return client;
}