#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#include "config.h"
#include "ev.h"
#include "ycommon_type.h"
#include "ydm_types.h"
#include "ydm_log.h"
#include "ydm_stream.h"
#include "yremote_command.h"

#define TERMINAL_GENERAL_REPLY          (0x0001) /* �ն�ͨ��Ӧ�� */
#define PLATFORM_GENERAL_REPLY          (0x8001) /* ƽ̨ͨ��Ӧ�� */
#define TERMINAL_OFFLINE_REQUEST        (0x0000) /* �ն�������Ϣ */
#define TERMINAL_HEARTBEAT_REQUEST      (0x0002) /* �ն��������� */
#define TERMINAL_REGISTER_REQUEST       (0x0100) /* �ն�ע������ */
#define PLATFORM_REGISTER_REPLY         (0x8100) /* �ն�ע��Ӧ�� */
#define TERMINAL_LOGIN_REQUEST          (0x0102) /* �ն˵�¼���� */
#define TERMINAL_INFO_REQUEST           (0x0109) /* �ն���Ϣ���� */
#define PLATFORM_UPGRADE_REQUEST        (0X8024) /* ����֪ͨ���� */
#define TERMINAL_UPGRADE_REPLAY         (0x0024) /* ����֪ͨӦ�� */
#define PLATFORM_UPGRADE_CANCEL_REQUEST (0X8027) /* ��������ȡ������ */
#define TERMINAL_UPGRADE_STATUS_REPLAY  (0x0028) /* ��������״̬�ϱ� */
#define PLATFORM_TRANSPARENT_REQUEST    (0x8900) /* ƽ̨͸����Ϣ���� */ 
#define TERMINAL_TRANSPARENT_REQUEST    (0x0900) /* �ն�͸����Ϣ���� */ 
#define PLATFORM_TRANSPARENT_REPLY      (0x8001) /* �Զ�����չƽ̨ͨ��Ӧ���Ӧ0x0900 */
#define TERMINAL_TRANSPARENT_REPLY      (0x0001) /* �Զ�����չ�ն�ͨ��Ӧ���Ӧ0x8900 */

/* ƽ̨����״̬ */
enum {
    TCS_DISCONNECT = 0,
    TCS_CONNECT_PROCESS = 1,
    TCS_CONNECT = 2,
};

/* �Զ���͸����Ϣ���� */
#define TRANSPARENT_MESSAGE_TYPE (0xF1)

/* ע��������Ӧ������ */
#define TCS_REGISTER_REPLY_CODE_SUCCESS           (0)
#define TCS_REGISTER_REPLY_CODE_VEHIVLE_ONLINE    (1)
#define TCS_REGISTER_REPLY_CODE_VEHIVLE_NOTFOUND  (2)
#define TCS_REGISTER_REPLY_CODE_TREMINAL_ONLINE   (3)
#define TCS_REGISTER_REPLY_CODE_TREMINAL_NOTFOUND (4)

/* ƽ̨ͨ��Ӧ������� */
#define TCS_GENERAL_REPLY_CODE_SUCCESS     (0)
#define TCS_GENERAL_REPLY_CODE_FAILED      (1)
#define TCS_GENERAL_REPLY_CODE_MESSAGE_ERR (2)
#define TCS_GENERAL_REPLY_CODE_UNSUPPORT   (3)

/* ƽ̨ͨ��Ӧ�������� */
typedef enum platform_general_reply_e {
    TCS_PGR_INVALID = 0,
    TCS_PGR_AUTHORIZATION = 1,
} platform_general_reply_e;

#define DEVICE_IDENTIFIER_LENGTH  (10) /* �豸��ʶ������ */
#define TCS_RXTX_BUFF_SIZE_DEF (10240)    

#define TCS_SET_BODY_LEN(attr, len) (attr &= (~0x03ff), attr |= (len & 0x03ff), attr)
#define TCS_GET_BODY_LEN(attr)      (attr & (0x03ff))

#define TCS_SET_SUB_PACKET(attr, sub) (attr |= (sub & 0x01) << 13)
#define TCS_GET_SUB_PACKET(attr)      (attr & (0x01 << 13))

typedef struct tcs_msg_header_s {
    yuint16 id;
    yuint16 attr;
    yuint8 version;
    yuint8 devid[DEVICE_IDENTIFIER_LENGTH];
    yuint16 msgnum;
    struct tcs_msg_packet_info_s {
        yuint16 total;
        yuint16 number;
    } packet_info;
} tcs_msg_header_t;

typedef struct tcs_msg_s {
    yuint8 flaghead; 
    tcs_msg_header_t header;
    yuint8 body[TCS_RXTX_BUFF_SIZE_DEF];
    yuint32 bodylen;
    yuint8 checksum;
    yuint8 flagtail; 
} tcs_msg_t;

#define DEVINFO_IMEI_LEN          (20)
#define DEVINFO_VIN_LEN           (17)
#define DEVINFO_DEV_NUMBER_SIZE   (30)
#define DEVINFO_DEV_MODEL_SIZE    (30)
#define DEVINFO_MANUFACTURER_SIZE (11)
#define DEVINFO_PHONE_NUMBER_SIZE (6)
typedef struct tcs_register_body_s {
    yuint16 provinceid;
    yuint16 cityid;
    yuint8 manufacturer[DEVINFO_MANUFACTURER_SIZE];
    yuint8 model[DEVINFO_DEV_MODEL_SIZE];
    yuint8 terid[DEVINFO_DEV_NUMBER_SIZE];
    yuint8 color;
    yuint8 vin[DEVINFO_VIN_LEN];
} tcs_register_body_t;

typedef struct terminal_control_service_s {
    boolean isruning; /* �ɹ�����ƽ̨����ͨ�� */
    struct ev_loop *loop; /* ���¼�ѭ���ṹ�� */
    ev_io tcprecv_watcher; /* ����TCP���¼� */
    ev_io tcpsend_watcher; /* ����TCPд�¼� */    
    ev_timer keepalive_watcher; /* ������ʱ�� */
    ev_timer reconnect_watcher; /* ������ʱ�� */
#define TCS_KEEPALIVE_INTERVAL_DEF (150) /* default 2.5 min */
    yuint32 keepalive_interval; /* ����������ʱ����unit s */ 
    int index; /* ������� */
    int sockfd; /* tcp socket�ļ������� */    
    int sock_stat; /* socket��ǰ״̬ */
    yuint8 *rxbuf; /* ����Buff */
    yuint32 rxlen; /* ����Buff���� */
    yuint32 recvlen; /* ����buff�ڴ��ڵ���Ч���ݳ��� */
    yuint8 *txbuf; /* ����Buff */
    yuint32 txlen; /* ����buff���� */
    yuint32 msgnum; /* ��Ϣ��ˮ��0��ʼ */
    platform_general_reply_e pgr;
    tcs_msg_t *tcsmsgt; /* ����Э������ṹ����ʱ���� */
#define REMOTE_CMD_HANDLER_SIZE (100) /* �����ע����ٸ�ָ����� */
    struct remote_cmd_handler_s {
        yuint16 msgid; /* Զ��ָ����� */
        premote_cmd_handler handler; /* ������ */
        void *argv; /* �û�����ָ�� */
    } rc_handler[REMOTE_CMD_HANDLER_SIZE];
    terminal_control_service_info_t tcs_config; /* һЩ���ò������ⲿ���ú������� */
#define TCS_AUTHCODE_SIZE_MAX (512)
    yuint8 authcode[TCS_AUTHCODE_SIZE_MAX]; /* ע��ɹ���ƽ̨������ע���� */
#define TCS_PROTOCOL_VERSION (1)    
    yuint8 protocol_version;  /* Э��汾 */
#define RECONNECT_DELAY_TIME_MAX (600) /* �������ʱʱ��600�� */
    int reconnect_num; /* �������� */
}terminal_control_service_t;

static int tcs_register_request(terminal_control_service_t *tcs);
static int tcs_terminal_offline_request(terminal_control_service_t *tcs);
static void tcs_keepalive_timer_refresh(terminal_control_service_t *tcs);
static void tcs_ev_timer_server_reconnect_handler(struct ev_loop *loop, ev_timer *w, int revents);
static void tcs_tcp_connect_abnormal_handler(terminal_control_service_t *tcs);
static int tcs_reconnect_server(terminal_control_service_t *tcs, int delay);

static yuint8 *tcs_rx_buff_pointer(terminal_control_service_t *tcs)
{
    return tcs->rxbuf;
}

static yuint32 tcs_rx_buff_size(terminal_control_service_t *tcs)
{
    return tcs->rxlen;
}

static yuint8 *tcs_tx_buff_pointer(terminal_control_service_t *tcs)
{
    return tcs->txbuf;
}

static yuint32 tcs_tx_buff_size(terminal_control_service_t *tcs)
{
    return tcs->txlen;
}

static void tcs_disconnect(terminal_control_service_t *tcs)
{
    log_d("tcs tcp disconnect");
    memset(tcs_rx_buff_pointer(tcs), 0, tcs_rx_buff_size(tcs));
    memset(tcs_tx_buff_pointer(tcs), 0, tcs_tx_buff_size(tcs));
    tcs->recvlen = 0;    
    tcs->sock_stat = TCS_DISCONNECT;        
    ev_timer_stop(tcs->loop, &tcs->reconnect_watcher);
    ev_timer_stop(tcs->loop, &tcs->keepalive_watcher);
    ev_io_stop(tcs->loop, &tcs->tcprecv_watcher);
    ev_io_stop(tcs->loop, &tcs->tcpsend_watcher);
    if (tcs->sockfd > 0) {
        close(tcs->sockfd);
        tcs->sockfd = -1;
    }      
}

int tcs_tcp_send(terminal_control_service_t *tcs, yuint8 *msg, yuint32 size)
{
    int bytesSent = -1;
    fd_set writefds;
    struct timeval timeout;
    int sret = 0;

    if (!(tcs->sockfd > 0)) {
        return -1;
    }

    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    FD_ZERO(&writefds);
    FD_SET(tcs->sockfd, &writefds);
    if ((sret = select(tcs->sockfd + 1, NULL, &writefds, NULL, &timeout)) > 0 &&\
        FD_ISSET(tcs->sockfd, &writefds)) {
        bytesSent = send(tcs->sockfd, msg, size, 0);
    }

    log_hex_d("send", msg, size);
        
    return bytesSent;
}

static int tcs_tcp_create()
{
    int sockfd = -1;
    int flag = 0;

#ifdef __linux__    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
#endif /* __linux__ */
    if (!(sockfd > 0)) {
        return -1;
    }
    flag = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flag | O_NONBLOCK);
       
    return sockfd;
}

static boolean tcs_tcp_connect_check(terminal_control_service_t *tcs)
{
    fd_set writefds;
    fd_set readfds;
    struct timeval timeout;
    int sret = 0;
    int error = 0;
    socklen_t len = sizeof(error);

    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    FD_ZERO(&writefds);
    FD_SET(tcs->sockfd, &writefds);
    FD_ZERO(&readfds);
    FD_SET(tcs->sockfd, &readfds);
    if ((sret = select(tcs->sockfd + 1, &readfds, &writefds, NULL, &timeout)) > 0) {
        if (FD_ISSET(tcs->sockfd, &writefds) &&\
            FD_ISSET(tcs->sockfd, &readfds)) {
            if (getsockopt(tcs->sockfd, SOL_SOCKET, SO_ERROR, (void *)&error, &len) < 0 || \
                error != 0) {
                return false;
            }
            else {                
                return true;
            }
        }
        else if (FD_ISSET(tcs->sockfd, &writefds)) {            
            return true;
        }
        else if (FD_ISSET(tcs->sockfd, &readfds)) {        
            return false;
        }
    }

    return false;
}

static void tcs_tcp_connect_success_handler(terminal_control_service_t *tcs)
{
    tcs->sock_stat = TCS_CONNECT;
    /* ���ӳɹ���������ʼע�� */
    tcs_register_request(tcs);
}

/* �����ƽ̨�ķ�����ͨ�Ŵ��� */
static void tcs_tcp_connect_abnormal_handler(terminal_control_service_t *tcs)
{   
    /* �Ͽ� socket ���� */
    tcs_disconnect(tcs); 
    
    /* ������������ */
    tcs->reconnect_num++;

    /* �������� */
    tcs_reconnect_server(tcs, tcs->reconnect_num * 30 > RECONNECT_DELAY_TIME_MAX ? \
                                RECONNECT_DELAY_TIME_MAX : tcs->reconnect_num * 30);
}

static void tcs_tcp_connect_process_handler(terminal_control_service_t *tcs)
{
    if (tcs_tcp_connect_check(tcs)) {
        log_d("tcs tcp connect success");
        tcs_tcp_connect_success_handler(tcs);
    }
    else {        
        log_d("tcs tcp connect failed"); 
        tcs_tcp_connect_abnormal_handler(tcs);
    }
    ev_io_stop(tcs->loop, &tcs->tcpsend_watcher);

    return ;
}

int tcs_message_decode(yuint8 *buf, yuint32 buflen, tcs_msg_t *msg)
{
    stream_t sp;
    stream_t tmpsp;
    yuint8 *tmpbuf = ymalloc(buflen);
    if (tmpbuf == 0) {
        return -1;
    }

    stream_init(&sp, buf, buflen);
    stream_init(&tmpsp, tmpbuf, buflen);
    stream_byte_read(&sp); /* ��ȡ�ʼ�ı�־��0x7e */
    for ( ; stream_left_len(&sp) > 1; ) { /* ����Ҫ�ж����һ����־�� */
        yuint8 c = stream_byte_read(&sp);
        if (c == 0x7d) {
            yuint8 b = stream_byte_read(&sp);
            if (b == 0x01) {
                stream_byte_write(&tmpsp, 0x7d);
            } 
            else if (b == 0x02) {
                stream_byte_write(&tmpsp, 0x7e);
            }
            else {
                stream_byte_write(&tmpsp, c);
                stream_byte_write(&tmpsp, b);
            }
        }
        else {
            stream_byte_write(&tmpsp, c);
        }
    }

    stream_init(&tmpsp, tmpbuf, buflen);
    msg->header.id = stream_2byte_read(&tmpsp);
    msg->header.attr = stream_2byte_read(&tmpsp);
    msg->header.version = stream_byte_read(&tmpsp);
    stream_nbyte_read(&tmpsp, msg->header.devid, sizeof(msg->header.devid));
    msg->header.msgnum = stream_2byte_read(&tmpsp);
    if (TCS_GET_SUB_PACKET(msg->header.attr)) {
        msg->header.packet_info.total = stream_2byte_read(&tmpsp);
        msg->header.packet_info.number = stream_2byte_read(&tmpsp);
    }
    msg->bodylen = TCS_GET_BODY_LEN(msg->header.attr);
    stream_nbyte_read(&tmpsp, msg->body, msg->bodylen);
    msg->checksum = stream_byte_read(&sp);

    if (tmpbuf) {
        yfree(tmpbuf);
    }

    return 0;
}

void tcs_msg_header_dump(tcs_msg_header_t *header)
{
    log_d("message id => %04x", header->id);
    log_d(" attr reserve    => %02x", (header->attr >> 14) & 0b00000011);
    log_d(" attr subpackage => %02x", (header->attr >> 13) & 0b00000001);
    log_d(" attr encrypt    => %02x", (header->attr >> 10) & 0b00000111);
    log_d(" attr bodylen    => %d", TCS_GET_BODY_LEN(header->attr));
    log_d("version => %d", header->version);
    log_d("message number => %d", header->msgnum);
}

int tcs_message_encode(yuint8 *buf, yuint32 buflen, tcs_msg_t *msg)
{
    int index = 0;
    yuint32 len = 0;
    stream_t sp;
    yuint8 *tmpbuf = ymalloc(buflen);
    if (tmpbuf == 0) {
        return -1;
    }

    stream_init(&sp, tmpbuf, buflen);
    stream_2byte_write(&sp, msg->header.id);
    TCS_SET_BODY_LEN(msg->header.attr, msg->bodylen);
    stream_2byte_write(&sp, msg->header.attr);
    stream_byte_write(&sp, msg->header.version);
    stream_nbyte_write(&sp, msg->header.devid, sizeof(msg->header.devid));
    stream_2byte_write(&sp, msg->header.msgnum);
    if (TCS_GET_SUB_PACKET(msg->header.attr)) {
        stream_2byte_write(&sp, msg->header.packet_info.total);
        stream_2byte_write(&sp, msg->header.packet_info.number);
    }
    stream_nbyte_write(&sp, msg->body, msg->bodylen);
    msg->checksum = tmpbuf[0] ^ tmpbuf[1];
    for (index = 2; index < stream_left_len(&sp); index++) {
        msg->checksum ^= tmpbuf[index];
    }    
    stream_byte_write(&sp, msg->checksum);
    len = stream_use_len(&sp);

    stream_init(&sp, buf, buflen);
    stream_byte_write(&sp, 0x7e);
    for (index = 0; index < len; index++) {
        if (tmpbuf[index] == 0x7e) {
            stream_byte_write(&sp, 0x7d);
            stream_byte_write(&sp, 0x02);
        }
        else if (tmpbuf[index] == 0x7d){
            stream_byte_write(&sp, 0x7d);
            stream_byte_write(&sp, 0x01);
        } 
        else {
            stream_byte_write(&sp, tmpbuf[index]);
        }
    }
    stream_byte_write(&sp, 0x7e);

    if (tmpbuf) {
        yfree(tmpbuf);
    }
    
    return stream_use_len(&sp);
}

int tcs_register_body_encode(yuint8 *buf, yuint32 buflen, tcs_register_body_t *msg)
{
    stream_t sp;

    stream_init(&sp, buf, buflen);
    stream_2byte_write(&sp, msg->provinceid);
    stream_2byte_write(&sp, msg->cityid);
    stream_nbyte_write(&sp, msg->manufacturer, sizeof(msg->manufacturer));
    stream_nbyte_write(&sp, msg->model, sizeof(msg->model));
    stream_nbyte_write(&sp, msg->terid, sizeof(msg->terid));
    stream_byte_write(&sp, msg->color);
    stream_nbyte_write(&sp, msg->vin, sizeof(msg->vin));
    
    return stream_use_len(&sp);
}

static int tcs_authorization_request(terminal_control_service_t *tcs)
{
    tcs_msg_t *msg = tcs->tcsmsgt;
    int slen = 0;
    stream_t sp;
    yuint8 authlen = 0;

    memset(&msg->header, 0, sizeof(msg->header));
    msg->header.id = TERMINAL_LOGIN_REQUEST;
    msg->header.version = tcs->protocol_version;
    msg->header.msgnum = tcs->msgnum++;
    msg->header.attr |= 0x4000;
    msg->checksum = 0;
    memcpy(msg->header.devid, tcs->tcs_config.devid, \
        MIN(sizeof(msg->header.devid), sizeof(tcs->tcs_config.devid)));
    
    authlen = strlen((char *)tcs->authcode);
    stream_init(&sp, msg->body, sizeof(msg->body));
    stream_byte_write(&sp, authlen);
    stream_nbyte_write(&sp, tcs->authcode, authlen);
    stream_nbyte_write(&sp, tcs->tcs_config.imei, sizeof(tcs->tcs_config.imei));
    stream_nbyte_write(&sp, tcs->tcs_config.app_version, sizeof(tcs->tcs_config.app_version));
    msg->bodylen = stream_use_len(&sp);
    tcs_msg_header_dump(&msg->header);
    slen = tcs_message_encode(tcs_tx_buff_pointer(tcs), tcs_tx_buff_size(tcs), msg);
    tcs_message_decode(tcs_tx_buff_pointer(tcs), slen, msg);    
    tcs_msg_header_dump(&msg->header);
    if (tcs_tcp_send(tcs, tcs_tx_buff_pointer(tcs), slen) != slen) {
        log_e("Terminal authorization request send failed");        
        tcs_tcp_connect_abnormal_handler(tcs);
        return -1;
    }
    tcs->pgr = TCS_PGR_AUTHORIZATION;

    return 0;
}

static int tcs_register_request(terminal_control_service_t *tcs)
{
    tcs_msg_t *msg = tcs->tcsmsgt;
    int slen = 0;
    tcs_register_body_t reginfo;
    
    memset(&msg->header, 0, sizeof(msg->header));
    msg->header.id = TERMINAL_REGISTER_REQUEST;
    msg->header.version = tcs->protocol_version;
    msg->header.msgnum = tcs->msgnum++;
    msg->header.attr |= 0x4000;
    msg->checksum = 0;
    memcpy(msg->header.devid, tcs->tcs_config.devid, \
        MIN(sizeof(msg->header.devid), sizeof(tcs->tcs_config.devid)));

    tcs_msg_header_dump(&msg->header);
    reginfo.provinceid = tcs->tcs_config.provinceid;
    reginfo.cityid = tcs->tcs_config.cityid;
    memcpy(reginfo.manufacturer, tcs->tcs_config.manufacturer, \
        MIN(sizeof(reginfo.manufacturer), sizeof(tcs->tcs_config.manufacturer)));
    memcpy(reginfo.model, tcs->tcs_config.model, \
        MIN(sizeof(reginfo.model), sizeof(tcs->tcs_config.model)));
    memcpy(reginfo.terid, tcs->tcs_config.terid, \
        MIN(sizeof(reginfo.terid), sizeof(tcs->tcs_config.terid)));
    reginfo.color = tcs->tcs_config.color;
    memcpy(reginfo.vin, tcs->tcs_config.vin, \
        MIN(sizeof(reginfo.vin), sizeof(tcs->tcs_config.vin)));
    msg->bodylen = tcs_register_body_encode(msg->body, sizeof(msg->body), &reginfo);
    slen = tcs_message_encode(tcs_tx_buff_pointer(tcs), tcs_tx_buff_size(tcs), msg);
    tcs_message_decode(tcs_tx_buff_pointer(tcs), slen, msg);    
    tcs_msg_header_dump(&msg->header);
    if (tcs_tcp_send(tcs, tcs_tx_buff_pointer(tcs), slen) != slen) {        
        log_e("Terminal register request send failed");        
        tcs_tcp_connect_abnormal_handler(tcs);
        return -1;
    }

    return 0;
}

static int tcs_terminal_offline_request(terminal_control_service_t *tcs)
{
    tcs_msg_t *msg = tcs->tcsmsgt;
    int slen = 0;
    
    memset(&msg->header, 0, sizeof(msg->header));
    msg->header.id = TERMINAL_OFFLINE_REQUEST;
    msg->header.version = tcs->protocol_version;
    msg->header.msgnum = tcs->msgnum++;
    msg->header.attr |= 0x4000;
    msg->checksum = 0;
    memcpy(msg->header.devid, tcs->tcs_config.devid, \
        MIN(sizeof(msg->header.devid), sizeof(tcs->tcs_config.devid)));
    
    slen = tcs_message_encode(tcs_tx_buff_pointer(tcs), tcs_tx_buff_size(tcs), msg);
    tcs_message_decode(tcs_tx_buff_pointer(tcs), slen, msg);    
    if (tcs_tcp_send(tcs, tcs_tx_buff_pointer(tcs), slen) != slen) {        
        log_e("Terminal offline request send failed");        
        tcs_tcp_connect_abnormal_handler(tcs);
        return -1;
    }

    return 0;
}

static int tcs_oem_terminal_general_reply(terminal_control_service_t *tcs, yuint16 msgid, yuint8 code)
{
    tcs_msg_t *msg = tcs->tcsmsgt;
    int slen = 0;    
    stream_t sp;
    
    memset(&msg->header, 0, sizeof(msg->header));
    msg->header.id = TERMINAL_TRANSPARENT_REPLY;
    msg->header.version = tcs->protocol_version;
    msg->header.msgnum = tcs->msgnum++;
    msg->header.attr |= 0x4000;
    msg->checksum = 0;
    memcpy(msg->header.devid, tcs->tcs_config.devid, \
        MIN(sizeof(msg->header.devid), sizeof(tcs->tcs_config.devid)));

    stream_init(&sp, msg->body, sizeof(msg->body));
    stream_2byte_write(&sp, PLATFORM_TRANSPARENT_REQUEST);
    stream_2byte_write(&sp, msgid);
    stream_byte_write(&sp, code);
    slen = tcs_message_encode(tcs_tx_buff_pointer(tcs), tcs_tx_buff_size(tcs), msg);
    if (tcs_tcp_send(tcs, tcs_tx_buff_pointer(tcs), slen) != slen) {        
        log_e("Terminal oem gemeral reply send failed");        
        tcs_tcp_connect_abnormal_handler(tcs);
        return -1;
    }

    return 0;
}

static int tcs_register_reply_handler(terminal_control_service_t *tcs, tcs_msg_t *msg)
{
    stream_t sp;
    yuint16 msgnum = 0;
    yuint8 recode = 0;

    stream_init(&sp, msg->body, msg->bodylen);
    msgnum = stream_2byte_read(&sp);
    recode = stream_byte_read(&sp);
    if (recode == TCS_REGISTER_REPLY_CODE_SUCCESS) {
        log_d("treminal register success");
        memset(tcs->authcode, 0, sizeof(tcs->authcode));
        stream_nbyte_read(&sp, tcs->authcode, MIN(sizeof(tcs->authcode), stream_left_len(&sp)));
        log_d("Authcode: %s", tcs->authcode);
        tcs_authorization_request(tcs);
    }
    else {
        log_w("treminal register failed %d", recode);        
        tcs_tcp_connect_abnormal_handler(tcs);
    }

    Y_UNUSED(msgnum);

    return 0;
}

static int tcs_reconnect_server(terminal_control_service_t *tcs, int delay)
{
    log_d("Wait %d seconds to reconnect to the server", delay);
    ev_timer_stop(tcs->loop, &tcs->keepalive_watcher);
    ev_timer_init(&tcs->reconnect_watcher, tcs_ev_timer_server_reconnect_handler, \
        delay, delay);
    ev_timer_start(tcs->loop, &tcs->reconnect_watcher);
}

static void tcs_ev_timer_server_reconnect_handler(struct ev_loop *loop, ev_timer *w, int revents)
{
    terminal_control_service_t *tcs = (terminal_control_service_t*)container_of(w, terminal_control_service_t, reconnect_watcher);

    /* ��ʼ���� */
    yterminal_control_service_connect(tcs);

    /* ֹͣ��ʱ��ֻ��ִ��һ�� */
    ev_timer_stop(tcs->loop, &tcs->reconnect_watcher);
}

static void tcs_ev_timer_keepalive_handler(struct ev_loop *loop, ev_timer *w, int revents)
{
    terminal_control_service_t *tcs = (terminal_control_service_t*)container_of(w, terminal_control_service_t, keepalive_watcher);
    tcs_msg_t *msg = tcs->tcsmsgt;
    int slen = 0;
    
    memset(&msg->header, 0, sizeof(msg->header));
    msg->header.id = TERMINAL_HEARTBEAT_REQUEST;
    msg->header.version = tcs->protocol_version;
    msg->header.msgnum = tcs->msgnum++;
    msg->header.attr |= 0x4000;
    msg->checksum = 0;
    memcpy(msg->header.devid, tcs->tcs_config.devid, \
        MIN(sizeof(msg->header.devid), sizeof(tcs->tcs_config.devid)));
    
    slen = tcs_message_encode(tcs_tx_buff_pointer(tcs), tcs_tx_buff_size(tcs), msg);
    tcs_message_decode(tcs_tx_buff_pointer(tcs), slen, msg);    
    if (tcs_tcp_send(tcs, tcs_tx_buff_pointer(tcs), slen) != slen) {        
        log_e("Terminal keepalive request send failed");        
        tcs_tcp_connect_abnormal_handler(tcs);
        return ;
    }

    return ;
}

static void tcs_keepalive_timer_refresh(terminal_control_service_t *tcs)
{
    ev_timer_stop(tcs->loop, &tcs->keepalive_watcher);
    ev_timer_init(&tcs->keepalive_watcher, tcs_ev_timer_keepalive_handler, \
        tcs->keepalive_interval, tcs->keepalive_interval);
    ev_timer_start(tcs->loop, &tcs->keepalive_watcher);
}

static int tcs_platform_general_reply_handler(terminal_control_service_t *tcs, tcs_msg_t *msg)
{
    stream_t sp;
    yuint16 msgnum = 0;
    yuint16 renum = 0;
    yuint8 recode = 0;

    stream_init(&sp, msg->body, msg->bodylen);
    msgnum = stream_2byte_read(&sp);
    renum = stream_2byte_read(&sp);
    recode = stream_byte_read(&sp);
    if (recode == TCS_GENERAL_REPLY_CODE_SUCCESS) {
        if (tcs->pgr == TCS_PGR_AUTHORIZATION) {
            log_d("treminal authorization success");
            /* �ɹ����ӵ�¼ƽ̨�������������ķ��Ͷ�ʱ�� */
            ev_timer_init(&tcs->keepalive_watcher, tcs_ev_timer_keepalive_handler, \
                tcs->keepalive_interval, tcs->keepalive_interval);
            ev_timer_start(tcs->loop, &tcs->keepalive_watcher);
            /* ����������� */
            tcs->reconnect_num = 0;
            tcs->isruning = true;
        }
    }
    else {
        if (tcs->pgr == TCS_PGR_AUTHORIZATION) {            
            log_d("treminal authorization failed %d", recode);            
            tcs_tcp_connect_abnormal_handler(tcs);
        }
    }
    tcs->pgr = TCS_PGR_INVALID;

    Y_UNUSED(msgnum);
    Y_UNUSED(renum);

    return 0;
}

static int tcs_platform_transparent_request_handler(terminal_control_service_t *tcs, tcs_msg_t *msg)
{
    stream_t sp;    
    int ii = 0;
    yuint8 msgtype = 0;
    yuint8 mver = 0; /* �Զ���Э�����汾�� */
    yuint8 sver = 0; /* �Զ���Э���޶��汾�� */
    yuint16 msgid = 0; /* �Զ�����ϢID */
    yuint16 bodylen = 0; /* �Զ�����Ϣ���� */
    yuint8 recode = TCS_GENERAL_REPLY_CODE_UNSUPPORT;

    stream_init(&sp, msg->body, msg->bodylen);
    msgtype = stream_byte_read(&sp);
    if (msgtype == 0xF1) { /* �Զ�����Ϣ���� */
        mver = stream_byte_read(&sp);
        sver = stream_byte_read(&sp);
        msgid = stream_2byte_read(&sp);
        bodylen = stream_2byte_read(&sp);

        log_d("0xF1 message id %04x", msgid);
        for (ii = 0; ii < REMOTE_CMD_HANDLER_SIZE; ii++) {
            if (tcs->rc_handler[ii].msgid == msgid && \
                tcs->rc_handler[ii].handler) {  
                tcs->rc_handler[ii].handler(tcs->rc_handler[ii].argv, msgid, NULL);
                recode = TCS_GENERAL_REPLY_CODE_SUCCESS;
            }
        }      
        tcs_oem_terminal_general_reply(tcs, msgid, recode);
    }

    Y_UNUSED(mver);
    Y_UNUSED(sver);
    Y_UNUSED(bodylen);

    return 0;
}

int tcs_reply_handler(terminal_control_service_t *tcs)
{
    /* �������յ���ƽ̨��Ϣ����ʱû��������Ĵ��� */
    for ( ; ; ) {
        yuint8 *head = 0, *tail = 0; 
        tcs_msg_t *msg = tcs->tcsmsgt;
    
        memset(&msg->header, 0, sizeof(msg->header));
        for (int index = 0; index < tcs->recvlen; index++) {
            if (tcs_rx_buff_pointer(tcs)[index] == 0x7e) {
                if (head == 0) {
                    /* �ҵ���Ϣͷ */
                    head = tcs_rx_buff_pointer(tcs) + index;
                }
                else if (tail == 0) {
                    /* �ҵ���Ϣβ */
                    tail = tcs_rx_buff_pointer(tcs) + index;
                    break;
                }
                else {
                    break;
                }
            }
        }

        if (!tail || !head) {
            if (head == 0) {
                /* ��Ϣ������������û����Ϣβ���������buff����û��������Ϣ�� */
                tcs->recvlen = 0;  
            }
            else if (head != tcs_rx_buff_pointer(tcs)){
                /* ��Ϣ����������������Ϣͷ�������ⲿ�������������յ�������Ϣ */
                tcs->recvlen -= (head - tcs_rx_buff_pointer(tcs)); 
                memmove(tcs_rx_buff_pointer(tcs), head, tcs->recvlen);
            }
        
            return 0; /* �˳� for ( ; ; ) */
        }
        /* ������Ϣ */
        tcs->recvlen -= (tail - tcs_rx_buff_pointer(tcs) + 1); 
        memmove(tcs_rx_buff_pointer(tcs), tail + 1, tcs->recvlen);  

        tcs_message_decode(head, tail - head + 1, msg);    
        tcs_msg_header_dump(&msg->header);
        switch (msg->header.id) {
            case PLATFORM_REGISTER_REPLY:
                tcs_register_reply_handler(tcs, msg);
                break;
            case PLATFORM_GENERAL_REPLY:
                tcs_platform_general_reply_handler(tcs, msg);
                break;
            case PLATFORM_TRANSPARENT_REQUEST:
                tcs_platform_transparent_request_handler(tcs, msg);
                break;
            default:
                break;
        }
    }

    return 0;
}

static void tcs_ev_io_tcp_read_handler(struct ev_loop* loop, ev_io* w, int e)
{
    ssize_t bytesRecv = -1;
    terminal_control_service_t* tcs = container_of(w, terminal_control_service_t, tcprecv_watcher);

    /* ����Ƿ������� */
    if (tcs->sock_stat == TCS_CONNECT_PROCESS) {
        tcs_tcp_connect_process_handler(tcs);
        return ;
    }
    
    bytesRecv = recv(tcs->sockfd, tcs_rx_buff_pointer(tcs) + tcs->recvlen, \
        tcs_rx_buff_size(tcs) - tcs->recvlen, 0);  
    if (!(bytesRecv > 0)) {
        tcs_tcp_connect_abnormal_handler(tcs);
        return ;
    }
    else {
        tcs_keepalive_timer_refresh(tcs);
        tcs->recvlen += bytesRecv; 
        log_hex_d("recv", tcs_rx_buff_pointer(tcs), tcs->recvlen);
        tcs_reply_handler(tcs);
    }

    return ;
}

static void tcs_ev_io_tcp_write_handler(struct ev_loop* loop, ev_io* w, int e)
{
    terminal_control_service_t* tcs = container_of(w, terminal_control_service_t, tcpsend_watcher);

    /* ����Ƿ������� */
    if (tcs->sock_stat == TCS_CONNECT_PROCESS) {
        tcs_tcp_connect_process_handler(tcs);
        return ;
    }

    return ;
}

int yterminal_control_service_connect(terminal_control_service_t *tcs)
{
    int ret = 0;
    struct sockaddr_in toAddr;

    /* �ȸ�λ��һ�����ӵ�״̬ */
    tcs_disconnect(tcs);

    /* ������������TCP���� */
    if (!(tcs->sockfd > 0)) {
        tcs->sockfd = tcs_tcp_create();
        if (tcs->sockfd < 0) {
            log_e("tcp sockfd create failed");
            return -2;
        }
    }

    struct in_addr addr;
    if (inet_pton(AF_INET, tcs->tcs_config.srv_addr, &addr) <= 0) {
        log_e("inet_pton() error");
        return -3;
    }
    
    /* ��ʼ����ƽ̨server */
    toAddr.sin_family = AF_INET;
    toAddr.sin_addr.s_addr = addr.s_addr;
    toAddr.sin_port = htons(tcs->tcs_config.srv_port);
    ret = connect(tcs->sockfd, (struct sockaddr *)&toAddr, sizeof(struct sockaddr));
    if (ret == -1 && errno == EINPROGRESS) {
        /* ���һ������������ */
        tcs->sock_stat = TCS_CONNECT_PROCESS; 
        /* ��������SOCKET��Ҫ�����ж� */
        ev_io_init(&tcs->tcprecv_watcher, tcs_ev_io_tcp_read_handler, tcs->sockfd, EV_READ);
        ev_io_start(tcs->loop, &tcs->tcprecv_watcher);
        ev_io_init(&tcs->tcpsend_watcher, tcs_ev_io_tcp_write_handler, tcs->sockfd, EV_WRITE);
        ev_io_start(tcs->loop, &tcs->tcpsend_watcher);
    }
    else if (ret == 0) {        
        log_w("TCP connect success");
        ev_io_init(&tcs->tcprecv_watcher, tcs_ev_io_tcp_read_handler, tcs->sockfd, EV_READ);
        ev_io_start(tcs->loop, &tcs->tcprecv_watcher);        
        tcs_tcp_connect_success_handler(tcs);
    }
    else {
        /* ����ʧ�� */
        log_w("TCP connect failed");
        tcs_tcp_connect_abnormal_handler(tcs);
    }

    return 0;

}

void yterminal_control_service_disconnect(terminal_control_service_t *tcs)
{
    /* ������½�˾ͷ����������� */
    if (tcs->isruning) {
        /* ��������������Ϣ��֪ͨƽ̨�ն����� */
        tcs_terminal_offline_request(tcs);
    }

    /* �Ͽ�TCP���� */
    tcs_disconnect(tcs);
}

terminal_control_service_t *yterminal_control_service_create(struct ev_loop *loop)
{
    terminal_control_service_t *tcs = ymalloc(sizeof(*tcs));
    if (tcs == NULL) {
        return NULL;
    }
    memset(tcs, 0, sizeof(*tcs));
    tcs->tcsmsgt = ymalloc(sizeof(*tcs->tcsmsgt));
    if (tcs->tcsmsgt == NULL) {
        yfree(tcs);
        return NULL;
    }
    memset(tcs->tcsmsgt, 0, sizeof(*tcs->tcsmsgt));
    tcs->loop = loop;
    tcs->sockfd = -1;
    tcs->sock_stat = TCS_DISCONNECT;
    tcs->protocol_version = TCS_PROTOCOL_VERSION;
    tcs->keepalive_interval = TCS_KEEPALIVE_INTERVAL_DEF;
    tcs->rxlen = TCS_RXTX_BUFF_SIZE_DEF;
    tcs->rxbuf = ymalloc(tcs->rxlen);
    if (tcs->rxbuf == NULL) {
        log_e("malloc rxbuff error");
        goto CREATE_FAILED;
    }
    memset(tcs->rxbuf, 0, tcs->rxlen);

    tcs->txlen = TCS_RXTX_BUFF_SIZE_DEF;
    tcs->txbuf = ymalloc(tcs->txlen);
    if (tcs->txbuf == NULL) {
        log_e("malloc txbuff error");
        goto CREATE_FAILED;
    }
    memset(tcs->txbuf, 0, tcs->txlen);

    return tcs;
CREATE_FAILED:
    yterminal_control_service_destroy(tcs);

    return NULL;
}

int yterminal_control_service_handler_set(terminal_control_service_t *tcs, yuint16 msgid, premote_cmd_handler call, void *argv)
{
    int ii = 0;

    for (ii = 0; ii < REMOTE_CMD_HANDLER_SIZE; ii++) {
        if (tcs->rc_handler[ii].msgid == msgid && \
            tcs->rc_handler[ii].handler) {
            /* �Ѿ��д�����ע�ᣬ������ע�� */
            return 0;
        }
    }

    for (ii = 0; ii < REMOTE_CMD_HANDLER_SIZE; ii++) {
        if (tcs->rc_handler[ii].handler == NULL) {
            tcs->rc_handler[ii].handler = call;
            tcs->rc_handler[ii].msgid = msgid;
            tcs->rc_handler[ii].argv = argv;
            break;
        }
    }

    return 0;
}

int yterminal_control_service_destroy(terminal_control_service_t *tcs)
{
    if (tcs == NULL) {
        return -1;
    }

    tcs_disconnect(tcs);
    if (tcs->rxbuf) {
        yfree(tcs->rxbuf);
        tcs->rxbuf = NULL;
        tcs->rxlen = 0;
    }
    if (tcs->txbuf) {
        yfree(tcs->txbuf);
        tcs->txbuf = NULL;
        tcs->txlen = 0;
    }
    if (tcs->tcsmsgt) {
        yfree(tcs->tcsmsgt);
        tcs->tcsmsgt = NULL;
    }

    yfree(tcs);

    return 0;
}

int yterminal_control_service_info_set(terminal_control_service_t *tcs, terminal_control_service_info_t *tcs_config)
{
    memcpy(&tcs->tcs_config, tcs_config, sizeof(terminal_control_service_info_t));

    return 0;
}

int yterminal_control_service_info_get(terminal_control_service_t *tcs, terminal_control_service_info_t *tcs_config)
{
    memcpy(tcs_config, &tcs->tcs_config, sizeof(terminal_control_service_info_t));

    return 0;
}

boolean yterminal_control_service_info_equal(terminal_control_service_t *tcs, terminal_control_service_info_t *tcs_config)
{
    if (memcmp(&tcs->tcs_config, tcs_config, sizeof(terminal_control_service_info_t)) == 0) {
        return true;
    }

    return false;
}

