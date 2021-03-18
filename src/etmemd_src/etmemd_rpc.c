/******************************************************************************
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * etmem is licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 * http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v2 for more details.
 * Author: louhongxiang
 * Create: 2019-12-10
 * Description: Interaction API between server and client.
 ******************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <signal.h>
#include "securec.h"
#include "etmemd_rpc.h"
#include "etmemd_project.h"
#include "etmemd_common.h"
#include "etmemd_log.h"

/* the max length of sun_path in struct sockaddr_un is 108 */
#define RPC_ADDR_LEN_MAX  108
#define RPC_TIMEOUT_LIMIT 10
#define RPC_CLIENT_MAX    1
#define RPC_BUFF_LEN_MAX  512

static bool g_exit = true;
static char *g_sock_name = NULL;
static int g_sock_fd;
struct server_rpc_params g_rpc_params;

struct rpc_resp_msg {
    enum opt_result result;
    const char *msg;
};

struct server_rpc_parser g_rpc_parser[] = {
    {"proj_name", DECODE_STRING, (void **)&(g_rpc_params.proj_name)},
    {"file_name", DECODE_STRING, (void **)&(g_rpc_params.file_name)},
    {"cmd", DECODE_INT, (void **)&(g_rpc_params.cmd)},
    {NULL, DECODE_END, NULL},
};

struct rpc_resp_msg g_resp_msg_arr[] = {
    {OPT_SUCCESS, "success"},
    {OPT_INVAL, "error: invalid parameters"},
    {OPT_PRO_EXISTED, "error: project has been existed"},
    {OPT_PRO_STARTED, "error: project has been started"},
    {OPT_PRO_STOPPED, "error: project has been stopped"},
    {OPT_PRO_NOEXIST, "error: project is not exist"},
    {OPT_INTER_ERR, "error: etmemd has internal error"},
    {OPT_RET_END, NULL},
};

static void etmemd_set_flag(int s)
{
    etmemd_log(ETMEMD_LOG_ERR, "caught signal %d\n", s);
    g_exit = true;
    close(g_sock_fd);
    g_sock_fd = -1;
}

void etmemd_handle_signal(void)
{
    signal(SIGINT, etmemd_set_flag);
    signal(SIGTERM, etmemd_set_flag);

    return;
}

static enum opt_result etmemd_switch_cmd(const struct server_rpc_params svr_param)
{
    enum opt_result ret = OPT_INVAL;

    switch (svr_param.cmd) {
        case PROJ_ADD:
            ret = etmemd_project_add(svr_param.proj_name, svr_param.file_name);
            return ret;
        case PROJ_DEL:
            ret = etmemd_project_delete(svr_param.proj_name);
            return ret;
        case PROJ_SHOW:
            ret = etmemd_project_show();
            return ret;
        case MIG_START:
            ret = etmemd_migrate_start(svr_param.proj_name);
            return ret;
        case MIG_STOP:
            ret = etmemd_migrate_stop(svr_param.proj_name);
            return ret;
        default:
            etmemd_log(ETMEMD_LOG_ERR, "Invalid command.\n");
            return ret;
    }
}

int etmemd_parse_sock_name(const char *sock_name)
{
    if (sock_name == NULL || strlen(sock_name) == 0) {
        return -1;
    }

    /* socket name which listened to need to add 0 at the beginning of the string,
     * so we minus 1 here */
    if (strlen(sock_name) >= RPC_ADDR_LEN_MAX - 1) {
        etmemd_log(ETMEMD_LOG_ERR, "length of socket name to listen should be less than %d\n",
                   RPC_ADDR_LEN_MAX - 1);
        return -1;
    }

    g_sock_name = (char *)calloc(RPC_ADDR_LEN_MAX, sizeof(char));
    if (g_sock_name == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "malloc for socket name failed!\n");
        return -1;
    }

    if (sprintf_s(g_sock_name, RPC_ADDR_LEN_MAX, "@%s", sock_name) <= 0) {
        etmemd_log(ETMEMD_LOG_ERR, "sprintf for socket name failed!\n");
        etmemd_safe_free((void **)&g_sock_name);
        return -1;
    }
    return 0;
}

bool etmemd_sock_name_set(void)
{
    return g_sock_name != NULL;
}

void etmemd_sock_name_free(void)
{
    etmemd_safe_free((void **)&g_sock_name);
}

static int etmemd_rpc_set(int sock_fd)
{
    int rc;
    int buf_len = RPC_BUFF_LEN_MAX;
    struct timeval timeout = {RPC_TIMEOUT_LIMIT, 0};

    /* set timeout limit to socket for sending */
    rc = setsockopt(sock_fd,
                    SOL_SOCKET,
                    SO_SNDTIMEO,
                    (const char *)&timeout,
                    sizeof(timeout));
    if (rc < 0) {
        etmemd_log(ETMEMD_LOG_ERR, "set send timeout for socket failed, err(%s)\n",
                   strerror(errno));
        return -1;
    }

    /* set max length of buffer to recive */
    rc = setsockopt(sock_fd,
                    SOL_SOCKET,
                    SO_RCVBUF,
                    (const char *)&buf_len,
                    sizeof(buf_len));
    if (rc < 0) {
        etmemd_log(ETMEMD_LOG_ERR, "set recive buffer length for socket failed, err(%s)\n",
                   strerror(errno));
        return -1;
    }

    /* set max length of buffer to send */
    rc = setsockopt(sock_fd,
                    SOL_SOCKET,
                    SO_SNDBUF,
                    (const char *)&buf_len,
                    sizeof(buf_len));
    if (rc < 0) {
        etmemd_log(ETMEMD_LOG_ERR, "set send buffer length for socket failed, err(%s)\n",
                   strerror(errno));
        return -1;
    }

    return 0;
}

static int etmemd_rpc_init(void)
{
    int sock_fd = -1;
    struct sockaddr_un sock_addr; 
    size_t sock_len;

    if (memset_s(&sock_addr, sizeof(struct sockaddr_un),
                 0, sizeof(struct sockaddr_un)) != EOK) {
        etmemd_log(ETMEMD_LOG_ERR, "fail to memset for sockaddr\n");
        return -1;
    }

    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        etmemd_log(ETMEMD_LOG_ERR, "create socket for fail, error(%s)\n",
                   strerror(errno));
        return -1;
    }

    sock_len = strlen(g_sock_name);
    sock_addr.sun_family = AF_UNIX;
    if (memcpy_s(sock_addr.sun_path, sizeof(sock_addr.sun_path),
                 g_sock_name, sock_len) != EOK) {
        etmemd_log(ETMEMD_LOG_ERR, "copy for sockect name to sun_path fail, error(%s)\n",
                   strerror(errno));
        close(sock_fd);
        return -1;
    }
    sock_addr.sun_path[0] = 0;
    sock_len += offsetof(struct sockaddr_un, sun_path);

    if (etmemd_rpc_set(sock_fd) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "set for socket fail, error(%s)\n", strerror(errno));
        close(sock_fd);
        return -1;
    }
    if (bind(sock_fd, (struct sockaddr *)&sock_addr, sock_len) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "socket bind %s fail, error(%s)\n",
                   (char *)&sock_addr.sun_path[1], strerror(errno));
        close(sock_fd);
        return -1;
    }

    return sock_fd;
}

static char *etmemd_rpd_get_param_str(const char *buf, unsigned long buf_len, unsigned long *idx)
{
    size_t start;
    size_t str_len;
    char *param_str = NULL;

    for (start = *idx; start < buf_len; start++) {
        if (buf[start] == ' ') {
            break;
        }
    }

    str_len = start - *idx;
    /* str_len equals 0 means that only get ' ' or the end of string after parsing */
    if (str_len == 0) {
        etmemd_log(ETMEMD_LOG_ERR, "parse %s for parameter fail\n", &buf[*idx]);
        return param_str;
    }

    param_str = (char *)calloc(str_len + 1, sizeof(char));
    if (param_str == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "malloc for string to parse %s fail\n", &buf[*idx]);
        return param_str;
    }

    if (strncpy_s(param_str, str_len + 1, &buf[*idx], str_len) != EOK) {
        etmemd_log(ETMEMD_LOG_ERR, "strncpy for parameter %s fail", &buf[*idx]);
        etmemd_safe_free((void **)&param_str);
        return NULL;
    }

    /* skip the ' ', so we add (size_t)start here */
    *idx = start + 1;
    return param_str;
}

static int etmemd_rpc_decode_str(void **ptr, const char *buf,
                                 unsigned long buf_len, unsigned long *idx)
{
    *ptr = etmemd_rpd_get_param_str(buf, buf_len, idx);
    if (*ptr == NULL) {
        return -1;
    }
    return 0;
}

static int etmemd_rpc_decode_int(void **ptr, const char *buf,
                                 unsigned long buf_len, unsigned long *idx)
{
    char *tmp_str = NULL;
    int tmp_type;

    tmp_str = etmemd_rpd_get_param_str(buf, buf_len, idx);
    if (tmp_str == NULL) {
        return -1;
    }

    if (get_int_value(tmp_str, &tmp_type) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "transform %s to int fail\n", tmp_str);
        free(tmp_str);
        return -1;
    }

    *(int *)ptr = tmp_type;
    free(tmp_str);
    return 0;
}

static int etmemd_rpc_decode(struct server_rpc_parser *parser, const char *buf,
                             unsigned long buf_len, unsigned long *idx)
{
    int ret;

    switch (parser->type) {
        case DECODE_STRING:
            ret = etmemd_rpc_decode_str(parser->member_ptr, buf, buf_len, idx);
            break;
        case DECODE_INT:
            ret = etmemd_rpc_decode_int(parser->member_ptr, buf, buf_len, idx);
            break;
        default:
            etmemd_log(ETMEMD_LOG_ERR, "invalide type for rpc to parse\n");
            ret = -1;
            break;
    }

    return ret;
}

static int etmemd_rpc_parse(const char *buf, unsigned long buf_len)
{
    unsigned long idx = 0;
    unsigned long str_len;
    int i = 0;
    int ret = -1;

    if (memset_s(&g_rpc_params, sizeof(g_rpc_params),
                 0, sizeof(g_rpc_params)) != EOK) {
        etmemd_log(ETMEMD_LOG_ERR, "memset for rpc parser fail\n");
        return ret;
    }

    while (g_rpc_parser[i].name != NULL) {
        /* check for the name of parameter before parse the value of parameter */
        str_len = strlen(g_rpc_parser[i].name);
        if (idx >= buf_len) {
            etmemd_log(ETMEMD_LOG_ERR, "idx: %lu is too long, buf may overflow.", idx);
            ret = -1;
            break;
        }

        if (strncmp(&buf[idx], g_rpc_parser[i].name, str_len) != 0) {
            etmemd_log(ETMEMD_LOG_ERR, "get wrong param name \"%s\" which should be \"%s\"",
                       &buf[idx], g_rpc_parser[i].name);
            ret = -1;
            break;
        }
        /* add 1 to skip the seperator of key and value */
        idx += (str_len + 1);
        ret = etmemd_rpc_decode(&g_rpc_parser[i], buf, buf_len, &idx);
        if (ret != 0) {
            etmemd_log(ETMEMD_LOG_ERR, "decode \"%s\" from rpc fail\n",
                       g_rpc_parser[i].name);
            break;
        }
        i++;
    }
    return ret;
}

static void free_server_rpc_params(struct server_rpc_params *svr)
{
    if (svr->proj_name != NULL) {
        free(svr->proj_name);
        svr->proj_name = NULL;
    }

    if (svr->file_name != NULL) {
        free(svr->file_name);
        svr->file_name = NULL;
    }
}

static void etmemd_rpc_send_response_msg(int sock_fd, enum opt_result result)
{
    int i = 0;
    ssize_t ret = -1;

    while (g_resp_msg_arr[i].msg != NULL) {
        if (result != g_resp_msg_arr[i].result) {
            i++;
            continue;
        }

        ret = send(sock_fd, g_resp_msg_arr[i].msg, strlen(g_resp_msg_arr[i].msg), 0);
        break;
    }

    if (ret < 0) {
        etmemd_log(ETMEMD_LOG_ERR, "send response to client fail, error(%s)\n",
                   strerror(errno));
    }
    return;
}

static void etmemd_rpc_handle(int sock_fd)
{
    enum opt_result ret;

    ret = etmemd_switch_cmd(g_rpc_params);
    if (ret != OPT_SUCCESS) {
        etmemd_log(ETMEMD_LOG_ERR, "operate cmd %d fail\n", g_rpc_params.cmd);
    }

    etmemd_rpc_send_response_msg(sock_fd, ret);
    free_server_rpc_params(&g_rpc_params);
    return;
}

static int check_socket_permission(int sock_fd) {
    struct ucred cred;
    socklen_t len;
    ssize_t rc;

    len = sizeof(struct ucred);

    rc = getsockopt(sock_fd,
                    SOL_SOCKET,
                    SO_PEERCRED,
                    &cred,
                    &len);
    if (rc < 0) {
        etmemd_log(ETMEMD_LOG_ERR, "getsockopt failed, err(%s)\n",
                   strerror(errno));
        return -1;
    }

    if (cred.uid != 0 || cred.gid != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "client socket connect failed, permition denied\n");
        return -1;
    }

    return 0;
}

static int etmemd_rpc_accept(int sock_fd)
{
    char *recv_buf = NULL;
    int accp_fd = -1;
    ssize_t rc;
    int ret = -1;

    recv_buf = (char *)calloc(RPC_BUFF_LEN_MAX, sizeof(char));
    if (recv_buf == NULL) {
        etmemd_log(ETMEMD_LOG_ERR, "malloc for buffer to recive socket message fail\n");
        return -1;
    }

    accp_fd = accept(sock_fd, NULL, NULL);
    if (accp_fd < 0) {
        etmemd_log(ETMEMD_LOG_ERR, "accept for socket %s fail, error(%s)\n",
                   g_sock_name, strerror(errno));
        free(recv_buf);
        /* wait for next accept round, do not return -1 to stop listening */
        return 0;
    }

    rc = check_socket_permission(accp_fd);
    if (rc != 0) {
        goto RPC_EXIT;
    }

    rc = recv(accp_fd, recv_buf, RPC_BUFF_LEN_MAX, 0);
    if (rc <= 0) {
        etmemd_log(ETMEMD_LOG_WARN, "socket recive from client fail, error(%s)\n",
                   strerror(errno));
        goto RPC_EXIT;
    }

    if (rc > RPC_BUFF_LEN_MAX) {
        etmemd_log(ETMEMD_LOG_WARN, "buffer sent to etmemd is too long, better less than %d\n",
                   RPC_BUFF_LEN_MAX);
        goto RPC_EXIT;
    }

    etmemd_log(ETMEMD_LOG_INFO, "etmemd get socket message \"%s\"\n", recv_buf);
    if (etmemd_rpc_parse(recv_buf, (unsigned long)rc) == 0) {
        etmemd_rpc_handle(accp_fd);
    }
    ret = 0;

RPC_EXIT:
    free(recv_buf);
    close(accp_fd);
    return ret;
}

int etmemd_rpc_server(void)
{
    if (!etmemd_sock_name_set()) {
        etmemd_log(ETMEMD_LOG_ERR, "socket name of rpc must be provided\n");
        return -1;
    }

    g_exit = false;
    etmemd_log(ETMEMD_LOG_INFO, "start rpc for etmemd\n");

    g_sock_fd = etmemd_rpc_init();
    if (g_sock_fd < 0) {
        etmemd_safe_free((void **)&g_sock_name);
        return -1;
    }
    
    /* allow RPC_CLIENT_MAX clients to connect at the same time */
    if (listen(g_sock_fd, RPC_CLIENT_MAX) != 0) {
        etmemd_log(ETMEMD_LOG_ERR, "etmemd listen to socket %s fail, error(%s)\n",
                   g_sock_name, strerror(errno));
        etmemd_safe_free((void **)&g_sock_name);
        return -1;
    }

    while (!g_exit) {
        if (etmemd_rpc_accept(g_sock_fd) != 0) {
            etmemd_log(ETMEMD_LOG_ERR, "handle remote call failed once, error(%s)\n",
                       strerror(errno));
        }
        etmemd_log(ETMEMD_LOG_INFO, "get one connection from etmem\n");
    }
    etmemd_safe_free((void **)&g_sock_name);

    etmemd_log(ETMEMD_LOG_INFO, "close rpc done\n");
    return 0;
}

