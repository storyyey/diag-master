#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <sys/un.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <dirent.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/prctl.h>

#include "config.h"
#include "ev.h"

#include "ycommon_type.h"
#include "ydm_types.h"
#include "ydm_log.h"
#include "yudsc_types.h"
#include "ydm_ipc_common.h"
#include "yuds_client.h"
#include "ydoip_client.h"
#include "yremote_command.h"
#include "yshow_services.h"
#ifdef __HAVE_UDS_PROTOCOL_ANALYSIS__        
#include "yuds_proto_parse.h"
#endif /* __HAVE_UDS_PROTOCOL_ANALYSIS__ */   
#include "ydm.h"

/* �汾�� */
#define DM_VERSION "1.0.1"

#ifdef __HAVE_DIAG_MASTER_PTHREAD__
static pthread_mutex_t dms_mutex_lock = PTHREAD_MUTEX_INITIALIZER;
#define DMS_LOCK   { pthread_mutex_lock(&dms_mutex_lock)
#define DMS_UNLOCK   pthread_mutex_unlock(&dms_mutex_lock);}
#else /* __HAVE_DIAG_MASTER_PTHREAD__ */
#define DMS_LOCK   { 
#define DMS_UNLOCK   }
#endif /* __HAVE_DIAG_MASTER_PTHREAD__ */

#define DM_ATTR_ISSET(attr, f) (!!(attr & f))

/* IPC���շ��ͻ��� */
#define DM_TX_BUFF(dm) dm_txbuf(dm),dm_txbuf_size(dm)
#define DM_RX_BUFF(dm) dm_rxbuf(dm),dm_rxbuf_size(dm)

/* ÿ��diag master�����󣬴������ٸ�Ĭ�ϵ�uds�ͻ��� */
#define DM_UDSC_CAPACITY_DEF (10)

struct DMS;
typedef struct DMS DMS;

struct ydiag_master;
typedef struct ydiag_master ydiag_master;

typedef void (*dm_event_handler_callback)(ydiag_master *, yuint32, yuint16);

typedef struct om_event_handler_s {
    dm_event_handler_callback call;
} dm_event_handler_t;
typedef struct ydiag_master {
    char name[8]; 
    dm_event_handler_t event_calls[DM_IPC_EVENT_TYPE_MAX];
    yuint32 event_valid_time; /* ������Чʱ��,�շ��˳�ʱ��������ɵ��쳣 */      
    yuint32 premsg_tv_sec; /* ǰһ����Ϣ��ʱ������������ظ���Ϣȥ�� */
    yuint32 premsg_tv_usec; /* ǰһ����Ϣ��ʱ������������ظ���Ϣȥ�� */
    int sockfd; /* ��dm_apiͨ��ʹ�� */
    int index; /* ����ֵ���ڴ���ʱ���� */
    struct ev_loop* loop; /* �¼���ѭ�� */
    ev_io iwrite_watcher; /* ����IPC���¼� */
    ev_io iread_watcher; /* ����IPCд�¼� */
    ev_timer keepalive_watcher; /* ������ʱ���������ж�diag master API�Ƿ����� */
    yuint32 keepalive_cnt; /* �����������յ�����Ӧ��������㣬������������Ϊdiag master API�����쳣 */
    yuint32 keepalive_interval;
    yuint32 udsc_cnt; /* UDS�ͻ������� */
    uds_client *udscs[DM_UDSC_CAPACITY_MAX]; /* uds�ͻ��˱������±����UDS�ͻ��˵�idֵ */
    yuint8 txbuf[IPC_TX_BUFF_SIZE]; /* socket ����buff */
    yuint8 rxbuf[IPC_RX_BUFF_SIZE]; /* socket ����buff */
    char dm_path[IPC_PATH_LEN]; /* diag master��UNIX socket·�� */
    char dmapi_event_emit_path[IPC_PATH_LEN]; /* diag master API��UNIX socket·�� */
    char dmapi_event_listen_path[IPC_PATH_LEN]; /* diag master API��UNIX socket·���ص���Ϣʹ�� */
#define TERMINAL_REMOTE_CAPACITY_MAX (10)
    terminal_control_service_t *tcss[TERMINAL_REMOTE_CAPACITY_MAX]; 
    DMS *dms;
} ydiag_master;

#define UDSC_IS_VALID(udscid) ((udscid) >= 0 && (udscid) < DM_UDSC_CAPACITY_MAX)
#define TCS_IS_VALID(tcsid) ((tcsid) >= 0 && (tcsid) < TERMINAL_REMOTE_CAPACITY_MAX)
#define IPC_CMD_IS_VALID(evtype) ((evtype) >= 0 && (evtype) < DM_IPC_EVENT_TYPE_MAX)

typedef struct DMS {    
    int sockfd; /* ��dm_apiͨ��ʹ�� */
    struct ev_loop* loop; /* �¼���ѭ�� */
    ev_io iwrite_watcher; /* ����IPC���¼� */
    ev_io iread_watcher; /* ����IPCд�¼� */    
    ev_timer logfile_limit_watcher; /* ��־�ļ��ϻ� */ 
    float logfile_cycle_period; /* unit s */
#define DMS_DIAG_MASTER_NUM_MAX (4)    
    ydiag_master *dm[DMS_DIAG_MASTER_NUM_MAX];     
    yuint8 txbuf[2048]; /* socket ����buff */
    yuint8 rxbuf[2048]; /* socket ����buff */
} DMS;

DMS *__g_oms__ = NULL;

struct ev_loop* ydiag_master_ev_loop(ydiag_master* dm);
static int dm_uds_service_request_event_emit(ydiag_master *dm, yuint16 udscid, const yuint8 *data, yuint32 size, yuint32 sa, yuint32 ta, yuint32 tatype);
static int dm_uds_client_result_event_emit(ydiag_master *dm, yuint16 udscid, yuint32 result, const yuint8 *ind, yuint32 indl, const yuint8 *resp, yuint32 respl);
static int dm_uds_service_result_event_emit(ydiag_master *dm, yuint16 udscid, const yuint8 *data, yuint32 size, yuint32 rr_callid);
static int dm_uds_service_27_sa_seed_event_emit(ydiag_master *dm, yuint16 udscid, yuint8 level, const yuint8 *data, yuint32 size);
static int dm_uds_service_36_transfer_progress_event_emit(ydiag_master *dm, yuint16 udscid, yuint32 file_size, yuint32 total_byte, yuint32 elapsed_time);
static void dm_logfile_limit_cycle_period_refresh(DMS *dms);
static void dm_ev_timer_logfile_limit_handler(struct ev_loop *loop, ev_timer *w, int revents);
static void dm_uds_asc_record(char *direction, yuint8 *msg, yuint32 mlen);
static void dm_ev_timer_keepalive_handler(struct ev_loop *loop, ev_timer *w, int revents);

static yuint8 *dm_rxbuf(ydiag_master *dm)
{
    return dm->rxbuf;
}

static yint32 dm_rxbuf_size(ydiag_master *dm)
{
    return sizeof(dm->rxbuf);
}

static yuint8 *dm_txbuf(ydiag_master *dm)
{
    return dm->txbuf;
}

static yint32 dm_txbuf_size(ydiag_master *dm)
{
    return sizeof(dm->txbuf);
}

static uds_client *dm_udsc(ydiag_master *dm, yuint16 id)
{
    if (!(id < dm->udsc_cnt) ||\
        dm->udscs[id] == NULL || \
        dm->udscs[id]->isidle == true) {
        return NULL;
    }
        
    return dm->udscs[id];
}

static void dm_keepalive_refresh(ydiag_master *dm)
{
    dm->keepalive_cnt = 0;

    ev_timer_stop(dm->loop, &dm->keepalive_watcher);
    ev_timer_init(&dm->keepalive_watcher, dm_ev_timer_keepalive_handler, \
        dm->keepalive_interval * 0.001, dm->keepalive_interval * 0.001);
    ev_timer_start(dm->loop, &dm->keepalive_watcher);
}

static int dm_event_emit_to_api(ydiag_master *dm, unsigned char *buff, unsigned int size) 
{
    struct sockaddr_un un;
    ssize_t bytes_sent = -1;
    fd_set writefds;
    struct timeval timeout;
    int sret = 0;
    
    if (dm->sockfd < 0) {
        return -1;
    }

    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    FD_ZERO(&writefds);
    FD_SET(dm->sockfd, &writefds);
    if ((sret = select(dm->sockfd + 1, NULL, &writefds, NULL, &timeout)) > 0 &&\
        FD_ISSET(dm->sockfd, &writefds)) {
        memset(&un, 0, sizeof(un));
        un.sun_family = AF_UNIX;
        snprintf(un.sun_path, sizeof(un.sun_path), dm->dmapi_event_listen_path, strlen(dm->dmapi_event_listen_path));    
        bytes_sent = sendto(dm->sockfd, buff, size, 0,\
                            (struct sockaddr *)&un, sizeof(struct sockaddr_un));
    }

    return bytes_sent;
}

static int dm_sendto_api(ydiag_master *dm, unsigned char *buff, unsigned int size)
{
    struct sockaddr_un un;
    ssize_t bytes_sent = -1;
    fd_set writefds;
    struct timeval timeout;
    int sret = 0;
    
    if (dm->sockfd < 0) {
        return -1;
    }
    
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    FD_ZERO(&writefds);
    FD_SET(dm->sockfd, &writefds);
    if ((sret = select(dm->sockfd + 1, NULL, &writefds, NULL, &timeout)) > 0 &&\
        FD_ISSET(dm->sockfd, &writefds)) {
        memset(&un, 0, sizeof(un));
        un.sun_family = AF_UNIX;
        snprintf(un.sun_path, sizeof(un.sun_path), dm->dmapi_event_emit_path, strlen(dm->dmapi_event_emit_path));    
        bytes_sent = sendto(dm->sockfd, buff, size, 0,\
                            (struct sockaddr *)&un, sizeof(struct sockaddr_un));
    }

    return bytes_sent;
}

/* ֹͣdiag master���ҵ��¼�ѭ�����⽫����diag master�¼�ѭ���˳� */
static void dm_ev_stop(ydiag_master *dm)
{ 
    ev_io_stop(dm->loop, &dm->iread_watcher);
    ev_io_stop(dm->loop, &dm->iwrite_watcher);
    ev_timer_stop(dm->loop, &dm->keepalive_watcher);
#ifndef __HAVE_DIAG_MASTER_PTHREAD__ 
    /* ��δ�����̵߳������ֱ������diag master */
    /* �������̵߳�ʱ�������¼��˳���,�����߳����Զ�����diag master */
    dm_destroy(dm);
#endif /* __HAVE_DIAG_MASTER_PTHREAD__ */
}

static void dm_security_access_keygen_callback(ydiag_master *dm, yuint16 udscid, yuint8 level, yuint8 *seed, yuint16 seed_size)
{
    dm_uds_service_27_sa_seed_event_emit(dm, udscid, level, seed, seed_size);
}

static void dm_doipc_diagnostic_response_callback(void *arg, yuint16 sa, yuint16 ta, yuint8 *msg, yuint32 len)
{
    if (yudsc_asc_record_enable(arg)) {
        dm_uds_asc_record("TX", msg, len);
    }
    yudsc_service_response_finish((uds_client *)arg, SERVICE_RESPONSE_NORMAL, sa, ta, msg, len);
}

static yint32 dm_udsc_request_transfer_callback(yuint16 udscid, ydiag_master *dm, const yuint8 *data, yuint32 size, yuint32 sa, yuint32 ta, yuint32 tatype)
{
    int sbytes = -1;
    doip_client_t *doipc = NULL;
    uds_client *udsc = dm_udsc(dm, udscid);

    if (yudsc_asc_record_enable(udsc)) {
        dm_uds_asc_record("TX", data, size);
    }
    /* ����ʹ��DOIP�ͻ��� */
    if ((doipc = yudsc_doip_channel(dm_udsc(dm, udscid)))) {
        sbytes = ydoipc_diagnostic_request(doipc, sa, ta, data, size, dm_doipc_diagnostic_response_callback, udsc);
    }
    else {
        sbytes = dm_uds_service_request_event_emit(dm, udscid, data, size, sa, ta, tatype);
    }

    return sbytes;
}

static void dm_service_36_transfer_progress_callback(ydiag_master *dm, yuint16 udscid, yuint32 file_size, yuint32 total_byte, yuint32 elapsed_time)
{    
    dm_uds_service_36_transfer_progress_event_emit(dm, udscid, file_size, total_byte, elapsed_time);
}

static void dm_udsc_task_end_callback(uds_client *udsc, udsc_finish_stat stat, void *arg, const yuint8 *ind, yuint32 indl, const yuint8 *resp, yuint32 respl)
{
    ydiag_master *dm = arg;
    yuint32 recode = DM_ERR_NO;

    if (stat == UDSC_UNEXPECT_RESPONSE_FINISH) {
        /* ��Ԥ����Ӧ */
        recode = DM_ERR_UDS_RESPONSE_UNEXPECT;
    }
    else if (stat == UDSC_TIMEOUT_RESPONSE_FINISH) {
        /* ��Ӧ��ʱ */
        recode = DM_ERR_UDS_RESPONSE_TIMEOUT;
    }
    if (ind) {
        log_hex_d("Services finish Request", ind, indl);
    }
    if (resp) {
        log_hex_d("Services finish Response", resp, respl);
    }
    yshow_uds_services(udsc->udsc_name, udsc->service_items, udsc->service_cnt);

    dm_uds_client_result_event_emit(dm, udsc->id, recode, ind, indl, resp, respl);
}

static void dm_ipc_event_service_response_handler(ydiag_master *dm, yuint32 recode, yuint16 udscid)
{
    yuint32 evtype = DM_EVENT_SERVICE_RESPONSE_ACCEPT;
    yuint32 to_recode = DM_ERR_NO;
    yuint16 to_id = udscid;
    yuint8 *dp = NULL;
    yuint8 *payload = NULL;
    service_item *sitem = NULL;
    yuint32 A_SA = 0, A_TA = 0, payload_length = 0, TA_TYPE = 0;
    uds_client *udsc = dm_udsc(dm, udscid);

    log_d("Event type: 0x%02X(%s) Recode: %d(%s) udsc ID: %d ", \
        evtype, ydm_ipc_event_str(evtype), to_recode, ydm_event_rcode_str(to_recode), to_id);
    ydm_common_encode(DM_TX_BUFF(dm), evtype, to_recode, to_id, IPC_USE_USER_TIME_STAMP);
    dm_sendto_api(dm, dm_txbuf(dm), DM_IPC_EVENT_MSG_SIZE);   

    dp = pointer_offset_nbyte(dm_rxbuf(dm), DM_IPC_EVENT_MSG_SIZE);
    A_SA = TYPE_CAST_UINT(pointer_offset_nbyte(dp, SERVICE_RESPONSE_A_SA_OFFSET));
    A_TA = TYPE_CAST_UINT(pointer_offset_nbyte(dp, SERVICE_RESPONSE_A_TA_OFFSET));
    payload_length = TYPE_CAST_UINT(pointer_offset_nbyte(dp, SERVICE_RESPONSE_PAYLOAD_LEN_OFFSET));
    TA_TYPE = TYPE_CAST_UINT(pointer_offset_nbyte(dp, SERVICE_RESPONSE_TA_TYPE_OFFSET));
    log_d("Mtype: %d A_SA: 0x%08X A_TA: 0x%08X TA_TYPE: %d pl: %d ", 0, A_SA, A_TA, TA_TYPE, payload_length);
    payload = pointer_offset_nbyte(dp, SERVICE_RESPONSE_PAYLOAD_OFFSET);  
    if (yudsc_asc_record_enable(udsc)) {
        dm_uds_asc_record("RX", payload, payload_length);
    }
    // log_hex_d("payload: ", payload, payload_length);        
    /* �ж�һ���Ƿ���Ҫ����Ӧ������͸�diag master API���� */
    sitem = yudsc_curr_service_item(udsc);
    if (sitem && sitem->rr_callid > 0) {
        if ((payload[0] == 0x7f && payload[1] != sitem->sid) && \
            (payload[0] != (sitem->sid | UDS_REPLY_MASK))) {
            /* �ǵ�ǰ��Ϸ�����Ӧ���ݣ����Դ��� */
        }
        else {
            dm_uds_service_result_event_emit(dm, udscid, payload, payload_length, sitem->rr_callid);
        }
    }
    /* ����UDS�ͻ��˴��������Ӧ             */
    yudsc_service_response_finish(udsc, SERVICE_RESPONSE_NORMAL, A_SA, A_TA, payload, payload_length);
}

static uds_client *dm_udsc_create(ydiag_master *dm)
{
    int udsc_index = 0;
    uds_client *udsc = NULL;
    
    udsc = yudsc_create();
    if (udsc) {            
        yudsc_ev_loop_set(udsc, ydiag_master_ev_loop(dm));
        for (udsc_index = dm->udsc_cnt; \
             udsc_index < DM_UDSC_CAPACITY_MAX; \
             udsc_index++) {
            if (dm->udscs[udsc_index] == NULL) {
                yudsc_id_set(udsc, udsc_index);
                dm->udscs[udsc_index] = udsc;
                dm->udsc_cnt++;
                log_i("UDSC %d create success", udsc_index);
                return udsc; /* �����ɹ� */
            }
        }
    }
    if (udsc) {
        yudsc_destroy(udsc);
        udsc = NULL;
    }
    
    return NULL;
}

static uds_client *dm_udsc_idle_find(ydiag_master *dm)
{
    int udsc_index = 0;
    uds_client *udsc = NULL;   
    
    for (udsc_index = 0; \
         udsc_index < dm->udsc_cnt;\
         udsc_index++) {
        if (dm->udscs[udsc_index] && \
            yudsc_is_idle(dm->udscs[udsc_index])) {
            udsc = dm->udscs[udsc_index];
            return udsc;
        }
    }

    return NULL;
}

static void dm_ipc_event_udsc_create_handler(ydiag_master *dm, yuint32 recode, yuint16 udscid)
{
    int udsc_index = 0;
    yuint32 evtype = DM_EVENT_UDS_CLIENT_CREATE_ACCEPT;
    yuint32 to_recode = DM_ERR_NO;
    yuint16 to_id = 0;
    uds_client *udsc = NULL;    
    udsc_create_config config;

    memset(&config, 0, sizeof(config));
    /* �����ҵ�һ�����е�UDS�ͻ��� */
    udsc = dm_udsc_idle_find(dm);
    /* û���ҵ����е�UDS�ͻ��ˣ�����һ���µ�UDS�ͻ���ʹ�� */
    if (udsc == NULL) {    
        udsc = dm_udsc_create(dm);
    }

    if (udsc != NULL) {
        ydm_udsc_create_config_decode(pointer_offset_nbyte(dm_rxbuf(dm), DM_IPC_EVENT_MSG_SIZE), \
            dm_rxbuf_size(dm) - DM_IPC_EVENT_MSG_SIZE, &config);
        if (config.service_item_capacity > yudsc_service_item_capacity(udsc)) {
            log_i("UDS client service item capacity reinit %d ", config.service_item_capacity);
            yudsc_service_item_capacity_init(udsc, config.service_item_capacity);
        }
        snprintf(udsc->dm_name, sizeof(udsc->dm_name), "%s", dm->name);
        to_id = yudsc_id(udsc); /* ��������UDS�ͻ���ID���ظ�diag master APIʹ�� */
        yudsc_idle_set(udsc, false); /* ����UDS�ͻ����Ѿ���ʹ�� */ 
        yudsc_name_set(udsc, config.udsc_name);
        yudsc_request_transfer_callback_set(udsc, (udsc_request_transfer_callback)dm_udsc_request_transfer_callback, dm);
        yudsc_task_end_callback_set(udsc, (udsc_task_end_callback)dm_udsc_task_end_callback, dm);
    }
    else {
        log_e("udsc create error ");
        to_recode = DM_ERR_UDSC_CREATE;
    }

    if (to_recode != DM_ERR_NO && udsc) {
        /* ����UDS�ͻ���ʧ�� */
        log_e("udsc create failed ");        
    }
    log_d("Event type: 0x%02X(%s) Recode: %d(%s) udsc ID: %d ", \
        evtype, ydm_ipc_event_str(evtype), to_recode, ydm_event_rcode_str(to_recode), to_id);
    ydm_common_encode(DM_TX_BUFF(dm), evtype, to_recode, to_id, IPC_USE_USER_TIME_STAMP);
    dm_sendto_api(dm, dm_txbuf(dm), DM_IPC_EVENT_MSG_SIZE);

    return ;
}

static void dm_ipc_event_udsc_destroy_handler(ydiag_master *dm, yuint32 recode, yuint16 udscid)
{
    yuint32 evtype = DM_EVENT_UDS_CLIENT_DESTORY_ACCEPT;
    yuint32 to_recode = DM_ERR_NO;
    yuint16 to_id = udscid;
    doip_client_t *doipc = NULL;

    /* �ͷŰ���UDS�ͻ����ϵ�DOIP�ͻ��� */
    if ((doipc = yudsc_doip_channel(dm_udsc(dm, udscid)))) {
        yudsc_doip_channel_unbind(dm_udsc(dm, udscid));
        ydoipc_destroy(doipc);
    }

    /* �����ͷ�UDS�ͻ��ˣ�ֻ�ǰ�UDS�ͻ�������Ϊ���У����ٷ�����YX_MALLOC��free */
    yudsc_reset(dm_udsc(dm, udscid));
    
    log_d("Event type: 0x%02X(%s) Recode: %d(%s) udsc ID: %d", \
        evtype, ydm_ipc_event_str(evtype), to_recode, ydm_event_rcode_str(to_recode), to_id);
    ydm_common_encode(DM_TX_BUFF(dm), evtype, to_recode, to_id, IPC_USE_USER_TIME_STAMP);
    dm_sendto_api(dm, dm_txbuf(dm), DM_IPC_EVENT_MSG_SIZE);

    return ;
}

static void dm_ipc_event_udsc_start_handler(ydiag_master *dm, yuint32 recode, yuint16 udscid)
{
    yuint32 evtype = DM_EVENT_START_UDS_CLIENT_ACCEPT;
    yuint32 to_recode = DM_ERR_NO;
    yuint16 to_id = udscid;
    uds_client *udsc = dm_udsc(dm, udscid);
    
    yudsc_services_start(udsc);
    yshow_uds_services(udsc->udsc_name, udsc->service_items, udsc->service_cnt);
    
    log_d("Event type: 0x%02X(%s) Recode: %d(%s) udsc ID: %d", \
        evtype, ydm_ipc_event_str(evtype), to_recode, ydm_event_rcode_str(to_recode), to_id);
    ydm_common_encode(DM_TX_BUFF(dm), evtype, to_recode, to_id, IPC_USE_USER_TIME_STAMP);
    dm_sendto_api(dm, dm_txbuf(dm), DM_IPC_EVENT_MSG_SIZE);

    return ;
}

static void dm_ipc_event_udsc_stop_handler(ydiag_master *dm, yuint32 recode, yuint16 udscid)
{
    yuint32 evtype = DM_EVENT_STOP_UDS_CLIENT_ACCEPT;
    yuint32 to_recode = DM_ERR_NO;
    yuint16 to_id = udscid;
    
    yudsc_services_stop(dm_udsc(dm, udscid));

    log_d("Event type: 0x%02X(%s) Recode: %d(%s) udsc ID: %d", \
        evtype, ydm_ipc_event_str(evtype), to_recode, ydm_event_rcode_str(to_recode), to_id);
    ydm_common_encode(DM_TX_BUFF(dm), evtype, to_recode, to_id, IPC_USE_USER_TIME_STAMP);
    dm_sendto_api(dm, dm_txbuf(dm), DM_IPC_EVENT_MSG_SIZE);

    return ;
}

static void dm_ipc_event_master_reset_handler(ydiag_master *dm, yuint32 recode, yuint16 udscid)
{
    yuint32 evtype = DM_EVENT_DIAG_MASTER_RESET_ACCEPT;
    yuint32 to_recode = DM_ERR_NO;
    yuint16 to_id = udscid;
    uds_client *udsc = NULL;
    yuint16 udsc_index = 0;
    
    for (udsc_index = 0; udsc_index < dm->udsc_cnt; udsc_index++) {
        udsc = dm->udscs[udsc_index];
        if (udsc) {
            yudsc_services_stop(udsc);
            yudsc_reset(udsc);
        }
    }

    log_d("Event type: 0x%02X(%s) Recode: %d(%s) udsc ID: %d", \
        evtype, ydm_ipc_event_str(evtype), to_recode, ydm_event_rcode_str(to_recode), to_id);
    ydm_common_encode(DM_TX_BUFF(dm), evtype, to_recode, to_id, IPC_USE_USER_TIME_STAMP);
    dm_sendto_api(dm, dm_txbuf(dm), DM_IPC_EVENT_MSG_SIZE);

    return ;
}

static void dm_ipc_event_service_config_handler(ydiag_master *dm, yuint32 recode, yuint16 udscid)
{
    yuint32 evtype = DM_EVENT_UDS_SERVICE_ITEM_ADD_ACCEPT;
    yuint32 to_recode = DM_ERR_NO;
    yuint16 to_id = udscid;
    service_config sconfig;
    service_item *si = NULL;
    yuint8 *dp = NULL;
    int flen = 0;
    
    uds_client *udsc = dm_udsc(dm, udscid);    
    dp = pointer_offset_nbyte(dm_rxbuf(dm), DM_IPC_EVENT_MSG_SIZE);
    log_i("Service config: %s", dp); 
    memset(&sconfig, 0, sizeof(sconfig));
    sconfig.variableByte = y_byte_array_new();
    sconfig.expectResponByte = y_byte_array_new();
    sconfig.finishByte = y_byte_array_new(); 
    y_byte_array_clear(sconfig.variableByte);
    y_byte_array_clear(sconfig.expectResponByte);
    y_byte_array_clear(sconfig.finishByte);
    ydm_service_config_decode(dp, strlen((const char *)dp), &sconfig);
    if ((si = yudsc_service_item_add(udsc, sconfig.desc))) {    
        yudsc_service_enable_set(si, true);  
        yudsc_service_sid_set(si, sconfig.sid & UDS_SID_MASK);
        yudsc_service_sub_set(si, sconfig.sub & UDS_SUBFUNCTION_MASK);
        yudsc_service_did_set(si, sconfig.did);
        yudsc_service_delay_set(si, sconfig.delay);
        yudsc_service_timeout_set(si, sconfig.timeout);

        y_byte_array_append_array(si->variable_byte, sconfig.variableByte);
  
        yudsc_service_expect_response_set(si, sconfig.expectRespon_rule, \
            y_byte_array_const_data(sconfig.expectResponByte), y_byte_array_count(sconfig.expectResponByte));
        yudsc_service_finish_byte_set(si, sconfig.finish_rule, \
            y_byte_array_const_data(sconfig.finishByte), y_byte_array_count(sconfig.finishByte));        
        si->finish_num_max = sconfig.finish_num_max;
        si->ta = sconfig.ta;
        si->sa = sconfig.sa;
        si->tatype = sconfig.tatype;
        si->issuppress = sconfig.issuppress;
        if (si->issuppress == false) {
            /* ����һ��������Ӧ������ӹ����ڵ���� */
            si->issuppress = sconfig.sub & UDS_SUPPRESS_POS_RSP_MSG_IND_MASK ? true : false;
        }
        si->rr_callid = sconfig.rr_callid;
        si->td_36.max_number_of_block_length = sconfig.max_number_of_block_length;
        if (si->sid == UDS_SERVICES_RD) {
            si->rd_34.data_format_identifier = sconfig.service_34_rd.data_format_identifier;
            si->rd_34.address_and_length_format_identifier = sconfig.service_34_rd.address_and_length_format_identifier;
            si->rd_34.memory_address = sconfig.service_34_rd.memory_address;            
            si->rd_34.memory_size = sconfig.service_34_rd.memory_size;            
        }
        else if (si->sid == UDS_SERVICES_RFT) {
            /* 38�����ǿ�ѡ���ݣ����ﶯ̬����һ���ڴ� */
            if (si->rft_38 == NULL) {
                si->rft_38 = ycalloc(sizeof(*si->rft_38), 1);
                if (si->rft_38 == NULL) {            
                    to_recode = DM_ERR_UDS_SERVICE_ADD;
                    log_e("fatal error UDS service add failed");
                }
            }
            if (si->rft_38) {
                si->rft_38->mode_of_operation = sconfig.service_38_rft.mode_of_operation;
                si->rft_38->file_path_and_name_length = sconfig.service_38_rft.file_path_and_name_length;
                si->rft_38->file_path_and_name = strdup(sconfig.service_38_rft.file_path_and_name);
                si->rft_38->data_format_identifier = sconfig.service_38_rft.data_format_identifier;            
                si->rft_38->file_size_parameter_length = sconfig.service_38_rft.file_size_parameter_length;
                si->rft_38->file_size_uncompressed = sconfig.service_38_rft.file_size_uncompressed;
                si->rft_38->file_size_compressed = sconfig.service_38_rft.file_size_compressed;
            }
        }
        else if (si->sid == UDS_SERVICES_SA) {
            yudsc_security_access_keygen_callback_set(udsc, (security_access_keygen_callback)dm_security_access_keygen_callback, dm);
        }
        
        if (si->sid == UDS_SERVICES_TD) {
            si->td_36.local_path = strdup(sconfig.local_path);
            yudsc_36_transfer_progress_callback_set(udsc, (service_36_transfer_progress_callback)dm_service_36_transfer_progress_callback, dm);
        }
        yudsc_service_request_build(si);
#ifdef __HAVE_UDS_PROTOCOL_ANALYSIS__
        yuds_protocol_parse(y_byte_array_const_data(si->request_byte), y_byte_array_count(si->request_byte)); 
#endif /* __HAVE_UDS_PROTOCOL_ANALYSIS__ */
    }
    else {
        to_recode = DM_ERR_UDS_SERVICE_ADD;
        log_e("fatal error UDS service add failed");
    }
    y_byte_array_delete(sconfig.variableByte);
    y_byte_array_delete(sconfig.expectResponByte);
    y_byte_array_delete(sconfig.finishByte);
    log_d("Event type: 0x%02X(%s) Recode: %d(%s) udsc ID: %d", \
        evtype, ydm_ipc_event_str(evtype), to_recode, ydm_event_rcode_str(to_recode), to_id);
    ydm_common_encode(DM_TX_BUFF(dm), evtype, to_recode, to_id, IPC_USE_USER_TIME_STAMP);
    dm_sendto_api(dm, dm_txbuf(dm), DM_IPC_EVENT_MSG_SIZE);

    return ;
}

static void dm_ipc_event_runtime_config_handler(ydiag_master *dm, yuint32 recode, yuint16 udscid)
{
    yuint32 evtype = DM_EVENT_UDS_CLIENT_GENERAL_CFG_ACCEPT;
    yuint32 to_recode = DM_ERR_NO;
    yuint16 to_id = udscid;
    udsc_runtime_config gconfig;
    yuint8 *dp = NULL;
    
    uds_client *udsc = dm_udsc(dm, udscid);
    dp = pointer_offset_nbyte(dm_rxbuf(dm), DM_IPC_EVENT_MSG_SIZE);
    log_d("Service config: %s", dp); 

    memset(&gconfig, 0, sizeof(gconfig));
    ydm_general_config_decode(dp, strlen((const char *)dp), &gconfig); 
    /* �����Ƿ���Ϸ���ִ�г��ַ�Ԥ�ڽ������ֹUDS�ͻ���ִ�� */
    yudsc_fail_abort(udsc, gconfig.isFailAbort);
    /* ����3E���ĵķ����߼� */
    yudsc_tester_present_config(udsc, gconfig.tester_present_enable, gconfig.is_tester_present_refresh, gconfig.tester_present_interval, gconfig.tester_present_ta, gconfig.tester_present_sa);
    /*  */
    yudsc_td_36_progress_interval_set(udsc, gconfig.td_36_notify_interval);

    yudsc_runtime_data_statis_enable(udsc, gconfig.runtime_statis_enable);
    
    yudsc_asc_record_enable_set(udsc, gconfig.uds_asc_record_enable);
    yudsc_msg_parse_enable_set(udsc, gconfig.uds_msg_parse_enable);
    log_d("Event type: 0x%02X(%s) Recode: %d(%s) udsc ID: %d", \
        evtype, ydm_ipc_event_str(evtype), to_recode, ydm_event_rcode_str(to_recode), to_id);
    ydm_common_encode(DM_TX_BUFF(dm), evtype, to_recode, to_id, IPC_USE_USER_TIME_STAMP);
    dm_sendto_api(dm, dm_txbuf(dm), DM_IPC_EVENT_MSG_SIZE);

    return ;
}

/* ����api��key������ */
static void dm_ipc_event_sa_key_handler(ydiag_master *dm, yuint32 recode, yuint16 udscid)
{
    uds_client *udsc = NULL;
    yuint32 evtype = DM_EVENT_UDS_SERVICE_27_SA_KEY_ACCEPT;
    yuint32 to_recode = DM_ERR_NO;
    yuint16 to_id = udscid;
    yuint16 key_size = 0;
    yuint8 *key = 0;
    yuint8 level = 0;
    
    log_d("Event type: 0x%02X(%s) Recode: %d(%s) udsc ID: %d", \
        evtype, ydm_ipc_event_str(evtype), to_recode, ydm_event_rcode_str(to_recode), to_id);
    ydm_common_encode(DM_TX_BUFF(dm), evtype, to_recode, to_id, IPC_USE_USER_TIME_STAMP);
    dm_sendto_api(dm, dm_txbuf(dm), DM_IPC_EVENT_MSG_SIZE);

    /* ��ȡ��api����õ���key */
    level = TYPE_CAST_UCHAR(pointer_offset_nbyte(dm_rxbuf(dm), SERVICE_SA_KEY_LEVEL_OFFSET));
    key_size = TYPE_CAST_USHORT(pointer_offset_nbyte(dm_rxbuf(dm), SERVICE_SA_KEY_SIZE_OFFSET));
    key = pointer_offset_nbyte(dm_rxbuf(dm), SERVICE_SA_KEY_OFFSET); 
    log_d("Security access level %02x", level);
    log_hex_d("UDS Request key", key, key_size);
    udsc = dm_udsc(dm, udscid);
    /* key����uds clientʹ�� */
    yudsc_security_access_key_set(udsc, key, key_size);

    return ;
}

/* ��ָ����uds client�ͻ����ϴ���һ��doip�ͻ��� */
static void dm_ipc_event_doipc_create_handler(ydiag_master *dm, yuint32 recode, yuint16 udscid)
{
    yuint32 evtype = DM_EVENT_DOIP_CLIENT_CREATE_ACCEPT;
    yuint32 to_recode = DM_ERR_NO;
    yuint16 to_id = udscid;
    doipc_config_t config;
    doip_client_t *doipc = NULL; int doipc_index = 0;
    uds_client *udsc = NULL;

    udsc = dm_udsc(dm, udscid);
    memset(&config, 0, sizeof(config));
 
    ydm_doipc_create_config_decode(pointer_offset_nbyte(dm_rxbuf(dm), DM_IPC_EVENT_MSG_SIZE), \
        dm_rxbuf_size(dm) - DM_IPC_EVENT_MSG_SIZE, &config);
    /* �ж�һ�¸�UDS�ͻ����Ƿ��Ѿ�������doip�ͻ��� */
    doipc = yudsc_doip_channel(udsc);
    if (doipc == NULL) {
        /* ����һ���µ�DOIP�ͻ��� */
        doipc = ydoipc_create(ydiag_master_ev_loop(dm));
    }
    /* doip�ͻ��˴����ɹ� */
    if (doipc) {
        memappend(pointer_offset_nbyte(dm_txbuf(dm), DOIPC_CREATE_DOIP_CLIENT_ID_OFFSET), \
            &doipc_index, DOIPC_CREATE_DOIP_CLIENT_ID_SIZE);
        /* UDS�ͻ��˺�DOIP�ͻ��˰� */
        yudsc_doip_channel_bind(udsc, doipc);
        /* ֱ������ doip server */
        ydoipc_connect_active_server(doipc, &config);
    }
    else {
        to_recode = DM_ERR_DOIPC_CREATE_FAILED;
    }
    log_d("Event type: 0x%02X(%s) Recode: %d(%s) udsc ID: %d", \
    evtype, ydm_ipc_event_str(evtype), to_recode, ydm_event_rcode_str(to_recode), to_id);
    ydm_common_encode(DM_TX_BUFF(dm), evtype, to_recode, to_id, IPC_USE_USER_TIME_STAMP);
    dm_sendto_api(dm, dm_txbuf(dm), DOIPC_CREATE_REPLY_SIZE);

    return ;
}

/* master��Ӧuds client�������ܲ���ͳ�ƣ���Ҫ�ڴ���uds clientʱʹ�ܸù����ǲ��ܻ�ȡ�� */
static void dm_ipc_event_udsc_rundata_statis_handler(ydiag_master *dm, yuint32 recode, yuint16 udscid)
{
    yuint32 evtype = DM_EVENT_UDS_CLIENT_RUNTIME_DATA_STATIS_ACCEPT;
    yuint32 to_recode = DM_ERR_NO;
    yuint16 to_id = udscid;
    runtime_data_statis_t statis;
    uds_client *udsc = NULL;
    int clen = 0;

    udsc = dm_udsc(dm, udscid);

    memset(&statis, 0, sizeof(statis));
    if (!yudsc_runtime_data_statis_get(udsc, &statis)) {
        to_recode = DM_ERR_UDSC_RUNDATA_STATIS;
    }
    else {        
        clen = ydm_udsc_rundata_statis_encode(pointer_offset_nbyte(dm_txbuf(dm), DM_IPC_EVENT_MSG_SIZE), \
            dm_txbuf_size(dm) - DM_IPC_EVENT_MSG_SIZE, &statis);
    }
    log_d("Event type: 0x%02X(%s) Recode: %d(%s) udsc ID: %d", \
    evtype, ydm_ipc_event_str(evtype), to_recode, ydm_event_rcode_str(to_recode), to_id);
    ydm_common_encode(DM_TX_BUFF(dm), evtype, to_recode, to_id, IPC_USE_USER_TIME_STAMP);
    dm_sendto_api(dm, dm_txbuf(dm), DM_IPC_EVENT_MSG_SIZE + clen);

    return ;
}

/* master��Ӧָ����uds client�Ƿ�����ִ�������� */
static void dm_ipc_event_check_udsc_active_handler(ydiag_master *dm, yuint32 recode, yuint16 udscid)
{
    yuint32 evtype = DM_EVENT_UDS_CLIENT_ACTIVE_CHECK_ACCEPT;
    yuint32 to_recode = DM_ERR_NO;
    yuint16 to_id = udscid;
    yuint32 isactive = true;
    uds_client *udsc = NULL;

    udsc = dm_udsc(dm, udscid);
    isactive = yudsc_service_isactive(udsc);
    memappend(pointer_offset_nbyte(dm_txbuf(dm), UDSC_IS_ACTIVE_OFFSET), &isactive, UDSC_IS_ACTIVE_SIZE);    

    log_d("UDS client isactive => %d", isactive);
    log_d("Event type: 0x%02X(%s) Recode: %d(%s) udsc ID: %d", \
    evtype, ydm_ipc_event_str(evtype), to_recode, ydm_event_rcode_str(to_recode), to_id);
    ydm_common_encode(DM_TX_BUFF(dm), evtype, to_recode, to_id, IPC_USE_USER_TIME_STAMP);
    dm_sendto_api(dm, dm_txbuf(dm), UDSC_IS_ACTIVE_REPLY_SIZE);

    return ;
}

/* master��Ӧ�����ǻ�Ծ�� */
static void dm_ipc_event_check_api_valid_handler(ydiag_master *dm, yuint32 recode, yuint16 udscid)
{
    yuint32 evtype = DM_EVENT_OMAPI_CHECK_VALID_ACCEPT;
    yuint32 to_recode = DM_ERR_NO;
    yuint16 to_id = udscid;

    log_d("Event type: 0x%02X(%s) Recode: %d(%s) udsc ID: %d", \
    evtype, ydm_ipc_event_str(evtype), to_recode, ydm_event_rcode_str(to_recode), to_id);
    ydm_common_encode(DM_TX_BUFF(dm), evtype, to_recode, to_id, IPC_USE_USER_TIME_STAMP);
    dm_sendto_api(dm, dm_txbuf(dm), DM_IPC_EVENT_MSG_SIZE);

    return ;
}

static int dm_terminal_control_service_find(ydiag_master *dm, terminal_control_service_info_t *tcsinfo)
{
    int trindex = 0;

    for (trindex = 0; trindex < TERMINAL_REMOTE_CAPACITY_MAX; trindex++) {
        if (dm->tcss[trindex]) {
            if (yterminal_control_service_info_equal(dm->tcss[trindex], tcsinfo)) {
                return trindex;
            }
        }
    }

    return -1;
}

static int dm_terminal_control_service_add(ydiag_master *dm, terminal_control_service_t *tcs)
{
    int tcsid = -1;
    int trindex = 0;
    terminal_control_service_info_t tcsinfo;

    memset(&tcsinfo, 0, sizeof(tcsinfo));
    /* �����Ƿ��ܴ����µ��ն˿��Ʒ��� */
    for (trindex = 0; trindex < TERMINAL_REMOTE_CAPACITY_MAX; trindex++) {
        if (dm->tcss[trindex] == NULL) {
            tcsid = trindex;
            dm->tcss[trindex] = tcs;
            break;
        }
    }

    return tcsid;
}

static int dm_terminal_control_service_del(ydiag_master *dm, int tcsid)
{
    if (!TCS_IS_VALID(tcsid)) {
        return -1;
    }

    dm->tcss[tcsid] = NULL;

    return tcsid;
}

static void dm_ipc_event_terminal_control_service_create_handler(ydiag_master *dm, yuint32 recode, yuint16 udscid)
{
    yuint32 evtype = DM_EVENT_TERMINAL_CONTROL_SERVICE_CREATE_ACCEPT;
    yuint32 to_recode = DM_ERR_NO;
    yuint16 to_id = udscid;
    terminal_control_service_info_t tcsinfo;
    yuint32 tcsid = 0;
    terminal_control_service_t *tcs = NULL; /* ����һ���µ��ն˿��Ʒ��� */

    memset(&tcsinfo, 0, sizeof(tcsinfo));
    ydm_terminal_control_service_info_decode(pointer_offset_nbyte(dm_rxbuf(dm), DM_IPC_EVENT_MSG_SIZE), \
        dm_rxbuf_size(dm) - DM_IPC_EVENT_MSG_SIZE, &tcsinfo);
    if (dm_terminal_control_service_find(dm, &tcsinfo) >= 0) {
        /* ���ն˿��Ʒ����Ѿ����� */
        to_recode = DM_ERR_TCS_EXIST;
        log_w("The terminal control service already exists");
    }
    else {
        /* �����µ��ն˿��Ʒ��� */
        tcs = yterminal_control_service_create(dm->loop);
    }
    /* �µ� */
    if (tcs) {
        /* �������� */
        yterminal_control_service_info_set(tcs, &tcsinfo);
        /* ����һ���´������ն˿��Ʒ��� */
        tcsid = dm_terminal_control_service_add(dm, tcs);
        if (!TCS_IS_VALID(tcsid)) {
            /* �ն˿��Ʒ������ʧ�� */
            yterminal_control_service_destroy(tcs);              
            to_recode = DM_ERR_TCS_OUT_NUM_MAX;        
            log_w("The terminal control service exceeds the upper limit");
        }
        else {
            /* ���񴴽��ɹ���ʼִ������ƽ̨���� */
            yterminal_control_service_connect(tcs);
        }        
    }
    else {
        /* ����ʧ�� */
        to_recode = DM_ERR_TCS_CREATE_FAILED;
        log_e("The terminal control service fails to be created");
    }

    log_d("Terminal control service create => %d", tcsid);
    log_d("Event type: 0x%02X(%s) Recode: %d(%s) udsc ID: %d", \
    evtype, ydm_ipc_event_str(evtype), to_recode, ydm_event_rcode_str(to_recode), to_id);    
    memappend(pointer_offset_nbyte(dm_txbuf(dm), TERMINAL_CONTROL_SERVICE_ID_OFFSET), \
        &tcsid, TERMINAL_CONTROL_SERVICE_ID_SIZE);
    ydm_common_encode(DM_TX_BUFF(dm), evtype, to_recode, to_id, IPC_USE_USER_TIME_STAMP);
    dm_sendto_api(dm, dm_txbuf(dm), TERMINAL_CONTROL_SERVICE_SIZE);

    return ;
}

static void dm_ipc_event_terminal_control_service_destroy_handler(ydiag_master *dm, yuint32 recode, yuint16 udscid)
{
    yuint32 evtype = DM_EVENT_TERMINAL_CONTROL_SERVICE_DESTORY_ACCEPT;
    yuint32 to_recode = DM_ERR_NO;
    yuint16 to_id = udscid;
    yuint32 tcsid = 0;
    terminal_control_service_t *tcs = NULL;

    tcsid = TYPE_CAST_UINT(pointer_offset_nbyte(dm_rxbuf(dm), TERMINAL_CONTROL_SERVICE_ID_OFFSET));
    log_d("Terminal control service destroy => %d", tcsid);
    if (TCS_IS_VALID(tcsid)) {
        /* id������������ֱ���ҵ� */
        tcs = dm->tcss[tcsid];
        if (tcs) {
            /* ���ٷ��� */
            yterminal_control_service_destroy(tcs); 
            /* ɾ������ */
            dm_terminal_control_service_del(dm, tcsid);
        }
        else {
            to_recode = DM_ERR_TCS_INVALID;
            log_w("The terminal control service invalid");
        }
    }
    else {
        to_recode = DM_ERR_TCS_INVALID;
    }
    log_d("Event type: 0x%02X(%s) Recode: %d(%s) udsc ID: %d", \
    evtype, ydm_ipc_event_str(evtype), to_recode, ydm_event_rcode_str(to_recode), to_id);    
    ydm_common_encode(DM_TX_BUFF(dm), evtype, to_recode, to_id, IPC_USE_USER_TIME_STAMP);
    dm_sendto_api(dm, dm_txbuf(dm), DM_IPC_EVENT_MSG_SIZE);

    return ;
}

static void dm_ipc_event_instance_destroy_handler(ydiag_master *dm, yuint32 recode, yuint16 udscid)
{
    log_i("The diag master %s instance destroy", dm->name);
    dm_ev_stop(dm);     
}

/* ����������Ϣ��api����ȷ��api�Ƿ���ʹ���У���ֹʧЧ��apiռ����Դ */
int dm_keepalive_event_emit(ydiag_master *dm)
{
    yuint32 evtype = DM_EVENT_OMAPI_KEEPALIVE_EMIT;
    yuint32 recode = DM_ERR_NO;
  
    ydm_common_encode(DM_TX_BUFF(dm), evtype, recode, 0, IPC_USE_USER_TIME_STAMP);
    dm_event_emit_to_api(dm, dm_txbuf(dm), DM_IPC_EVENT_MSG_SIZE);
    
    return 0;
}

static void dm_ev_timer_keepalive_handler(struct ev_loop *loop, ev_timer *w, int revents)
{
    ydiag_master *dm = (ydiag_master *)container_of(w, ydiag_master, keepalive_watcher);
    dm->keepalive_cnt++;
    dm_keepalive_event_emit(dm);
    if (dm->keepalive_cnt > 10) {           
        log_put_mdc(DM_DIAG_MASTER_MDC_KEY, dm->name);
        log_put_mdc(DM_UDSC_MDC_KEY, "*");
        log_w("The diag master api is offline. The diag master exits");
        dm_ev_stop(dm);     
    }
}

static void dm_ipc_event_handler_add(ydiag_master *dm, int evtype, dm_event_handler_callback call)
{
    if (IPC_CMD_IS_VALID(evtype & (~DM_EVENT_ACCEPT_MASK))) {
        dm->event_calls[evtype & (~DM_EVENT_ACCEPT_MASK)].call = call;
    }
}

static void dm_ev_io_dm_ipc_read(struct ev_loop* loop, ev_io* w, int e)
{
    ydiag_master* dm = (ydiag_master*)container_of(w, ydiag_master, iread_watcher);
    yuint32 evtype = 0; yuint32 recode = 0; yuint16 udscid = 0;
    yuint32 tv_sec = 0; yuint32 tv_usec = 0;
    int recvByte = -1;
    
    recvByte = ydm_recvfrom(dm->sockfd, DM_RX_BUFF(dm), 0);
    if (recvByte > 0) {
        dm->dms->logfile_cycle_period = 60.0;
        dm_logfile_limit_cycle_period_refresh(dm->dms);
        /* ˢ��������ʱ�� */
        dm_keepalive_refresh(dm);
        ydm_common_decode(DM_RX_BUFF(dm), &evtype, &recode, &udscid, &tv_sec, &tv_usec);
        if (DM_EVENT_ACCEPT_MASK & evtype) {
            /* ֻ����������Ϣ��Ӧ����Ϣֱ�Ӻ��� */
            if (evtype == DM_EVENT_OMAPI_KEEPALIVE_ACCEPT) {
                /* ���յ�������Ӧ��ˢд���������� */
                dm->keepalive_cnt = 0;
            }
            return ;
        }
            
        if (evtype != DM_EVENT_UDS_CLIENT_CREATE_EMIT && \
            evtype != DM_EVENT_DIAG_MASTER_RESET_EMIT && \
            evtype != DM_EVENT_OMAPI_CHECK_VALID_EMIT && \
            evtype != DM_EVENT_TERMINAL_CONTROL_SERVICE_CREATE_EMIT && \
            evtype != DM_EVENT_TERMINAL_CONTROL_SERVICE_DESTORY_EMIT && \
            evtype != DM_EVENT_INSTANCE_DESTORY_EMIT) {
            /* ��Ҫ�õ�uds�ͻ���ID�����������ж�ID�Ƿ���Ч */
            uds_client *udsc = dm_udsc(dm, udscid);
            if (udsc == NULL) {
                log_w("The udsc id is invalid => %d", udscid);
                recode = DM_ERR_UDSC_INVALID;
                goto REPLY;
            }            
            log_put_mdc(DM_DIAG_MASTER_MDC_KEY, dm->name);
            log_put_mdc(DM_UDSC_MDC_KEY, udsc->udsc_name);
        }
        else {   
        
            log_put_mdc(DM_DIAG_MASTER_MDC_KEY, dm->name);
            log_put_mdc(DM_UDSC_MDC_KEY, "*");
        }
        
        if (evtype != DM_EVENT_OMAPI_KEEPALIVE_ACCEPT) {
            log_d("Request event: 0x%02X(%s) Recode: %d(%s) udsc ID: %d tv_sec: %d tv_usec: %d", \
                evtype, ydm_ipc_event_str(evtype), recode, ydm_event_rcode_str(recode), udscid, tv_sec, tv_usec);
        }

        if (ydm_common_is_system_time_stamp(tv_sec, tv_usec)) {
            /* ʹ��ϵͳʱ�������Ϣ��Ҫ�ж���Ϣ��Ч�� */
            if (dm->premsg_tv_sec == tv_sec && \
                dm->premsg_tv_usec == tv_usec) {
                /* ��Ϣȥ�أ���ֹ�ط���Ϣ���ִ�� */
                return ;
            }

            if (dm->event_valid_time > 0 && \
                ydm_tv_currtoms(tv_sec, tv_usec) > (dm->event_valid_time - 1)) {
                log_w("When the event expires Ignore the event."); 
                unsigned int curr_tv_sec = 0; unsigned int curr_tv_usec = 0;
                ydm_tv_get(&curr_tv_sec, &curr_tv_usec);
                log_w("curr_tv_sec : %d  api_tv_sec : %d", curr_tv_sec, tv_sec);
                log_w("curr_tv_usec: %d  api_tv_usec: %d", curr_tv_usec, tv_usec);
                log_w("event_valid_time : %d", dm->event_valid_time);
                return ;    
            }
        }
       
        if (IPC_CMD_IS_VALID(evtype & (~DM_EVENT_ACCEPT_MASK)) && \
            dm->event_calls[evtype].call) {
            if (ydm_common_is_system_time_stamp(tv_sec, tv_usec)) {
                dm->premsg_tv_sec = tv_sec; /* ��¼�ɹ�ִ���¼���ʱ��� */
                dm->premsg_tv_usec = tv_usec; /* ��¼�ɹ�ִ���¼���ʱ��� */
            }
            dm->event_calls[evtype].call(dm, recode, udscid);
        }
        else {
            log_d("diag master IPC unknown event => %d", evtype);    
        }
        memset(dm_rxbuf(dm), 0, recvByte);
    }

    return ;
REPLY:
    log_d("Event type: 0x%02X(%s) Recode: %d(%s) udsc ID: %d", \
        evtype | DM_EVENT_ACCEPT_MASK, ydm_ipc_event_str(evtype | DM_EVENT_ACCEPT_MASK), \
        recode, ydm_event_rcode_str(recode), udscid);
    ydm_common_encode(DM_TX_BUFF(dm), evtype | DM_EVENT_ACCEPT_MASK, recode, udscid, IPC_USE_USER_TIME_STAMP);
    dm_sendto_api(dm, dm_txbuf(dm), DM_IPC_EVENT_MSG_SIZE);
    
    return ;    
}

/* ��ʱ����ʹ��������� */
static int dm_ipc_create(struct sockaddr_un *un)
{
    int handle = -1;
    int flag = 0;

    if (!un) {
        goto CREATE_FALIED;
    }

    handle = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (handle < 0) {
        goto CREATE_FALIED;    
    }
    flag = fcntl(handle, F_GETFL, 0);
    fcntl(handle, F_SETFL, flag | O_NONBLOCK);

    unlink(un->sun_path);
    if (bind(handle, (struct sockaddr *)un, sizeof(struct sockaddr_un)) < 0) {
        goto CREATE_FALIED;
    }

    return handle;
CREATE_FALIED:
    if (handle > 0) {
        close(handle);
    }

    return -1;
}
/*
    UDS�����͸�diag master api����diag master api�����UDS�����͸�����ECU
*/
static int dm_uds_service_request_event_emit(ydiag_master *dm, yuint16 udscid, const yuint8 *data, yuint32 size, yuint32 sa, yuint32 ta, yuint32 tatype)
{
    yuint8 *dp = pointer_offset_nbyte(dm_txbuf(dm), DM_IPC_EVENT_MSG_SIZE);
    yuint32 evtype = DM_EVENT_SERVICE_INDICATION_EMIT;
    yuint32 recode = DM_ERR_NO;
    yuint32 sl = DM_IPC_EVENT_MSG_SIZE + SERVICE_INDICATION_PAYLOAD_OFFSET + size;

    if (dm_txbuf_size(dm) < sl) {
        log_e("The length of the ipc send cache is insufficient. The expected length is %d, but the actual length is %d", sl, dm_txbuf_size(dm));
        return -1;
    }

    /* encode 4byte UDS����Դ��ַ */
    memappend(pointer_offset_nbyte(dp, SERVICE_INDICATION_A_SA_OFFSET), &sa, SERVICE_INDICATION_A_SA_SIZE);    
    /* encode 4byte UDS����Ŀ�ĵ�ַ */
    memappend(pointer_offset_nbyte(dp, SERVICE_INDICATION_A_TA_OFFSET), &ta, SERVICE_INDICATION_A_TA_SIZE);    
    /* encode 4byte UDS����Ŀ�ĵ�ַ���� */
    memappend(pointer_offset_nbyte(dp, SERVICE_INDICATION_TA_TYPE_OFFSET), &tatype, SERVICE_INDICATION_TA_TYPE_SIZE);    
    /* encode 4byte UDS�����ĳ��� */
    memappend(pointer_offset_nbyte(dp, SERVICE_INDICATION_PAYLOAD_LEN_OFFSET), &size, SERVICE_INDICATION_PAYLOAD_LEN_SIZE);    
    /* encode nbyte UDS������ */
    memappend(pointer_offset_nbyte(dp, SERVICE_INDICATION_PAYLOAD_OFFSET), data, size);
    ydm_common_encode(DM_TX_BUFF(dm), evtype, recode, udscid, IPC_USE_USER_TIME_STAMP);
    dm_event_emit_to_api(dm, dm_txbuf(dm), sl);

    return size;
}

/*
    ���е�UDS����������������쳣�ж�ִ��UDS����󣬽�������͸�diag master api����
    ����������һ��UDS���������Ӧ������
*/
static int dm_uds_client_result_event_emit(ydiag_master *dm, yuint16 udscid, yuint32 result, const yuint8 *ind, yuint32 indl, const yuint8 *resp, yuint32 respl)
{
    yuint32 evtype = DM_EVENT_UDS_CLIENT_RESULT_EMIT;
    yuint32 recode = result;
    yuint32 sl = DM_IPC_EVENT_MSG_SIZE +  SERVICE_FINISH_RESULT_IND_LEN_SIZE + indl + SERVICE_FINISH_RESULT_RESP_LEN_SIZE + respl;

    if (ind == NULL) {indl = 0;}
    if (resp == NULL) {respl = 0;}
    if (dm_txbuf_size(dm) <  sl) {
        log_e("The length of the ipc send cache is insufficient. The expected length is %d, but the actual length is %d", sl, dm_txbuf_size(dm));
        return -1;
    }
    /* encode 4 byte �������ݳ��� */
    memappend(pointer_offset_nbyte(dm_txbuf(dm), SERVICE_FINISH_RESULT_IND_LEN_OFFSET), &indl, SERVICE_FINISH_RESULT_IND_LEN_SIZE);
    /* encode n byte �������� */
    memappend(pointer_offset_nbyte(dm_txbuf(dm), SERVICE_FINISH_RESULT_IND_OFFSET), ind, indl);
    /* encode 4 byte Ӧ�����ݳ��� */
    memappend(pointer_offset_nbyte(dm_txbuf(dm), SERVICE_FINISH_RESULT_IND_OFFSET + indl), &respl, SERVICE_FINISH_RESULT_RESP_LEN_SIZE);    
    /* encode n byte Ӧ������ */
    memappend(pointer_offset_nbyte(dm_txbuf(dm), SERVICE_FINISH_RESULT_IND_OFFSET + indl + SERVICE_FINISH_RESULT_RESP_LEN_SIZE), resp, respl);
    /* encode ͨ�õ�ͷ���̶����� */
    ydm_common_encode(DM_TX_BUFF(dm), evtype, recode, udscid, IPC_USE_USER_TIME_STAMP);
    /* ���͸�diag master api */
    dm_event_emit_to_api(dm, dm_txbuf(dm), sl);

    return 0;
}

/* 
   ���diag master apiע����UDS��������������
   diag master ��UDS�����Ӧ�����ݣ����͸� diag master api ���� 
*/
static int dm_uds_service_result_event_emit(ydiag_master *dm, yuint16 udscid, const yuint8 *data, yuint32 size, yuint32 rr_callid)
{
    yuint32 evtype = DM_EVENT_UDS_SERVICE_RESULT_EMIT;
    yuint32 recode = DM_ERR_NO;

    /* ��ֹ���ݹ���Խ���дbuff */
    if (size > dm_txbuf_size(dm) - SERVICE_REQUEST_RESULT_SIZE - DM_IPC_EVENT_MSG_SIZE) {
        size = dm_txbuf_size(dm) - SERVICE_REQUEST_RESULT_SIZE - DM_IPC_EVENT_MSG_SIZE;
        recode = DM_ERR_UDS_RESPONSE_OVERLEN; /* ������ */
    }
    /* encode 4 byte diag master api�������Ͻ������ص�����ID */
    memappend(pointer_offset_nbyte(dm_txbuf(dm), SERVICE_REQUEST_RESULT_RR_CALLID_OFFSET), &rr_callid, SERVICE_REQUEST_RESULT_RR_CALLID_SIZE);    
    /* encode 4 byte ��Ͻ�����ݳ��� */
    memappend(pointer_offset_nbyte(dm_txbuf(dm), SERVICE_REQUEST_RESULT_DATA_LEN_OFFSET), &size, SERVICE_REQUEST_RESULT_DATA_LEN_SIZE);    
    /* encode n byte ��Ͻ������ */
    memappend(pointer_offset_nbyte(dm_txbuf(dm), SERVICE_REQUEST_RESULT_DATA_OFFSET), data, size);
    yuint8 *payload = pointer_offset_nbyte(dm_txbuf(dm), SERVICE_REQUEST_RESULT_DATA_OFFSET);
    log_hex_d("UDS Request result: ", payload, size);    
    /* encode ͨ�õ�ͷ���̶����� */
    ydm_common_encode(DM_TX_BUFF(dm), evtype, recode, udscid, IPC_USE_USER_TIME_STAMP);
    dm_event_emit_to_api(dm, dm_txbuf(dm), DM_IPC_EVENT_MSG_SIZE + SERVICE_REQUEST_RESULT_SIZE + size);

    return 0;
}

static int dm_uds_service_27_sa_seed_event_emit(ydiag_master *dm, yuint16 udscid, yuint8 level, const yuint8 *data, yuint32 size)
{
    yuint32 evtype = DM_EVENT_UDS_SERVICE_27_SA_SEED_EMIT;
    yuint32 recode = DM_ERR_NO;
    yuint32 sl = SERVICE_SA_SEED_OFFSET + size;

    if (dm_txbuf_size(dm) < sl) {
        log_e("The length of the ipc send cache is insufficient. The expected length is %d, but the actual length is %d", sl, dm_txbuf_size(dm));
        return -1;
    }

    /* encode 1 byte level */
    memappend(pointer_offset_nbyte(dm_txbuf(dm), SERVICE_SA_SEED_LEVEL_OFFSET), &level, SERVICE_SA_SEED_LEVEL_SIZE);    
    /* encode 2 byte ���ӳ��� */
    memappend(pointer_offset_nbyte(dm_txbuf(dm), SERVICE_SA_SEED_SIZE_OFFSET), &size, SERVICE_SA_SEED_SIZE_SIZE);    
    /* encode n byte �������� */
    memappend(pointer_offset_nbyte(dm_txbuf(dm), SERVICE_SA_SEED_OFFSET), data, size);    
    log_hex_d("UDS Request seed: ", data, size);    
    /* encode ͨ�õ�ͷ���̶����� */
    ydm_common_encode(DM_TX_BUFF(dm), evtype, recode, udscid, IPC_USE_USER_TIME_STAMP);
    dm_event_emit_to_api(dm, dm_txbuf(dm), sl);

    return 0;
}

static int dm_uds_service_36_transfer_progress_event_emit(ydiag_master *dm, yuint16 udscid, yuint32 file_size, yuint32 total_byte, yuint32 elapsed_time)
{
    yuint32 evtype = DM_EVENT_36_TRANSFER_PROGRESS_EMIT;
    yuint32 recode = DM_ERR_NO;
    yuint32 sl = IPC_36_TRANSFER_PROGRESS_MSG_SIZE;

    if (dm_txbuf_size(dm) < sl) {
        log_e("The length of the ipc send cache is insufficient. The expected length is %d, but the actual length is %d", sl, dm_txbuf_size(dm));
        return -1;
    }
    
    /* encode 1 byte level */
    memappend(pointer_offset_nbyte(dm_txbuf(dm), IPC_36_TRANSFER_FILE_SIZE_OFFSET), &file_size, IPC_36_TRANSFER_FILE_SIZE_SIZE);    
    memappend(pointer_offset_nbyte(dm_txbuf(dm), IPC_36_TRANSFER_TOTAL_BYTE_OFFSET), &total_byte, IPC_36_TRANSFER_TOTAL_BYTE_SIZE);    
    memappend(pointer_offset_nbyte(dm_txbuf(dm), IPC_36_TRANSFER_ELAPSED_TIME_OFFSET), &elapsed_time, IPC_36_TRANSFER_ELAPSED_TIME_SIZE);    
    /* encode ͨ�õ�ͷ���̶����� */
    ydm_common_encode(DM_TX_BUFF(dm), evtype, recode, udscid, IPC_USE_USER_TIME_STAMP);
    dm_event_emit_to_api(dm, dm_txbuf(dm), sl);

    return 0;
}

static int dm_instance_destroy_event_emit(ydiag_master *dm)
{
    /* encode ͨ�õ�ͷ���̶����� */
    ydm_common_encode(DM_TX_BUFF(dm), DM_EVENT_INSTANCE_DESTORY_EMIT, 0, 0, IPC_USE_USER_TIME_STAMP);
    dm_event_emit_to_api(dm, dm_txbuf(dm), DM_IPC_EVENT_MSG_SIZE);

    return 0;
}

ydiag_master *dm_create(DMS *dms, const char *dm_path, const char *dmapi_path)
{
    int ret = 0;
    int udsc_id = 0;
    struct sockaddr_un un;

    if (dm_path == NULL || \
        dmapi_path == NULL || \
        dms == NULL) {
        return NULL;
    }
    ydiag_master *dm = ymalloc(sizeof(*dm));
    if (dm == NULL) {
        return NULL;
    }
    memset(dm, 0, sizeof(*dm));
    dm->index = -1;
    dm->sockfd = -1;
#ifdef __HAVE_DIAG_MASTER_PTHREAD__
    /* �����̺߳���Ҫ�����µ��¼�ѭ��loop */
    dm->loop = ev_loop_new(0);
    if (dm->loop == NULL) {
        log_e("new event loop failed ");
        goto CREAT_FAILED;
    }
#else /* __HAVE_DIAG_MASTER_PTHREAD__ */
    /* diag master ��ʹ���߳̽����¼�ѭ�� */
    /* ����һ���¼�ѭ��loop */
    dm->loop = dms->loop;
#endif /* __HAVE_DIAG_MASTER_PTHREAD__ */
    dm->event_valid_time = IPC_MSG_VALID_TIME;
    dm->keepalive_interval = 10000; /* ms */
    dm->dms = NULL;

    memset(&un, 0, sizeof(struct sockaddr_un));
    un.sun_family = AF_UNIX;
    memcpy(un.sun_path, dm_path, strlen(dm_path));
    dm->sockfd = ydm_ipc_channel_create(&un, dm_rxbuf_size(dm), dm_txbuf_size(dm));
    if (dm->sockfd < 0) {
        log_e("fatal error yom_ipc_channel_create failed");
        goto CREAT_FAILED;
    }
    /* ����diag master��unix socket��·�� */
    snprintf(dm->dm_path, sizeof(dm->dm_path), "%s", dm_path);

    /* ����diag master api��unix socket��·�� */
    snprintf(dm->dmapi_event_emit_path, sizeof(dm->dmapi_event_emit_path), "%s", dmapi_path);
#ifdef __HAVE_DIAG_MASTER_PTHREAD__   
    /* �����û�����ָ��, �����ڸ����ص�������ʹ�� */
    ev_set_userdata(dm->loop, dm);
#endif /* __HAVE_DIAG_MASTER_PTHREAD__ */

    /* ����IPC���¼� */
    ev_io_init(&dm->iread_watcher, dm_ev_io_dm_ipc_read, dm->sockfd, EV_READ);
    ev_io_start(dm->loop, &dm->iread_watcher);

    /* ����IPCд�¼� */
    // ev_io_init(&dm->iwrite_watcher, dm_ipc_write, dm->sockfd, EV_WRITE);
    /* �����������߼�� */
    ev_timer_init(&dm->keepalive_watcher, dm_ev_timer_keepalive_handler, \
        dm->keepalive_interval * 0.001, dm->keepalive_interval * 0.001);
    ev_timer_start(dm->loop, &dm->keepalive_watcher);

    /* ��ʼ������Ĭ����Ͽͻ��ˣ������Ͳ���Ҫ�ظ��Ĵ������� */
    for (udsc_id = 0; udsc_id < DM_UDSC_CAPACITY_DEF; udsc_id++) {
        uds_client *udsc = yudsc_create();
        if (udsc) {            
            yudsc_ev_loop_set(udsc, ydiag_master_ev_loop(dm));
            udsc->id = udsc_id;
            dm->udscs[udsc->id] = udsc;
            dm->udsc_cnt++;
        }
    }

    /* ���̼�ͨ����Ϣ������ */
    dm_ipc_event_handler_add(dm, DM_EVENT_SERVICE_RESPONSE_EMIT, \
                                    dm_ipc_event_service_response_handler);
    dm_ipc_event_handler_add(dm, DM_EVENT_UDS_CLIENT_CREATE_EMIT, \
                                    dm_ipc_event_udsc_create_handler);
    dm_ipc_event_handler_add(dm, DM_EVENT_UDS_CLIENT_DESTORY_EMIT, \
                                    dm_ipc_event_udsc_destroy_handler);
    dm_ipc_event_handler_add(dm, DM_EVENT_START_UDS_CLIENT_EMIT, \
                                    dm_ipc_event_udsc_start_handler);
    dm_ipc_event_handler_add(dm, DM_EVENT_STOP_UDS_CLIENT_EMIT, \
                                    dm_ipc_event_udsc_stop_handler);
    dm_ipc_event_handler_add(dm, DM_EVENT_DIAG_MASTER_RESET_EMIT, \
                                    dm_ipc_event_master_reset_handler);
    dm_ipc_event_handler_add(dm, DM_EVENT_UDS_SERVICE_ITEM_ADD_EMIT, \
                                    dm_ipc_event_service_config_handler);
    dm_ipc_event_handler_add(dm, DM_EVENT_UDS_CLIENT_GENERAL_CFG_EMIT, \
                                    dm_ipc_event_runtime_config_handler);
    dm_ipc_event_handler_add(dm, DM_EVENT_UDS_SERVICE_27_SA_KEY_EMIT, \
                                    dm_ipc_event_sa_key_handler);
    dm_ipc_event_handler_add(dm, DM_EVENT_DOIP_CLIENT_CREATE_EMIT, \
                                    dm_ipc_event_doipc_create_handler);
    dm_ipc_event_handler_add(dm, DM_EVENT_UDS_CLIENT_RUNTIME_DATA_STATIS_EMIT, \
                                    dm_ipc_event_udsc_rundata_statis_handler);
    dm_ipc_event_handler_add(dm, DM_EVENT_UDS_CLIENT_ACTIVE_CHECK_EMIT, \
                                    dm_ipc_event_check_udsc_active_handler);
    dm_ipc_event_handler_add(dm, DM_EVENT_OMAPI_CHECK_VALID_EMIT, \
                                    dm_ipc_event_check_api_valid_handler);
    dm_ipc_event_handler_add(dm, DM_EVENT_TERMINAL_CONTROL_SERVICE_CREATE_EMIT, \
                                    dm_ipc_event_terminal_control_service_create_handler);
    dm_ipc_event_handler_add(dm, DM_EVENT_TERMINAL_CONTROL_SERVICE_DESTORY_EMIT, \
                                    dm_ipc_event_terminal_control_service_destroy_handler);
    dm_ipc_event_handler_add(dm, DM_EVENT_INSTANCE_DESTORY_EMIT, \
                                    dm_ipc_event_instance_destroy_handler);
    
    return dm;
CREAT_FAILED:
    log_d(" diag master create error");
    if (dm->sockfd > 0) {
        close(dm->sockfd);
    }
    if (dm->loop) {
        ev_loop_destroy(dm->loop);
    }
    if (dm) {
        yfree(dm);
    }

    Y_UNUSED(ret);

    return NULL;
}

void dm_destroy(ydiag_master *dm)
{
    int udsc_index = 0;

    /* ֪ͨdiag master�������� */
    dm_instance_destroy_event_emit(dm);

    /* ֹͣ��Ծ��UDS�ͻ��˲����� */
    for (udsc_index = 0; udsc_index < dm->udsc_cnt; udsc_index++) {
        if (dm->udscs[udsc_index]) {
            yudsc_services_stop(dm->udscs[udsc_index]);
            yudsc_destroy(dm->udscs[udsc_index]); /* �����ͷ�UDS�ͻ��� */
            dm->udscs[udsc_index] = NULL;
        }
    }

#ifdef __HAVE_DIAG_MASTER_PTHREAD__ 
    /* �����߳���Ҫ�����¼�ѭ��loop */
    if (dm->loop) {
        dm_ev_stop(dm);
        ev_loop_destroy(dm->loop);
    }
#endif /* __HAVE_DIAG_MASTER_PTHREAD__ */

    if (dm->sockfd > 0) {       
        close(dm->sockfd);
        unlink(dm->dm_path);
        unlink(dm->dmapi_event_emit_path);
        unlink(dm->dmapi_event_listen_path);
    }
    
    DMS_LOCK;
    if (dm->dms && \
        dm->index < DMS_DIAG_MASTER_NUM_MAX) {
        /* �Ƴ�diag master��oms�ϵļ�¼ */
        dm->dms->dm[dm->index] = 0;
    }
    DMS_UNLOCK;
    memset(dm, 0, sizeof(*dm));
    
    yfree(dm);
}


struct ev_loop *ydiag_master_ev_loop(ydiag_master *dm)
{
    return dm->loop;
}

void *dm_thread_run(void *arg)
{
    ydiag_master *dm = (ydiag_master *)arg;
    if (dm == NULL) {
        log_d("Parameter error Thread exits");
        return 0;
    }
    pthread_detach(pthread_self());
    /* ��ʼ�¼�ѭ��������ʱ�̶��ô����¼������û���¼��������߳��˳� */
    log_d("diag master event loop start.");

    ev_run(ydiag_master_ev_loop(dm), 0);
    /* �¼�ѭ���˳�����diag master */
    dm_destroy(dm);
    log_d("diag master event loop exit thread exits.");

    return 0;
}

static void dm_ev_io_oms_read(struct ev_loop *loop, ev_io *w, int e)
{
    DMS *dms = ev_userdata(loop);
    yuint32 om_cmd = 0; yuint32 recode = DM_ERR_NO; yuint16 udscid = 0;
    yuint32 tv_sec = 0; yuint32 tv_usec = 0;
    struct sockaddr_un un;
    int len = sizeof(struct sockaddr_un);
    int bytes_recv = -1;
    char dm_path[256] = {0};
    yuint32 om_index = 0;
    yuint32 path_len = 0;
    omapi_connect_master_t cf;
    pthread_t tidp = 0;
    ydiag_master *dm = NULL;

    memset(&cf, 0, sizeof(cf));

    memset(dms->rxbuf, 0, sizeof(dms->rxbuf));
    bytes_recv = recvfrom(dms->sockfd, dms->rxbuf, sizeof(dms->rxbuf), 0, \
                                (struct sockaddr*)&un, (socklen_t *)&len);
    if (bytes_recv > 0) {     
        ydm_common_decode(dms->rxbuf, sizeof(dms->rxbuf), &om_cmd, &recode, &udscid, &tv_sec, &tv_usec);
        log_d("Request event: 0x%02X(%s) Recode: %d udsc ID: %d tv_sec: %d tv_usec: %d", \
            om_cmd, ydm_ipc_event_str(om_cmd), recode, udscid, tv_sec, tv_usec);
        if (om_cmd == DM_EVENT_CONNECT_DIAG_MASTER_EMIT) {
            ydm_omapi_connect_master_decode(dms->rxbuf + DM_IPC_EVENT_MSG_SIZE, \
                sizeof(dms->rxbuf) - DM_IPC_EVENT_MSG_SIZE, &cf);
            if (access(cf.event_emit_path, F_OK) != 0 || \
                access(cf.event_listen_path, F_OK) != 0) {
                /* �޷���ȡdiag master api�Ĵ��� */
                recode = DM_ERR_OMAPI_UNKNOWN;
                goto REPLY;
            }
            /* �ҵ�һ�����е�diag masterλ�� */            
            DMS_LOCK;
            for (om_index = 0; om_index < DMS_DIAG_MASTER_NUM_MAX; om_index++) {
                if (dms->dm[om_index] == NULL) {
                    break;
                }
            }            
            DMS_UNLOCK;
            /* �ж������Ƿ���Ч */
            if (!(om_index < DMS_DIAG_MASTER_NUM_MAX)) {
                recode = DM_ERR_DIAG_MASTER_MAX;            
                goto REPLY;
            }
            /* �����µ�diag master��unix socket path */
            snprintf(dm_path, sizeof(dm_path), DM_UNIX_SOCKET_PATH_PREFIX"%d", om_index);
            /* �����µ�diag master */
            log_i("diag master unix socket address: %s yapi unix socket address: %s", dm_path, cf.event_emit_path);
            dm = dm_create(dms, dm_path, cf.event_emit_path);
            if (dm == NULL) {
                log_d("diag master create error");
                recode = DM_ERR_DIAG_MASTER_CREATE;
                goto REPLY;
            }    
            snprintf(dm->dmapi_event_listen_path, sizeof(dm->dmapi_event_listen_path), "%s", cf.event_listen_path);
            /* ����diag master���¼�ѭ���߳�,���ں�diag master api����IPCͨ�Ŵ��� */
#ifdef __HAVE_DIAG_MASTER_PTHREAD__
/* diag master ʹ���߳̽����¼�ѭ��,��Ҫ�����߳� */
            if (pthread_create(&tidp, 0, dm_thread_run, dm) != 0) {
                log_d(" diag master thread create failed.");  
                dm_destroy(dm); /* ����ʧ�������ͷ�diag master */              
                recode = DM_ERR_DIAG_MASTER_CREATE;
                goto REPLY;
            }
#endif /* __HAVE_DIAG_MASTER_PTHREAD__ */            
            log_i("yapi connect diag master create success.");            
            DMS_LOCK;
            dms->dm[om_index] = dm;
            snprintf(dm->name, sizeof(dm->name), "%d", om_index);
            DMS_UNLOCK;
            dm->index = om_index;
            dm->dms = dms;
            goto REPLY;
        }
    }
     
    return ;
REPLY:    
    memset(dms->txbuf, 0, sizeof(dms->txbuf));
    log_d("Event type: 0x%02X Recode: %d udsc ID: %d", DM_EVENT_CONNECT_DIAG_MASTER_ACCEPT, recode, udscid);   
    path_len = strlen(dm_path);
    memappend(pointer_offset_nbyte(dms->txbuf, DMREP_CONNECT_DM_PATH_LEN_OFFSET), &path_len, DMREP_CONNECT_DM_PATH_LEN_SIZE);    
    memappend(pointer_offset_nbyte(dms->txbuf, DMREP_CONNECT_DM_PATH_OFFSET), dm_path, path_len);
    ydm_common_encode(dms->txbuf, sizeof(dms->txbuf), DM_EVENT_CONNECT_DIAG_MASTER_ACCEPT, recode, udscid, IPC_USE_USER_TIME_STAMP);
    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    memcpy(un.sun_path, cf.event_emit_path, strlen(cf.event_emit_path));    
    sendto(dms->sockfd, dms->txbuf, DMREQ_CONNECT_DM_MSG_MIN_SIZE + path_len, 0,\
                        (struct sockaddr *)&un, sizeof(struct sockaddr_un));
    Y_UNUSED(tidp);
    
    return ;    
}

static void dm_logfile_limit_cycle_period_refresh(DMS *dms)
{
    ev_timer_stop(dms->loop, &dms->logfile_limit_watcher);
    ev_timer_init(&dms->logfile_limit_watcher, dm_ev_timer_logfile_limit_handler, \
        dms->logfile_cycle_period, dms->logfile_cycle_period);
    ev_timer_start(dms->loop, &dms->logfile_limit_watcher);
}

static void dm_ev_timer_logfile_limit_handler(struct ev_loop *loop, ev_timer *w, int revents)
{
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    yuint32 dir_file_size = 0;
    char old_file_path[256] = {0};
    yuint32 old_file_mtime = time(NULL);
    DMS *dms = ev_userdata(loop);

    if ((dir = opendir(DM_LOG_SAVE_DIR_DEF)) == NULL) {
        log_e("Unable to open directory => %s", DM_LOG_SAVE_DIR_DEF);
        return ;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        char path[256] = {0};
        struct stat fileStat;

        if ((strlen(entry->d_name) == 1 && strcmp(".", entry->d_name) == 0) || \
            (strlen(entry->d_name) == 2 && strcmp("..", entry->d_name) == 0)) {
            continue;
        }

        if (strstr(entry->d_name, DM_LOG_FILE_PREFIX) == NULL || \
            strstr(entry->d_name, DM_VERSION"_"DM_LOG_CONF_FILE)) {
            continue;
        }
            
        sprintf(path, "%s/%s", DM_LOG_SAVE_DIR_DEF, entry->d_name);
        if (lstat(path, &fileStat) == -1) {
            log_e("Failed to obtain file status information => %s", path);
            continue;
        }
        // �ж��Ƿ�Ϊ�����ļ��ҷǿհ��ַ���
        if (S_ISREG(fileStat.st_mode) && !isspace(entry->d_name[0])) {            
            /* Ŀ¼����־�ļ���Сͳ�� */
            dir_file_size += fileStat.st_size;
            if (fileStat.st_mtime < old_file_mtime) {
                /* �������ϵ���־�ļ���Ϣ */
                memset(old_file_path, 0, sizeof(old_file_path));
                snprintf(old_file_path, sizeof(old_file_path), "%s", path);
                old_file_mtime = fileStat.st_mtime;
            }
        }
    }

    /* ����ļ���С��200M��500M֮�䣬��־��õ���־����12���¾�ɾ�� */
    if (dir_file_size > (200/* M */ * 1024/* K */ * 1024/* byte */) && \
        dir_file_size < (500/* M */ * 1024/* K */ * 1024/* byte */)) {
        int def_time = (time(NULL) - old_file_mtime);
        if (def_time > (60/* sec */ * 60/* min */ * 24/* hour */ * 365/* day */)) {
            if (access(old_file_path, F_OK) == 0) {
                log_i("Size of the current dir log file => %f M", (float)dir_file_size / (float)(1024 * 1024));
                log_i("Delete log files created %d seconds ago => %s", def_time, old_file_path);
                remove(old_file_path); 
                dms->logfile_cycle_period = 5.0;
            }
        }
        else {
            dms->logfile_cycle_period = 60.0;
        }
    }
    else if (dir_file_size > 500 * 1024 * 1024) {        
        if (access(old_file_path, F_OK) == 0) {
            log_i("Size of the current dir log file => %f M", (float)dir_file_size / (float)(1024 * 1024));
            log_i("Delete log file => %s", old_file_path);                
            remove(old_file_path);                
            dms->logfile_cycle_period = 5.0;
        }
    }
    else {
        dms->logfile_cycle_period = 60.0;
    }
    dm_logfile_limit_cycle_period_refresh(dms);

    closedir(dir);
}

void dm_oms_destroy(DMS *dms)
{
    int index = 0;

    if (dms == NULL) {
        return ;
    }

    /* ֹͣ�����¼�ѭ�� */
    ev_timer_stop(dms->loop, &dms->logfile_limit_watcher);
    ev_io_stop(dms->loop, &dms->iwrite_watcher);
    ev_io_stop(dms->loop, &dms->iread_watcher);

    for (index = 0; index < DMS_DIAG_MASTER_NUM_MAX; index++) {
        if (dms->dm[index]) {
            dm_ev_stop(dms->dm[index]);         
        }
    }

    if (dms->sockfd > 0) {
        close(dms->sockfd);
    }
    
    unlink(OMS_UNIX_SOCKET_PATH);
}

DMS *dm_oms_create()
{
    int ret = 0;
    struct sockaddr_un un;

    DMS *dms = ymalloc(sizeof(*dms));
    if (dms == NULL) {
        return NULL;
    }
    memset(dms, 0, sizeof(*dms));
    dms->sockfd = -1;
    memset(&un, 0, sizeof(struct sockaddr_un));
    un.sun_family = AF_UNIX;
    memcpy(un.sun_path, OMS_UNIX_SOCKET_PATH, sizeof(OMS_UNIX_SOCKET_PATH));
    dms->sockfd = ydm_ipc_channel_create(&un, sizeof(dms->rxbuf), sizeof(dms->txbuf));
    if (dms->sockfd < 0) {
        log_d("fatal error yom_ipc_channel_create failed");
        goto CREAT_FAILED;
    }
    dms->loop = ev_loop_new(0);;
    /* �����û�����ָ��, �����ڸ����ص�������ʹ�� */
    ev_set_userdata(dms->loop, dms);

    /* ����IPC���¼� */
    ev_io_init(&dms->iread_watcher, dm_ev_io_oms_read, dms->sockfd, EV_READ);
    ev_io_start(dms->loop, &dms->iread_watcher);
    
    /* �����־�ļ���С */
    ev_timer_init(&dms->logfile_limit_watcher, dm_ev_timer_logfile_limit_handler, 60, 60);
    ev_timer_start(dms->loop, &dms->logfile_limit_watcher);
    return dms;
CREAT_FAILED:
    log_d("diag master create error");
    if (dms->sockfd > 0) {
        close(dms->sockfd);
    }
    unlink(OMS_UNIX_SOCKET_PATH);
    yfree(dms);
    Y_UNUSED(ret);

    return NULL;
}

void *dm_oms_thread_run(void *arg)
{
    DMS *dms = arg;
    if (dms == NULL) {
        log_d("Parameter error Thread exits");
        return 0;
    }
    pthread_detach(pthread_self());
    /* ��ʼ�¼�ѭ��������ʱ�̶��ô����¼������û���¼��������߳��˳� */
    log_d("diag master oms event loop start.");

    ev_run(dms->loop, 0);
    /* ����������¼�ѭ���ǲ����˳��ģ��˳���˵���������� */
    log_e("diag master oms event loop exit thread exits.");    
    /* �ͷ��ڴ� */
    ev_loop_destroy(dms->loop);
    yfree(dms);
    __g_oms__ = NULL;
    
    return 0;
}

int dm_oms_start(void)
{
    pthread_t tidp = 0;

    /* OMS�ṹ�����ڹ�������diag master api */
    DMS *dms = dm_oms_create();
    if (dms == NULL) {
        log_d("diag master oms create failed.");
        return -1;
    }
    /* �����߳������¼�ѭ�� */
    if (pthread_create(&tidp, 0, dm_oms_thread_run, dms) == 0) {
        log_d(" diag master oms thread create success.");
    }
    else {
        goto OMS_START_FAILE;
    }
    /* ȫ�ֱ���һ��,�ͷ�ʱ��Ҫʹ�õ� */
    __g_oms__ = dms;

    return 0;
OMS_START_FAILE:
    log_d("diag master oms thread start faile");
    if (dms->sockfd > 0) {
        close(dms->sockfd);
    }
    unlink(OMS_UNIX_SOCKET_PATH);
    yfree(dms);
    __g_oms__ = NULL;
    
    return -2;
}

struct om_runtime_config_s {
    boolean parent_active;
    boolean is_debug_enable;
    pid_t child_pid;
    int process_lock_fd;
    char log_config_file[256];
    char log_save_dir[256];
    int sync_time_wait;
} rt_config = {false, false, 0, -1, {""}, {DM_LOG_SAVE_DIR_DEF}, 120};

#define HEX_TO_CHAR(h) (((h) >= 10) ? ((h) - 10) + 'A' : (h) + '0')

static void dm_uds_asc_record(char *direction, yuint8 *msg, yuint32 mlen)
{
    static char str[4096] = {0};
    yuint32 str_index = 0;
    yuint32 msg_index = 0;
    static yuint32 msg_counter = 0;

    msg_counter++;
    /* 36������ļ����ݲ��ֲ��������־�ˣ�̫����־�˼�һ�� */
    if (msg[0] == 0x36) {
        str[str_index++] = HEX_TO_CHAR(((msg[0] >> 4) & 0x0f));
        str[str_index++] = HEX_TO_CHAR(((msg[0]) & 0x0f));
        str[str_index++] = ' ';
        str[str_index++] = HEX_TO_CHAR(((msg[1] >> 4) & 0x0f));
        str[str_index++] = HEX_TO_CHAR(((msg[1]) & 0x0f));
        str[str_index++] = ' ';
        str[str_index++] = '.';
        str[str_index++] = '.';
        str[str_index++] = '.';         
        str[str_index++] = ' ';
        str[str_index++] = HEX_TO_CHAR(((msg[mlen -1] >> 4) & 0x0f));
        str[str_index++] = HEX_TO_CHAR(((msg[mlen -1]) & 0x0f));
        str[str_index++] = '\0';
#ifdef __HAVE_ZLOG__
        dzlog_notice("[%d][%s][%d/%d]%s", msg_counter, direction, mlen, mlen, str);
#endif /* __HAVE_ZLOG__ */    
    }
    else {
        for ( ; msg_index < mlen; msg_index++) {
            str[str_index++] = HEX_TO_CHAR((msg[msg_index] >> 4) & 0x0f);
            str[str_index++] = HEX_TO_CHAR((msg[msg_index]) & 0x0f);
            str[str_index++] = ' ';
            if (sizeof(str) - str_index < 3) {
                str[str_index - 1] = '\0';            
#ifdef __HAVE_ZLOG__
                dzlog_notice("[%d][%s][%d/%d]%s", msg_counter, direction, mlen, mlen, str);
#endif /* __HAVE_ZLOG__ */    
                str_index = 0;
            }
        }    
        str[str_index - 1] = '\0';
#ifdef __HAVE_ZLOG__
        dzlog_notice("[%d][%s][%d/%d]%s", msg_counter, direction, mlen, mlen, str);
#endif /* __HAVE_ZLOG__ */    
    }

    return ;
}

void dm_log_conf_init()
{
    FILE *lfd = NULL;
    char *def_conf = "[global] \n"
                     "strict init = true \n"
                     "buffer min = 1024 \n"
                     "buffer max = 1MB \n"
                     "rotate lock file = /tmp/zlog.lock \n"
                     "default format = \"default - %d(%F %X.%ms) %-6V (%c:%F:%U:%L) - %m%n\" \n"
                     "[levels] \n"
                     "show = 30, LOG_DEBUG \n"
                     "[formats] \n"
                     "null = \"%n\" \n"
                     "print = \"print - [%-10.3d(%F)]%n\" \n"
                     "simple = \"[%d(%Y/%m/%d %T).%ms][%V][%f:%L][%M("DM_DIAG_MASTER_MDC_KEY")/%M("DM_UDSC_MDC_KEY")] %m%n\" \n"
                     "detail = \"[%d(%Y/%m/%d %T).%ms][%V][%f:%L][%M("DM_DIAG_MASTER_MDC_KEY")/%M("DM_UDSC_MDC_KEY")][PID:%p: TID:%t] %m%n\" \n"                     
                     "asc = \"[%d(%T).%ms][><][%M("DM_DIAG_MASTER_MDC_KEY")/%M("DM_UDSC_MDC_KEY")]%m%n\" \n"
                     "showformat = \"%m%n\" \n"
                     "[rules]\n"
                     "dmcat.=info \""DM_LOG_SAVE_DIR_DEF""DM_LOG_FILE_FORMAT"\", 10M * 100 ~ \""DM_LOG_SAVE_DIR_DEF""DM_LOG_FILE_FORMAT".#r\"; simple \n"
                     "dmcat.=warn \""DM_LOG_SAVE_DIR_DEF""DM_LOG_FILE_FORMAT"\", 10M * 100 ~ \""DM_LOG_SAVE_DIR_DEF""DM_LOG_FILE_FORMAT".#r\"; detail \n"
                     "dmcat.=error \""DM_LOG_SAVE_DIR_DEF""DM_LOG_FILE_FORMAT"\", 10M * 100 ~ \""DM_LOG_SAVE_DIR_DEF""DM_LOG_FILE_FORMAT".#r\"; detail \n"
                     "dmcat.=fatal \""DM_LOG_SAVE_DIR_DEF""DM_LOG_FILE_FORMAT"\", 10M * 100 ~ \""DM_LOG_SAVE_DIR_DEF""DM_LOG_FILE_FORMAT".#r\"; detail \n"    
                     "dmcat.=notice \""DM_LOG_SAVE_DIR_DEF""DM_LOG_FILE_FORMAT"\", 10M * 100 ~ \""DM_LOG_SAVE_DIR_DEF""DM_LOG_FILE_FORMAT".#r\"; asc \n"
                     "dmcat.=show \""DM_LOG_SAVE_DIR_DEF""DM_LOG_FILE_FORMAT"\", 10M * 100 ~ \""DM_LOG_SAVE_DIR_DEF""DM_LOG_FILE_FORMAT".#r\"; showformat \n";
    /* �Ƿ������־�����ļ� */
    if (access(rt_config.log_config_file, F_OK) == 0) {
        /* ���ھ����� */
        return ;
    }
    /* ������ʹ��Ĭ�ϵ���־���� */
    lfd = fopen(rt_config.log_config_file, "w+");
    if (!lfd) {
        return ;
    }
    fwrite(def_conf, 1, strlen(def_conf), lfd);
    fclose(lfd);

    return ;
}

int dm_log_init()
{
    int rc;

    /* ��־�ļ��洢Ŀ¼��û�оʹ��� */
    if (access(rt_config.log_save_dir, F_OK) != 0) {
        mkdir(rt_config.log_save_dir, 0777);
    }
    
    /* ��־�����ļ� */
    dm_log_conf_init();
    
#ifdef __HAVE_ZLOG__
    /*  */
    rc = dzlog_init(rt_config.log_config_file, "dmcat");
    if (rc) {
        printf("init failed\n");
        return -1;
    }

    /* ����zlog�Ե��� */
    putenv("export ZLOG_PROFILE_ERROR="DM_LOG_SAVE_DIR_DEF"zlog.error.log"); /* ���������Ϣ */
    // putenv("export ZLOG_PROFILE_DEBUG="DM_LOG_SAVE_DIR_DEF"zlog.debug.log"); /* ���������Ϣ */
#endif /* __HAVE_ZLOG__ */

    return 0;
}

void dm_log_conf_init_debug()
{
    FILE *lfd = NULL;
    char *def_conf = "[global] \n"
                     "strict init = true \n"
                     "buffer min = 1024 \n"
                     "buffer max = 1MB \n"
                     "rotate lock file = /tmp/zlog.lock \n"
                     "default format = \"default - %d(%F %X.%ms) %-6V (%c:%F:%U:%L) - %m%n\" \n"                     
                     "[levels] \n"
                     "show = 30, LOG_DEBUG \n"
                     "[formats] \n"
                     "null = \"%n\" \n"
                     "print = \"print - [%-10.3d(%F)]%n\" \n"
                     "simple = \"[%d(%Y/%m/%d %T).%ms][%V][%f:%L][%M("DM_DIAG_MASTER_MDC_KEY")/%M("DM_UDSC_MDC_KEY")] %m%n\" \n"
                     "detail = \"[%d(%Y/%m/%d %T).%ms][%V][%f:%L][%M("DM_DIAG_MASTER_MDC_KEY")/%M("DM_UDSC_MDC_KEY")][PID:%p: TID:%t] %m%n\" \n"                     
                     "asc = \"[%d(%T).%ms][><][%M("DM_DIAG_MASTER_MDC_KEY")/%M("DM_UDSC_MDC_KEY")]%m%n\" \n"                     
                     "showformat = \"%m%n\" \n"
                     "[rules]\n"                     
                     "dmcat.=debug >stdout;simple \n"
                     "dmcat.=info >stdout;simple \n"
                     "dmcat.=warn >stdout;detail \n"
                     "dmcat.=error >stdout;detail \n"
                     "dmcat.=fatal >stdout;detail \n"
                     "dmcat.=notice >stdout;asc \n"
                     "dmcat.=show >stdout;showformat \n";

    /* �Ƿ������־�����ļ� */
    if (access(rt_config.log_config_file, F_OK) == 0) {
        /* ���ھ����� */
        return ;
    }
    /* ������ʹ��Ĭ�ϵ���־���� */
    lfd = fopen(rt_config.log_config_file, "w+");
    if (!lfd) {
        return ;
    }
    fwrite(def_conf, 1, strlen(def_conf), lfd);
    fclose(lfd);

    return ;
}

int dm_log_init_debug()
{
    int rc;

    /* ��־�ļ��洢Ŀ¼��û�оʹ��� */
    if (access(rt_config.log_save_dir, F_OK) != 0) {
        mkdir(rt_config.log_save_dir, 0777);
    }
    
    /* ��־�����ļ� */
    dm_log_conf_init_debug();
    
#ifdef __HAVE_ZLOG__
    /*  */
    rc = dzlog_init(rt_config.log_config_file, "dmcat");
    if (rc) {
        printf("init failed\n");
        return -1;
    }

    /* ����zlog�Ե��� */
    putenv("export ZLOG_PROFILE_ERROR="DM_LOG_SAVE_DIR_DEF"zlog.error.log"); /* ���������Ϣ */
    putenv("export ZLOG_PROFILE_DEBUG="DM_LOG_SAVE_DIR_DEF"zlog.debug.log"); /* ���������Ϣ */
#endif /* __HAVE_ZLOG__ */

    return 0;
}

void dm_process_info()
{
    char *logo =" ";

    log_i("%s \n""Version: \t%s \n""Compile time: \t%s %s \n", logo, DM_VERSION, __DATE__, __TIME__);
}

void dm_init(int attr)
{
    /* ��־ϵͳ��ʼ�� */
    if (rt_config.is_debug_enable) {
        dm_log_init_debug();
    }
    else {
        dm_log_init();
    }

    /* ������Ϣ */
    dm_process_info();

    /* �������¼�ѭ���߳� */
    dm_oms_start();
}

#define DM_REFERENCE_TIME_STAMP (1704038400) /* 2024-01-01 00:00:00 */

void *dm_wait_time_sync_run(void *arg)
{
    struct timeval timeout;
    int time_total = rt_config.sync_time_wait; /* 2min���ǽ���������� */

    /* �ȴ�ϵͳ���Уʱ */
    while (time(NULL) < DM_REFERENCE_TIME_STAMP && \
           time_total > 0) {
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;        
        time_total -= timeout.tv_sec + (timeout.tv_usec / 1000000);
        select(0, 0, 0, 0, &timeout); 
    }
    dm_init((int)arg);

    return 0;
}

void dm_wait_time_sync(int attr)
{
    pthread_t tidp = 0;

    pthread_create(&tidp, 0, dm_wait_time_sync_run, attr);
}

void dm_runtime_config_init()
{
    if (strlen(rt_config.log_config_file) == 0 || \
        access(rt_config.log_config_file, F_OK) != 0) {
        if (rt_config.is_debug_enable) {
            snprintf(rt_config.log_config_file, sizeof(rt_config.log_config_file), \
                "%s/%s_debug_%s", rt_config.log_save_dir, DM_VERSION, DM_LOG_CONF_FILE);
        }
        else {
            snprintf(rt_config.log_config_file, sizeof(rt_config.log_config_file), \
                "%s/%s_%s", rt_config.log_save_dir, DM_VERSION, DM_LOG_CONF_FILE);
        }
    }
}

int dm_start(int attr)
{
    if (__g_oms__ != NULL) {
        /* �Ѿ����������ظ����� */
        return 0;
    }

    /* ����ʱ���ó�ʼ�� */
    dm_runtime_config_init();

    if (DM_ATTR_ISSET(attr, DM_START_WAIT_SYSTEM_TIME_SYNC_ATTR) && \
        time(NULL) < DM_REFERENCE_TIME_STAMP) {
        /* �ȴ�ϵͳ���Уʱ */
        printf("diag master waiting system time sync... \n");
        dm_wait_time_sync(attr);
    }
    else {
        dm_init(attr);
    }

    return 0;
}

/* �ӽ��̰�ȫ�˳� */
void child_process_graceful_exit(int sig) 
{
    printf("child process graceful exit \n");
    dm_oms_destroy(__g_oms__);

    exit(0);
}

/* �����̰�ȫ�˳� */
void parent_process_graceful_exit(int sig) 
{
    int status = 0;
    pid_t pid = getpid();

    printf("diag master shutting down \n");
    printf("diag master kill(-%d, %d) \n", pid, sig);
    if (rt_config.process_lock_fd > 0) { 
        close(rt_config.process_lock_fd);
        rt_config.process_lock_fd = -1;
    }
    kill(-pid, sig);
    printf("diag master waiting for child process %d exit \n", rt_config.child_pid);
    waitpid(rt_config.child_pid, &status, 0);

    exit(0);
}

void signals_ignore(int sig) 
{
    printf("diag master ignore signal %d \n", sig);
}

/* �ӽ�����ʱ����ʱ�� */
#define DM_CHILD_PROCESS_RESTART_DELAY (5) 

int dm_self_monitor_start(int attr)
{
    int status = 0;

    int lock_result = 0;
    char *lck_file = "/tmp/om_process.lock";

    /* ��ֹ���̱�����ɵ� */
    signal(SIGRTMAX - 14, signals_ignore);

    /* ��ֹ������� */
    rt_config.process_lock_fd = open(lck_file, O_RDWR | O_CREAT);
    if (rt_config.process_lock_fd < 0) {
        printf("Open file failed => %s \n", lck_file);
        return -1;
    }
    
    lock_result = lockf(rt_config.process_lock_fd, F_TEST, 0);  
    if (lock_result < 0) {
        printf("The process failed to start already started \n");
        return -2;
    }
    
    lock_result = lockf(rt_config.process_lock_fd, F_TLOCK, 0);
    if (lock_result < 0) {
        printf("The process failed to start already started \n");
        return -3;
    }

    /* ������ע����Щ�źţ��ڸ������˳�ʱ�����ӽ��� */
    signal(SIGTERM, parent_process_graceful_exit);
    signal(SIGQUIT, parent_process_graceful_exit);    
    /* �����������̣��������ָ��ӽ��� */
    prctl(PR_SET_NAME, "dm-monitor", NULL, NULL, NULL);
    
    rt_config.parent_active = true; /* ������ѭ����ʼ */
    do {        
        rt_config.child_pid = fork();
        switch (rt_config.child_pid) {
            case -1:            
                perror("fork()");
                exit(1);
            case 0:
                signal(SIGTERM, child_process_graceful_exit);
                signal(SIGQUIT, child_process_graceful_exit);
                if (rt_config.process_lock_fd > 0) { 
                    close(rt_config.process_lock_fd);
                    rt_config.process_lock_fd = -1;
                }
                /* �������ӽ��̣��������ָ��ӽ��� */
                prctl(PR_SET_NAME, "dm-working", NULL, NULL, NULL);
                dm_start(attr); /* ��ҵ������ */
                while (1) { sleep(1); } /* ��ֹ�����˳� */
            default:
                // wait for exit
                printf("diag master monitoring the child process pid %d \n", rt_config.child_pid);
                waitpid(rt_config.child_pid, &status, 0);
                printf("diag master process %d exit status %d \n", rt_config.child_pid, status);
        }        
        sleep(DM_CHILD_PROCESS_RESTART_DELAY); /* ��ʱ������fork�ӽ��� */
    }while (rt_config.parent_active);

    return 0;
}

void ydiag_master_ver_info()
{
    printf("Version: \t%s \n""Compile time: \t%s %s \n", DM_VERSION, __DATE__, __TIME__);
}

void ydiag_master_debug_enable(int b)
{
    rt_config.is_debug_enable = b;
}

void ydiag_master_log_config_file_set(const char *file)
{
    if (!file) {
        return ;
    }
    snprintf(rt_config.log_config_file, sizeof(rt_config.log_config_file), "%s", file);
}

void ydiag_master_sync_time_wait_set(int s)
{
    rt_config.sync_time_wait = s > 0 ? s : 120;
}

void ydiag_master_log_save_dir_set(const char *dir)
{
    if (!dir) {
        return ;
    }
    snprintf(rt_config.log_save_dir, sizeof(rt_config.log_save_dir), "%s", dir);
}

int ydiag_master_start(int attr)
{
    /* fork��ҵ���߼� */
    if (DM_ATTR_ISSET(attr, DM_SELF_MONITOR_ATTR)) {
        return dm_self_monitor_start(attr);
    }
    else {
        return dm_start(attr);
    }

    return 0;
}

int ydiag_master_stop()
{
    rt_config.parent_active = false;
    dm_oms_destroy(__g_oms__);
#ifdef __HAVE_ZLOG__
    zlog_fini();
#endif /* __HAVE_ZLOG__ */
    return 0;
}

