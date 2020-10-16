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

/** @file config.h
    @brief xfrp client config related
    @author Copyright (C) 2016 Dengfeng Liu <liudengfeng@kunteng.org>
*/
#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "client.h"
#include "common.h"

#define FTP_RMT_CTL_PROXY_SUFFIX	"_ftp_remote_ctl_proxy"

struct base_conf{
	char	*name;
	char	*auth_token;
	int		use_encryption;
	int		use_gzip;
	int		privilege_mode;
	char	*privilege_token;
	int		pool_count;
	char	*host_header_rewrite;
	char	*subdomain;
};

// common config
struct common_conf {
	char	*server_addr; 	/* default 0.0.0.0 */
	char 	*server_ip;
	int		server_port; 	/* default 7000 */
	char	*http_proxy;
	char	*log_file; 		/* default consol */
	char	*log_way; 		/* default console */
	char	*log_level; 	/* default info */
	int		log_max_days;	/* default 3 */
	char	*privilege_token;
	char	*auth_token;
	int		heartbeat_interval; /* default 10 */
	int		heartbeat_timeout;	/* default 30 */
	int 	tcp_mux;		/* default 0 */
	char	*user;

	/* private fields */
	int 	is_router;	// to sign router (Openwrt/LEDE) or not
};

struct common_conf *get_common_config();
struct proxy_service *get_all_proxy_services();

void free_common_config();

void free_base_config(struct base_conf *bconf);

struct proxy_client *get_all_pc();

void load_config(const char *confile);
char *get_ftp_data_proxy_name(const char *ftp_proxy_name);
void set_common_server_ip(const char *ip);
int is_running_in_router();

#endif //_CONFIG_H_
