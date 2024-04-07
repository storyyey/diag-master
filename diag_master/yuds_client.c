#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#include "config.h"
#include "ev.h"
#include "ydm_types.h"
#include "ydm_log.h"
#include "ydm_stream.h"
#include "yudsc_types.h"
#include "ydm_ipc_common.h"       
#include "yuds_client.h"
#ifdef __HAVE_UDS_PROTOCOL_ANALYSIS__        
#include "yuds_proto_parse.h"
#endif /* __HAVE_UDS_PROTOCOL_ANALYSIS__ */    

static void udsc_td_36_progress_notify_callback(struct ev_loop *loop, ev_timer *w, int revents);
static int udsc_transfer_average_speed(yuint32 *preTime, int *totalByte, int *averageByte);
static int udsc_service_36_td_config_init(uds_client *udsc);

/* ��ȡ��Ӧ��Ϣ�е�SID */
static yuint8 udsc_service_response_sid(yuint8 *rmsg, yuint32 size)
{
    if (rmsg && size > 0) {
        if (rmsg[0] == NEGATIVE_RESPONSE_ID) {
            if (size >= 2) {
                return rmsg[1] & (~UDS_REPLY_MASK);
            }
            else {
                return 0;
            }
        }
        else {
            return rmsg[0] & (~UDS_REPLY_MASK);
        }
    }
    else {
        return 0;
    }
}

/* ��ȡ��Ӧ��Ϣ�еĸ���Ӧ�� */
static yuint8 udsc_service_response_nrc(yuint8 *rmsg, yuint32 size)
{
    if (rmsg && size >= NEGATIVE_RESPONSE_FIX_SIZE &&\
        rmsg[0] == NEGATIVE_RESPONSE_ID) {
        return rmsg[2]; 
    }
    else {
        return 0;
    }
}

/* �Ƿ���������Ӧ */
static BOOLEAN udsc_service_negative_response(yuint8 *rmsg, yuint32 size)
{
    if (rmsg && size > 0) {
        if (rmsg[0] == NEGATIVE_RESPONSE_ID) {
            return true;
        }
        else {
            return false;
        }
    }
    else {
        return false;
    }
}

/* ��ȡ��ǰ��uds�ͻ�����������ͳ�� */
static void udsc_run_time_total_statis(uds_client *udsc)
{
    if (udsc->runtime_statis_enable) {
        unsigned int tv_sec = 0;
        unsigned int tv_usec = 0;
        ydm_tv_get(&tv_sec, &tv_usec);
        udsc->rt_data_statis.run_time_total = tv_sec * 1000 + tv_usec / 1000 - udsc->run_start_time;
    }     
}

/* ���õ�ǰ��Ծ����Ϸ�������Ч */
static void udsc_service_item_invalid_set(uds_client *udsc)
{
    udsc->sindex = -1;
}

/* ��鵱ǰ��Ծ����Ϸ������Ƿ���Ч */
static BOOLEAN udsc_service_item_index_valid(uds_client *udsc, yint32 index)
{
    if (index >= 0 && \
        index < udsc->service_cnt && \
        index < udsc->service_size) {
        return true;
    }

    return false;
}

static BOOLEAN udsc_service_have_sub(yuint8 sid)
{
    if (sid == UDS_SERVICES_DSC || \
        sid == UDS_SERVICES_ER || \
        sid == UDS_SERVICES_SA || \
        sid == UDS_SERVICES_CC || \
        sid == UDS_SERVICES_CDTCS || \
        sid == UDS_SERVICES_ROE || \
        sid == UDS_SERVICES_LC || \
        sid == UDS_SERVICES_RDTCI || \
        sid == UDS_SERVICES_DDDI || \
        sid == UDS_SERVICES_RC) {
        return true;
    }

    return false;
}

/* �Ƿ���ʧ�ܼ���ֹ */
static BOOLEAN udsc_is_fail_abort(uds_client *udsc)
{
    return !!udsc->isFailAbort;
}

/* ��Ӧ��Ԥ��һ�� */
static void udsc_service_response_expect(uds_client *udsc)
{
    udsc->response_meet_expect = true;
}

/* ��Ӧ��Ԥ�ڲ�һ�� */
static void udsc_service_response_unexpect(uds_client *udsc)
{
    udsc->response_meet_expect = false;
}

static BOOLEAN udsc_service_response_meet_expect(uds_client *udsc)
{
    return !!udsc->response_meet_expect;
}

static BOOLEAN udsc_service_isactive(uds_client *udsc)
{
    return !!udsc_service_item_index_valid(udsc, udsc->sindex);
}

/*  */
static service_item *udsc_active_service_item(uds_client *udsc)
{
    /* ���ﲻӦ�÷��ؿ�ָ�룬���ؿ�ָ���ǲ������ */
    service_item *item = udsc->service_items[udsc->sindex];
    assert(item);
    return item;
}

/* ��ѯ����һ����Ϸ����� */
static void udsc_next_service_item(uds_client *udsc)
{
    if (udsc_service_isactive(udsc)) {
        udsc->sindex++;
    }
}

static service_item *udsc_service(uds_client *udsc, int index)
{
    if (udsc_service_item_index_valid(udsc, index)) {
        return udsc->service_items[index];
    }

    return NULL;
}

static int udsc_surplus_service_find(uds_client *udsc, yuint8 sid)
{
    int ii = 0;

    for (ii = udsc->sindex + 1; udsc_service_item_index_valid(udsc, ii); ii++) {
        if (sid == udsc->service_items[ii]->sid) {
            return ii;
        }
    }

    return -1;
}

/* ����ʣ��δִ�еķ������Ƿ���ָ������ */
static BOOLEAN udsc_surplus_service_container(uds_client *udsc, yuint8 sid)
{
    if (udsc_surplus_service_find(udsc, sid) > 0) {
        return true;
    }

    return false;
}

/* ֹͣUDS�ͻ��˵����� */
static BOOLEAN  udsc_services_stop(uds_client *udsc)
{
    /* ��ǵ���Ч����Ϸ������� */
    udsc_service_item_invalid_set(udsc);    

    ev_timer_stop(udsc->loop, &udsc->testerPresent_watcher);
    ev_timer_stop(udsc->loop, &udsc->request_watcher);
    ev_timer_stop(udsc->loop, &udsc->response_watcher);
    ev_timer_stop(udsc->loop, &udsc->td_36_progress_watcher);
    log_i("UDS client(%d) stop runing", udsc->id);

    return true;
}

static void udsc_service_tester_present_refresh(uds_client *udsc)
{
    ev_timer_stop(udsc->loop, &udsc->testerPresent_watcher);
    ev_timer_set(&udsc->testerPresent_watcher, udsc->tpInterval * 0.001, udsc->tpInterval * 0.001);
    ev_timer_start(udsc->loop, &udsc->testerPresent_watcher);
}

static void udsc_service_tester_present_timer_start(uds_client *udsc)
{
    ev_timer_stop(udsc->loop, &udsc->testerPresent_watcher);
    ev_timer_set(&udsc->testerPresent_watcher, udsc->tpInterval * 0.001, udsc->tpInterval * 0.001);
    ev_timer_start(udsc->loop, &udsc->testerPresent_watcher);
}

static void udsc_service_request_timer_now_trigger(uds_client *udsc)
{
    ev_timer_stop(udsc->loop, &udsc->request_watcher);
    ev_timer_set(&udsc->request_watcher, 0, 0);
    ev_timer_start(udsc->loop, &udsc->request_watcher);
}

static void udsc_service_request_delay_trigger(uds_client *udsc)
{
    service_item *item = udsc_active_service_item(udsc);  

    item->status = SERVICE_ITEM_RUN;
    log_d("UDSC Service %s Pre-request delay %d ms start", item->desc ? item->desc : " ", item->delay);
    ev_timer_stop(udsc->loop, &udsc->request_watcher);
    ev_timer_set(&udsc->request_watcher, item->delay * 0.001, item->delay * 0.001);
    ev_timer_start(udsc->loop, &udsc->request_watcher);
}

static void udsc_service_response_wait_start(uds_client *udsc)
{
    yint32 timeout = 0;
    service_item *item = udsc_active_service_item(udsc);
    
    if (item->timeout == 0) {
        timeout = udsc->common_var.p2_server_max > 0 ? udsc->common_var.p2_server_max : item->timeout;
    }
    else {
        timeout = item->timeout;
    }    
    log_d("UDSC Service %s Response timeout %d ms start", item->desc ? item->desc : " ", timeout);
    ev_timer_stop(udsc->loop, &udsc->response_watcher);
    ev_timer_set(&udsc->response_watcher, timeout * 0.001, timeout * 0.001);
    ev_timer_start(udsc->loop, &udsc->response_watcher);
}

static void udsc_service_response_wait_stop(uds_client *udsc)
{
    yint32 timeout = 0;
    service_item *item = udsc_active_service_item(udsc);

    timeout = udsc->common_var.p2_server_max > 0 ? udsc->common_var.p2_server_max : item->timeout;
    log_d("UDSC Service %s Response timeout %d ms stop", item->desc ? item->desc : " ", timeout);
    ev_timer_stop(udsc->loop, &udsc->response_watcher);
}

static void udsc_service_response_wait_refresh(uds_client *udsc)
{
    yint32 timeout = 0;
    service_item *item = udsc_active_service_item(udsc);

    /* +100 ��Ԥ����һ�룬��ֹ������̼��ʱ������ */
    timeout = udsc->common_var.p2_e_server_max > 0 ? (udsc->common_var.p2_e_server_max + 100)  : item->timeout;
    timeout *= 10;
    log_d("UDSC Service %s Response timeout %d ms refresh", item->desc ? item->desc : " ", timeout);
    ev_timer_stop(udsc->loop, &udsc->response_watcher);
    ev_timer_set(&udsc->response_watcher, timeout * 0.001, timeout * 0.001);
    ev_timer_start(udsc->loop, &udsc->response_watcher);
}

/* �Ƿ񱣳�ִ�е�ǰ���� */
static BOOLEAN udsc_service_is_keep(uds_client *udsc)
{
    return udsc->iskeep;
}

static void udsc_curr_service_next(uds_client *udsc)
{
    /* �л�����һ�������� */
    udsc->iskeep = false;
}

static void udsc_curr_service_keep(uds_client *udsc)
{
    /* ���ֵ�ǰ��Ϸ������ִ�� */
    udsc->iskeep = true;
}

static void udsc_td_36_progress_notify(uds_client *udsc)
{
    unsigned int tv_sec = 0, tv_usec = 0;

    if (udsc->service_36_transfer_progress_cb) {        
        ydm_tv_get(&tv_sec, &tv_usec);
        udsc->service_36_transfer_progress_cb(udsc->service_36_transfer_progress_arg, udsc->id, udsc->common_var.fileSize, \
            udsc->common_var.fileTransferTotalSize, \
            (tv_sec * 1000 + tv_usec / 1000) - udsc->td_36_start_time);
    }
}

static void udsc_service_request_preprocessor(uds_client *udsc)
{
    unsigned int tv_sec = 0, tv_usec = 0; 
    size_t rsize = 0;
    service_item *item = udsc_active_service_item(udsc);
    
    if (item->sid == UDS_SERVICES_SA) {
        if (item->sub % 2 == 0) {            
            YByteArrayClear(item->request_byte);
            YByteArrayAppendChar(item->request_byte, UDS_SERVICES_SA);        
            YByteArrayAppendChar(item->request_byte, item->sub);
            YByteArrayAppendArray(item->request_byte, udsc->common_var.key_byte);
        }
    }
    else if (item->sid == UDS_SERVICES_TD) {
        /* ��ʼ��36����������Ҫ������ */
        udsc_service_36_td_config_init(udsc);

        /* �ж��Ƿ����ش����� */
        if (item->td_36.is_retrans) {
            item->td_36.is_retrans = false;
            /* ���ش���36���ģ������ļ���ȡ�µ��� */
            return ;
        }
        
        if (udsc->common_var.ff_cache_buff) {
            /* �ļ��Ѿ���ȫ�������ڴ��У�ֻ��Ҫ���ڴ���ȡ���� */
            if ((udsc->common_var.fileSize - \
                udsc->common_var.fileTransferTotalSize) > \
                (udsc->common_var.maxNumberOfBlockLength - 2)) {
                rsize = udsc->common_var.maxNumberOfBlockLength - 2;
            }
            else {
                rsize = udsc->common_var.fileSize - udsc->common_var.fileTransferTotalSize;
            }            
            udsc->common_var.td_buff = udsc->common_var.ff_cache_buff + \
                udsc->common_var.fileTransferTotalSize;
        }
        else {
            /* ��Ҫ���ļ������ζ�ȡ�ļ����� */
            if (udsc->common_var.td_buff && udsc->common_var.filefd) {
                rsize = fread(udsc->common_var.td_buff, 1, \
                    udsc->common_var.maxNumberOfBlockLength - 2, udsc->common_var.filefd);
                if (rsize != udsc->common_var.maxNumberOfBlockLength - 2) {
                    /* �ļ�����֮��ʱ�ر��ļ� */
                    fclose(udsc->common_var.filefd);
                    udsc->common_var.filefd = NULL;
                }
            }
        }

        if (rsize > 0) {
            /* ���ļ��ڻ�ȡ���ļ����ݳɹ� */
            YByteArrayClear(item->request_byte);
            YByteArrayAppendChar(item->request_byte, UDS_SERVICES_TD);
            YByteArrayAppendChar(item->request_byte, udsc->common_var.fileTransferCount);
            YByteArrayAppendNChar(item->request_byte, udsc->common_var.td_buff, rsize);
        }
        else {
            log_w("Failed to read the file. The planned read %d bytes was actually read %d bytes", \
                udsc->common_var.maxNumberOfBlockLength - 2, (int)rsize);
        }
        /* �����������ͨ�涨ʱ�� */
        if (udsc->service_36_transfer_progress_cb && udsc->td_36_progress_interval > 0 \
            && !ev_is_active(&udsc->td_36_progress_watcher)) {
            log_i("Start 36 td transfer progress notify timer");
            ev_timer_init(&udsc->td_36_progress_watcher, udsc_td_36_progress_notify_callback, \
                udsc->td_36_progress_interval * 0.001, udsc->td_36_progress_interval * 0.001);
            ev_timer_start(udsc->loop, &udsc->td_36_progress_watcher);
            ydm_tv_get(&tv_sec, &tv_usec);
            udsc->td_36_start_time = tv_sec * 1000 + tv_usec / 1000; /* ת���ɺ��� */
        }
    }
}

static void udsc_sent_data_statis(uds_client *udsc, int size)
{
    if (!udsc->runtime_statis_enable) {
        return ;
    }
    udsc->rt_data_statis.sent_num++;
    udsc->rt_data_statis.sent_total_byte += size;
    udsc->sent_total_byte += size;
    udsc_transfer_average_speed(&udsc->sent_pretime, &udsc->sent_total_byte, &udsc->sent_average_byte);
    if (udsc->sent_average_byte > 0) {
        udsc->rt_data_statis.sent_valley_speed = \
            udsc->sent_average_byte < \
            udsc->rt_data_statis.sent_valley_speed ? \
            udsc->sent_average_byte : \
            udsc->rt_data_statis.sent_valley_speed;
        udsc->rt_data_statis.sent_peak_speed = \
            udsc->sent_average_byte > \
            udsc->rt_data_statis.sent_peak_speed ? \
            udsc->sent_average_byte : \
            udsc->rt_data_statis.sent_peak_speed;
    }
}

/* ����ǰ�������� */
static void udsc_service_request_handler(uds_client *udsc)
{
    yint32 sentbyte = -1;
    BOOLEAN isfail = false;
    service_item *item = NULL;

    log_put_mdc(DM_DIAG_MASTER_MDC_KEY, udsc->om_name);
    log_put_mdc(DM_UDSC_MDC_KEY, udsc->udsc_name);
    /* û���������Ҫ����ֱ�ӷ��� */
    if (!udsc_service_isactive(udsc)) {
        log_w("The service request has been stopped");
        return ;
    }
    log_d("UDS Service request start");
    /* ˢ����������߷��Ͷ�ʱ�� */
    if (udsc->tpEnable && udsc->isTpRefresh) {
        /* ����������������Ҫ�������ﴦ�� */
        /* ����������ʱ����Ҫ���ڵķ��ͣ��������豸������ָ���Ựģʽ�� */
        udsc_service_tester_present_refresh(udsc);
    }
    
    do {
        isfail = false; /* ���ñ�־λ */
        item = udsc_active_service_item(udsc);
        /* �жϵ�ǰ������Ƿ���Ҫִ�� */
        if (item->isenable) {
            log_d("Execute the diagnostic service item %s", item->desc ? item->desc : " ");
            /* Ԥ�ȴ�������������� */
            udsc_service_request_preprocessor(udsc);
            /* �ҵ���Ҫִ�е���Ϸ������ʼִ�������Ϸ����� */   
            if (udsc->udsc_request_transfer_cb) {
                udsc_sent_data_statis(udsc, YByteArrayCount(item->request_byte));
                sentbyte = udsc->udsc_request_transfer_cb(udsc->id, udsc->udsc_request_transfer_arg, \
                    YByteArrayConstData(item->request_byte), YByteArrayCount(item->request_byte), \
                    item->sa, item->ta, item->timeout);
            }
#ifdef __HAVE_UDS_PROTOCOL_ANALYSIS__
            if (udsc->uds_msg_parse_enable) {
                yuds_protocol_parse(YByteArrayConstData(item->request_byte), YByteArrayCount(item->request_byte));
            }
#endif /* __HAVE_UDS_PROTOCOL_ANALYSIS__ */            
            /* ��Ϸ���������ʧ�� */
            if (sentbyte != YByteArrayCount(item->request_byte)) {
                log_e("Sent UDS Service request fail %d", sentbyte);
                const unsigned char *msg = YByteArrayConstData(item->request_byte);
                int msglen = YByteArrayCount(item->request_byte);
                log_hex_e("UDS Service request", msg, msglen);
                isfail = true;
            }
            if (isfail == false) {
                /* ���ͳɹ�����Ӧ��ʱ��ʱ�� */
               udsc_service_response_wait_start(udsc);
            }
            if (isfail == false || udsc_is_fail_abort(udsc)) {
                /* ��Ϸ���ִ�гɹ����߱����ִ��ʧ�ܼ��������˳�������һ������� */
                goto REQUEST_FINISH;
            }
        }
        udsc_next_service_item(udsc); /* ������һ��������Ƿ����ִ�� */
    } while (udsc_service_isactive(udsc));

REQUEST_FINISH:
    if (isfail == false) { /* �����ִ�гɹ� */
        if (!udsc_service_isactive(udsc)) {/* ���е�������Ѿ������� */
            log_i("All diagnostic service items are executed finish");
            if (udsc->udsc_task_end_cb) {
                /* �������������ִ�н��� */
                udsc_run_time_total_statis(udsc);
                udsc->udsc_task_end_cb(udsc, UDSC_NORMAL_FINISH, udsc->udsc_task_end_arg, 0, 0, 0, 0);
                yudsc_runtime_data_statis_get(udsc, 0);
            }            
            udsc_services_stop(udsc); /* ֹͣ�������������ִ�� */
        }
    }
    else {
        /* ��Ϸ���ִ��ʧ�� */  
        log_i("diagnostic service items are executed error");
        if (udsc_is_fail_abort(udsc)) {
            log_i("diagnostic service items are executed error finish");
            if (udsc->udsc_task_end_cb) {                
                item = udsc_active_service_item(udsc);
                udsc->udsc_task_end_cb(udsc, UDSC_SENT_ERROR_FINISH, udsc->udsc_task_end_arg, \
                    YByteArrayConstData(item->request_byte), YByteArrayCount(item->request_byte), 0, 0);            
                udsc_run_time_total_statis(udsc);
                yudsc_runtime_data_statis_get(udsc, 0);
            }            
            udsc_services_stop(udsc); /* ֹͣ�������������ִ�� */
        }
    }
    
    return ;
}

static void udsc_service_27_sa_response_handler(uds_client *udsc, yuint8 *data, yuint32 size)
{
    stream_t sp;
    service_item *item = udsc_active_service_item(udsc);
    yuint8 seed[SA_KEY_SEED_SIZE] = {0};    
    yuint8 key[SA_KEY_SEED_SIZE] = {0};
    yuint16 seed_size = 0;
    yuint16 key_size = sizeof(key);

    if (!udsc_service_negative_response(data, size)) {
        stream_init(&sp, data, size);
        stream_byte_read(&sp); /* ������RSID */
        yuint8 sub =stream_byte_read(&sp);
        if (sub % 2) { /* ������������ */
            seed_size = stream_left_len(&sp);
            stream_nbyte_read(&sp, seed, MIN(sizeof(seed), seed_size));
            log_hex_d("Request seed ", seed, seed_size);
            YByteArrayClear(udsc->common_var.key_byte);
            if (item->ac_27.key_callback) {
                item->ac_27.key_callback(seed, sizeof(seed), sub, (const char *)udsc->id, key, &key_size);
                if (key_size <= sizeof(key)) {
                    log_hex_d("Request key ", key, key_size);
                    YByteArrayAppendNChar(udsc->common_var.key_byte, key, key_size);
                }
            }
            if (udsc->security_access_keygen_cb) {
                /* ����ΪOTA MASTERר�����õĻص�������OTA MASTER�����ӷ��͸�OTA MASTER API��������Key */
                udsc->security_access_keygen_cb(udsc->security_access_keygen_arg, udsc->id, sub, seed, seed_size);
                
                /* Ϊ�����㹻��ʱ��õ�keyֵ����ʱһ�º�����key���� */
                service_item *key_item = udsc_service(udsc, udsc_surplus_service_find(udsc, UDS_SERVICES_SA));
                if (key_item && key_item->sub % 2 == 0) {
                    key_item->delay = key_item->delay < IPC_MSG_VALID_TIME * 2 ? \
                                            IPC_MSG_VALID_TIME * 2 : key_item->delay;
                }
            }
        }
    }
    else {
        log_i("service 0x27 response not conform to the regulation");
        log_hex_d(" ", data, size);
    }
}

/* 36��������ļ��ر� */
static void udsc_service_36_td_file_close(uds_client *udsc)
{
    if (udsc->common_var.filefd) {
        fclose(udsc->common_var.filefd);
        udsc->common_var.filefd = NULL;
    }
}

static void udsc_service_36_td_file_cache_buff_free(uds_client *udsc)
{
    /* ���ָ�����ȼ��� */
    if (udsc->common_var.ff_cache_buff) {
        yfree(udsc->common_var.ff_cache_buff);
        udsc->common_var.ff_cache_buff = NULL;    
        udsc->common_var.td_buff = NULL;
    }
    else {        
        /* ���ָ�����ȼ��� */
        if (udsc->common_var.td_buff) {
            yfree(udsc->common_var.td_buff);
            udsc->common_var.td_buff = NULL;
        }
    }
}

/* ���յ�34��38�������Ӧʱ��Ϳ�������Ӧ��������36�����һЩ������ */
static int udsc_service_36_td_config_init(uds_client *udsc)
{
    struct stat file_info;
    service_item *td_36_item = NULL;    
    int rsize = 0;

    if (udsc->common_var.ff_cache_buff || \
        udsc->common_var.filefd) {
        /* �ļ��Ѿ��򿪻�ȡ�Ѿ�ȫ�������ڴ棬�����ظ���ȡ�ʹ� */
        return 0;
    }

    /* ���������һ��36���� */
    td_36_item = udsc_active_service_item(udsc);
    if (td_36_item == NULL || td_36_item->sid != UDS_SERVICES_TD) {
        td_36_item = udsc_service(udsc, udsc_surplus_service_find(udsc, UDS_SERVICES_TD));
    }
    /* �޷��ҵ�36���� */
    if (td_36_item == NULL) {
        return -1; 
    }

    /* 36������Ҫ����ı����ļ�·���Ƿ�Ϸ� */
    if (td_36_item->td_36.local_path == 0) {
        log_w("item->td_36.local_path is null");
        return -2;
    }

    /* Ϊ�˼�������������Ĵ��� */
    if (udsc->common_var.maxNumberOfBlockLength == 0) {
        udsc->common_var.maxNumberOfBlockLength = \
            td_36_item->td_36.maxNumberOfBlockLength;
    }
    
    /* �����鳤�������Ƿ�Ϸ� */
    if (udsc->common_var.maxNumberOfBlockLength == 0) {
        log_w("maxNumberOfBlockLength value zero error");
        return -3;
    }

    /* �Ƚ����õ�maxNumberOfBlockLength��34,36��Ӧ��ȡС����һ�� */
    if (td_36_item->td_36.maxNumberOfBlockLength > 0) {
        udsc->common_var.maxNumberOfBlockLength = \
        udsc->common_var.maxNumberOfBlockLength > td_36_item->td_36.maxNumberOfBlockLength ? \
        td_36_item->td_36.maxNumberOfBlockLength : udsc->common_var.maxNumberOfBlockLength;
    }

    /* ���ܳ������̼�ͨ��buff���� */
    if (udsc->common_var.maxNumberOfBlockLength > IPC_TX_BUFF_SIZE - \
        SERVICE_INDICATION_PAYLOAD_OFFSET - DM_IPC_EVENT_MSG_SIZE) {
        log_w("maxNumberOfBlockLength %d  too long", udsc->common_var.maxNumberOfBlockLength);
        udsc->common_var.maxNumberOfBlockLength = IPC_TX_BUFF_SIZE - \
        SERVICE_INDICATION_PAYLOAD_OFFSET - DM_IPC_EVENT_MSG_SIZE;
        log_w("maxNumberOfBlockLength Truncate into %d", udsc->common_var.maxNumberOfBlockLength);
    }
    
    /* �ļ����ڴ��ļ� */
    udsc->common_var.filefd = fopen(td_36_item->td_36.local_path, "rb");
    if (!udsc->common_var.filefd) {
        log_w("Failed to open the transfer file(%s)", td_36_item->td_36.local_path);
        return -4;
    }
    log_i("Open file => %s success", td_36_item->td_36.local_path);

    udsc->common_var.fileTransferCount = 1; /* ���к�1��ʼ */
    udsc->common_var.fileTransferTotalSize = 0;
    /* ��ȡ��Ҫ������ļ���С */
    if (stat(td_36_item->td_36.local_path, &file_info) != 0) {
        log_f("Failed to obtain the file(%s) size", td_36_item->td_36.local_path);
        return -5;
    }
    udsc->common_var.fileSize = file_info.st_size;
    
    udsc_service_36_td_file_cache_buff_free(udsc);
    /* ����ļ�С��ĳһ��С��ֱ�Ӷ����ļ������������ڴ� */
    if (udsc->common_var.fileSize <= FIRMWARE_FILE_CACHE_BUFF_MAX) {
        log_i("Start to malloc for file cache buff size %d", udsc->common_var.fileSize);
        udsc->common_var.ff_cache_buff = ymalloc(udsc->common_var.fileSize);
        if (!udsc->common_var.ff_cache_buff) {
            log_f("Description Failed to malloc");
        }
        else {            
            rsize = fread(udsc->common_var.ff_cache_buff, 1, \
                udsc->common_var.fileSize, udsc->common_var.filefd);
            if (udsc->common_var.fileSize != rsize) {
                log_f("Failed to read the file into memory");
                /* һ���Զ�ȡ�ļ�ʧ�ܣ��ͷ��ڴ�ת�����������ļ���ȡ */
                yfree(udsc->common_var.ff_cache_buff);
            }
            else {
                log_i("The file was successfully read into memory");
                /* �ļ���ȡ����ֱ�ӹر��ļ� */
                udsc_service_36_td_file_close(udsc);
            }
        }
    }

    /* ��һ�ζ����ļ����ڴ棬����ÿ�η���36�����ʱ���ȡ�ļ� */
    if (udsc->common_var.ff_cache_buff == NULL) {
        /* ����36�����ļ���ȡ������ */
        udsc->common_var.td_buff = ymalloc(udsc->common_var.maxNumberOfBlockLength);
        if (udsc->common_var.td_buff == NULL) {
            log_f("Malloc size %d failed", udsc->common_var.maxNumberOfBlockLength);        
            return -6;
        }
    }
    td_36_item->td_36.is_retrans = false;

    log_i("fileTransferCount: %d", udsc->common_var.fileTransferCount);    
    log_i("fileTransferTotalSize: %d", udsc->common_var.fileTransferTotalSize);    
    log_i("fileSize: %d", udsc->common_var.fileSize);    
    log_i("maxNumberOfBlockLength: %d", udsc->common_var.maxNumberOfBlockLength);

    return 0;
}

static void udsc_service_38_rft_response_handler(uds_client *udsc, yuint8 *data, yuint32 size)
{
    stream_t sp;
    int nbyte = 0;
    yuint8 modeOfOperation = 0;
    yuint8 lengthFormatIdentifier = 0;
    yuint8 byte = 0;
    yuint8 dataFormatIdentifier = 0;

    if (!udsc_service_negative_response(data, size)) {
        stream_init(&sp, data, size);
        stream_byte_read(&sp); /* ������SID */
        modeOfOperation = stream_byte_read(&sp);
        if (modeOfOperation == UDS_RFT_MODE_ADD_FILE || \
            modeOfOperation == UDS_RFT_MODE_REPLACE_FILE) {
            lengthFormatIdentifier = stream_byte_read(&sp);
            for (nbyte = 0; nbyte < lengthFormatIdentifier; nbyte++) {
                byte = 0;
                byte = stream_byte_read(&sp);
                udsc->common_var.maxNumberOfBlockLength |=  byte << ((lengthFormatIdentifier - nbyte - 1) * 8);
            }
            dataFormatIdentifier = stream_byte_read(&sp);
            Y_UNUSED(dataFormatIdentifier);
        }
        log_i("maxNumberOfBlockLength => %d", udsc->common_var.maxNumberOfBlockLength);
        /* ���úͳ�ʼ��36����ʹ�õ��Ĳ��� */
        if (udsc_surplus_service_container(udsc, UDS_SERVICES_TD)) {
            udsc_service_36_td_config_init(udsc);
        }
    }    
    else {
        log_i("service 0x38 response not conform to the regulation");
        log_hex_i(" ", data, size);
    }
}

static void udsc_service_34_rd_response_handler(uds_client *udsc, yuint8 *data, yuint32 size)
{
    stream_t sp;
    int nbyte = 0;

    if (!udsc_service_negative_response(data, size)) {
        stream_init(&sp, data, size);
        stream_byte_read(&sp); /* ������SID */

        yuint8 lengthFormatIdentifier = stream_byte_read(&sp) >> 4 & 0x0f;
        for (nbyte = 0; nbyte < lengthFormatIdentifier; nbyte++) {
            yuint8 byte = 0;

            byte = stream_byte_read(&sp);
            udsc->common_var.maxNumberOfBlockLength |=  byte << ((lengthFormatIdentifier - nbyte - 1) * 8);
        }
        log_i("maxNumberOfBlockLength => %d", udsc->common_var.maxNumberOfBlockLength);
        /* ���úͳ�ʼ��36����ʹ�õ��Ĳ��� */
        if (udsc_surplus_service_container(udsc, UDS_SERVICES_TD)) {
            udsc_service_36_td_config_init(udsc);
        }
    }
    else {
        log_i("service 0x34 response not conform to the regulation");
        log_hex_i(" ", data, size);
    }
}

/* ����36�����ش� */
static BOOLEAN udsc_service_36_td_retrans_handler(uds_client *udsc)
{
    service_item *item = udsc_active_service_item(udsc);

    item->td_36.rt_cnt++; /* �ش������ۼ� */
    if (item->td_36.rt_cnt > TD_RETRANS_NUM_MAX) {
        /* �Ѿ��ﵽ����ش����� */
        log_w("The maximum number %d of retransmissions has been reached", TD_RETRANS_NUM_MAX);
        udsc_curr_service_next(udsc);
        /* ��ʱ�ر�36�������ļ������� */
        udsc_service_36_td_file_close(udsc);        
        /* ͬʱ�ͷ��ļ�����ռ� */
        udsc_service_36_td_file_cache_buff_free(udsc);
        log_i("Stop 36 td transfer progress notify timer");
        ev_timer_stop(udsc->loop, &udsc->td_36_progress_watcher);        
        udsc_td_36_progress_notify(udsc);
    }
    else {
        /* ���Խ����ش�36���� */
        item->td_36.is_retrans = true; /* ��ǿ�ʼ�ش� */
        log_i("retransmit 36 service count %d", udsc->common_var.fileTransferCount);
        udsc_curr_service_keep(udsc);
    }

    return true;
}

static void udsc_service_36_td_response_handler(uds_client *udsc, yuint8 *data, yuint32 size)
{
    service_item *item = udsc_active_service_item(udsc);

    if (udsc_service_negative_response(data, size)) {
        /* �յ�����Ӧ�˳��Խ����ش� */
        log_i("36 service negative response attempt retransmission");
        udsc_service_36_td_retrans_handler(udsc);
    }
    else {
        if (size != 2 || data[1] != udsc->common_var.fileTransferCount) {
            /* ����Ӧ������Ҫ���Խ����ش� */ 
            log_i("The 36 service positive response does not match");
            udsc_service_36_td_retrans_handler(udsc);
        }
        else {
            item->td_36.rt_cnt = 0; /* �ش�������� */
            udsc->common_var.fileTransferCount++; /* ���к��ۼ� */
            udsc->common_var.fileTransferTotalSize += (udsc->common_var.maxNumberOfBlockLength - 2);
            if (udsc->common_var.fileTransferTotalSize > udsc->common_var.fileSize) {
                udsc->common_var.fileTransferTotalSize = udsc->common_var.fileSize;
            }
            log_d("fileTransferTotalSize => %d fileSize => %d", \
                    udsc->common_var.fileTransferTotalSize, udsc->common_var.fileSize);
            if (udsc->common_var.fileTransferTotalSize >= udsc->common_var.fileSize) {
                log_d("The file has been transferred fileTransferTotalSize => %d fileSize => %d", \
                    udsc->common_var.fileTransferTotalSize, udsc->common_var.fileSize);                
                if (udsc->common_var.ff_cache_buff || \
                    udsc->common_var.filefd) {
                    /* ������ɺ������ʱ���� */
                    YByteArrayClear(item->request_byte);
                    YByteArrayAppendChar(item->request_byte, UDS_SERVICES_TD);
                }
                /* �ļ��Ѿ�������� */
                udsc_service_36_td_file_close(udsc);
                /* ͬʱ�ͷ��ļ�����ռ� */
                udsc_service_36_td_file_cache_buff_free(udsc);
                log_i("Stop 36 td transfer progress notify timer");
                ev_timer_stop(udsc->loop, &udsc->td_36_progress_watcher);
                udsc_td_36_progress_notify(udsc);
            }
            else {
                /* �ļ�����δ��ɣ����������ļ� */
                udsc_curr_service_keep(udsc);
            }
        }
    }
}

static void udsc_service_37_rte_response_handler(uds_client *udsc, yuint8 *data, yuint32 size)
{
    /* �ļ��Ѿ��򿪹ر��ļ� */
    udsc_service_36_td_file_close(udsc);
    log_i("service 0x37 close file");    
    /* ͬʱ�ͷ��ļ�����ռ� */
    udsc_service_36_td_file_cache_buff_free(udsc);
}

static void udsc_service_10_dsc_response_handler(uds_client *udsc, yuint8 *data, yuint32 size)
{
    if (!udsc_service_negative_response(data, size) &&\
         size == DSC_10_RESPONSE_FIX_LEN) {
        udsc->common_var.p2_server_max = data[2] << 8 | data[3];
        udsc->common_var.p2_e_server_max = data[4] << 8 | data[5];
        log_i("p2_server_max => %d", udsc->common_var.p2_server_max);
        log_i("p2_e_server_max => %d", udsc->common_var.p2_e_server_max);
    }
    else {
        log_i("service 0x10 response not conform to the regulation");
        log_hex_d(" ", data, size);
    }
}

static void udsc_service_response_handler(uds_client *udsc, yuint32 sa, yuint32 ta, yuint8 *data, yuint32 size)
{
    yuint8 rsid = 0;

    /* ��������Ӧ����������Ӧ */
    rsid = udsc_service_response_sid(data, size);
    log_d("Service (%s) response", ydesc_uds_services(rsid));
    switch (rsid) {
        case UDS_SERVICES_SA:
            udsc_service_27_sa_response_handler(udsc, data, size);
            return ;
        case UDS_SERVICES_RFT:
            udsc_service_38_rft_response_handler(udsc, data, size);    
            return ;
        case UDS_SERVICES_RD:
            udsc_service_34_rd_response_handler(udsc, data, size);  
            return ;
        case UDS_SERVICES_TD:
            udsc_service_36_td_response_handler(udsc, data, size);
            return ;
        case UDS_SERVICES_RTE:
            udsc_service_37_rte_response_handler(udsc, data, size);
            return ;
        case UDS_SERVICES_DSC:
            udsc_service_10_dsc_response_handler(udsc, data, size);            
            return ;
        default:

            break;
    }
}

static void udsc_service_response_timeout_handler(uds_client *udsc)
{
    service_item *item = udsc_active_service_item(udsc);

    switch (item->sid) {
        case UDS_SERVICES_TD:
            udsc_service_36_td_retrans_handler(udsc);
            break;
        default:
            break;
    }
}

BOOLEAN udsc_is_hex_digit(char c) 
{  
    if ((c >= '0' && c <= '9') || 
        (c >= 'a' && c <= 'f') ||   
        (c >= 'A' && c <= 'F')) {  
        
        return true;  
    }  

    return false;  
}

static void udsc_service_match_response_expect(uds_client *udsc, service_response_stat stat, yuint32 sa, yuint32 ta, yuint8 *data, yuint32 size)
{
    int arrindex = 0;
    BOOLEAN ismatch = false;
    service_item *item = udsc_active_service_item(udsc);

    if (stat == SERVICE_RESPONSE_NORMAL && \
        (item->response_rule == NO_RESPONSE_EXPECT || \
         item->issuppress == true)) {            
        udsc_service_response_unexpect(udsc);
        log_e("Expected response %d, the actual response timed out.", item->response_rule);
        return ;
    }

    if (item->response_rule == NOT_SET_RESPONSE_EXPECT) {

    }
    else if (item->response_rule == POSITIVE_RESPONSE_EXPECT) {    
        if (stat == SERVICE_RESPONSE_TIMEOUT) {
            /*  */
            udsc_service_response_unexpect(udsc);
            log_w("Response matching error");
            return ;
        }
        if (udsc_service_negative_response(data, size) || \
            udsc_service_response_sid(data, size) != item->sid) {
            udsc_service_response_unexpect(udsc);
            log_i("Expected response Positive response, actual negative response");
            return ;   
        } 
    }
    else if (item->response_rule == NEGATIVE_RESPONSE_EXPECT) {    
        if (stat == SERVICE_RESPONSE_TIMEOUT) {
            udsc_service_response_unexpect(udsc);
            log_w("Response matching error");
            return ;
        }    
        if (!udsc_service_negative_response(data, size) || \
            udsc_service_response_sid(data, size) != item->sid) {
            udsc_service_response_unexpect(udsc);
            log_i("Expected response negative response, actual positive response");
            return ;   
        } 
    }
    else if (item->response_rule == MATCH_RESPONSE_EXPECT) {
        if (stat == SERVICE_RESPONSE_TIMEOUT) {
            udsc_service_response_unexpect(udsc);
            log_w("Response matching error");
            return ;
        }

        for (arrindex = 0; arrindex < sizeof(item->expect_bytes) / sizeof(item->expect_bytes[0]) && \
                                          item->expect_bytes[arrindex]; arrindex++) {
            if (YByteArrayCharEqual(item->expect_bytes[arrindex], data, size)) {
                ismatch = true;
            }
        }
        
        if (!ismatch) {
            udsc_service_response_unexpect(udsc);
            log_hex_i("expect response", item->expect_byte->data, item->expect_byte->dlen);
            log_hex_i("actual response", data, size);
            return ;
        }
    }
    else if (item->response_rule == NO_RESPONSE_EXPECT) {
        if (stat == SERVICE_RESPONSE_NORMAL) {
            udsc_service_response_unexpect(udsc);
            log_i("Expected no response");
            log_hex_i("actual response", data, size);
            return ;
        }
    }
}

static void udsc_service_match_finish_rule(uds_client *udsc, service_response_stat stat, yuint32 sa, yuint32 ta, yuint8 *data, yuint32 size)
{
    int arrindex = 0;
    BOOLEAN ismatch = false;
    service_item *item = udsc_active_service_item(udsc);

    if (item->finish_rule == FINISH_DEFAULT_SETTING) {
        /* ignore */
        /* Ĭ����ִ����һ����Ϸ��� */
        udsc_curr_service_next(udsc);
    }
    else if (item->finish_rule == FINISH_EQUAL_TO) {        
        /* ƥ�䵽�˾�Ҫ�˳���ǰ����ִ�У�û��ƥ�䵽�˾�Ҫ���ֵ�ǰ����ִ�� */
        for (arrindex = 0; arrindex < sizeof(item->finish_bytes) / sizeof(item->finish_bytes[0]) && \
                                          item->finish_bytes[arrindex]; arrindex++) {
            if (YByteArrayCharEqual(item->finish_bytes[arrindex], data, size)) {
                ismatch = true;
            }
        }
        if (ismatch == false) {
            udsc_curr_service_keep(udsc);
            item->finish_try_num++;
            if (item->finish_try_num > item->finish_num_max) {
                /* ����������Դ�����ִ����һ����Ϸ��� */
                item->finish_try_num = 0;
                udsc_curr_service_next(udsc);            
                log_i("When the maximum number of attempts is exceeded, the next diagnostic service item is executed");
            }
        }
        else {
            /* ������ǰ����ִ�� */
            item->finish_try_num = 0;
        }
    }
    else if (item->finish_rule == FINISH_UN_EQUAL_TO) { 
        /* ûƥ�䵽Ԥ��������Ӧ�˳���ǰ����ִ�У�ƥ�䵽�˾�Ҫ���ֵ�ǰ����ִ�� */
        for (arrindex = 0; arrindex < sizeof(item->finish_bytes) / sizeof(item->finish_bytes[0]) && \
                                          item->finish_bytes[arrindex]; arrindex++) {
            if (YByteArrayCharEqual(item->finish_bytes[arrindex], data, size)) {
                ismatch = true;
            }
        }
        if (ismatch == true) {        
            udsc_curr_service_keep(udsc);
            item->finish_try_num++;
            if (item->finish_try_num > item->finish_num_max) {
                /* ����������Դ�����ִ����һ����Ϸ��� */
                item->finish_try_num = 0;
                udsc_curr_service_next(udsc);            
                log_i("When the maximum number of attempts is exceeded, the next diagnostic service item is executed");
            }
        }        
        else {
            /* ������ǰ����ִ�� */
            item->finish_try_num = 0;
        }
    }
    else if (item->finish_rule == FINISH_EQUAL_TO_POSITIVE_RESPONSE) { /* �յ�����Ӧ�ͽ�����ǰ����Ϸ��� */
        if (stat != SERVICE_RESPONSE_NORMAL || \
            udsc_service_negative_response(data, size)) {        
            /* ������Ԥ�ڽ�����Ӧ���������ֵ�ǰ��Ϸ���ִ�� */
            udsc_curr_service_keep(udsc);
            item->finish_try_num++;
            if (item->finish_try_num > item->finish_num_max) {
                /* ����������Դ�����ִ����һ����Ϸ��� */
                item->finish_try_num = 0;
                udsc_curr_service_next(udsc);            
                log_i("When the maximum number of attempts is exceeded, the next diagnostic service item is executed");
            }
        }        
        else {
            /* ������ǰ����ִ�� */
            item->finish_try_num = 0;
        }        
    }
    else if (item->finish_rule == FINISH_EQUAL_TO_NEGATIVE_RESPONSE) { /* �յ�����Ӧ�ͽ�����ǰ����Ϸ��� */
        if (stat != SERVICE_RESPONSE_NORMAL || \
            !udsc_service_negative_response(data, size)) {        
            /* ������Ԥ�ڽ�����Ӧ���������ֵ�ǰ��Ϸ���ִ�� */
            udsc_curr_service_keep(udsc);
            item->finish_try_num++;
            if (item->finish_try_num > item->finish_num_max) {
                /* ����������Դ�����ִ����һ����Ϸ��� */
                item->finish_try_num = 0;
                udsc_curr_service_next(udsc);            
                log_i("When the maximum number of attempts is exceeded, the next diagnostic service item is executed");
            }
        }        
        else {
            /* ������ǰ����ִ�� */
            item->finish_try_num = 0;
        }
    }
    else if (item->finish_rule == FINISH_EQUAL_TO_NO_RESPONSE) { /* δ�յ���Ӧ�ͽ�����ǰ����Ϸ��� */
        if (stat == SERVICE_RESPONSE_NORMAL) {        
            /* ������Ԥ�ڽ�����Ӧ���������ֵ�ǰ��Ϸ���ִ�� */
            udsc_curr_service_keep(udsc);
            item->finish_try_num++;
            if (item->finish_try_num > item->finish_num_max) {
                /* ����������Դ�����ִ����һ����Ϸ��� */
                item->finish_try_num = 0;
                udsc_curr_service_next(udsc);            
                log_i("When the maximum number of attempts is exceeded, the next diagnostic service item is executed");
            }
        }        
        else {
            /* ������ǰ����ִ�� */
            item->finish_try_num = 0;
        }
    }
    else if (item->finish_rule == FINISH_EQUAL_TO_NORMAL_RESPONSE) { /* �յ���Ӧ���ľͽ�����ǰ����Ϸ��� */
        if (stat != SERVICE_RESPONSE_NORMAL) {        
            /* ������Ԥ�ڽ�����Ӧ���������ֵ�ǰ��Ϸ���ִ�� */
            udsc_curr_service_keep(udsc);
            item->finish_try_num++;
            if (item->finish_try_num > item->finish_num_max) {
                /* ����������Դ�����ִ����һ����Ϸ��� */
                item->finish_try_num = 0;
                udsc_curr_service_next(udsc);
                log_i("When the maximum number of attempts is exceeded, the next diagnostic service item is executed");
            }
        }        
        else {
            /* ������ǰ����ִ�� */
            item->finish_try_num = 0;
        }
    }
}

static void udsc_recv_data_statis(uds_client *udsc, int size)
{
    if (!udsc->runtime_statis_enable) {
        return ;
    }
    udsc->rt_data_statis.recv_num++;
    udsc->rt_data_statis.recv_total_byte += size;
    udsc->recv_total_byte += size;
    udsc_transfer_average_speed(&udsc->recv_pretime, &udsc->recv_total_byte, &udsc->recv_average_byte);
    if (udsc->recv_average_byte > 0) {
        udsc->rt_data_statis.recv_valley_speed = \
            udsc->recv_average_byte < \
            udsc->rt_data_statis.recv_valley_speed ? \
            udsc->recv_average_byte : \
            udsc->rt_data_statis.recv_valley_speed;
        udsc->rt_data_statis.recv_peak_speed = \
            udsc->recv_average_byte > \
            udsc->rt_data_statis.recv_peak_speed ? \
            udsc->recv_average_byte : \
            udsc->rt_data_statis.recv_peak_speed;
    }
}

static void udsc_service_response_finish(uds_client *udsc, service_response_stat stat, yuint32 sa, yuint32 ta, yuint8 *data, yuint32 size)
{
    yuint32 fcode = UDSC_NORMAL_FINISH;
    service_item *item = NULL;

    log_put_mdc(DM_DIAG_MASTER_MDC_KEY, udsc->om_name);
    log_put_mdc(DM_UDSC_MDC_KEY, udsc->udsc_name);
    /* û���������Ҫ����ֱ�ӷ��� */
    if (!udsc_service_isactive(udsc)) {        
        log_w("The service request has been stopped");
        return ;
    }

    if (data == NULL || size == 0) {
        /* û����Ӧ���ݾ��ǳ�ʱ�� */
        stat = SERVICE_RESPONSE_TIMEOUT;
    }

    /* ������������ͳ�� */
    if (stat == SERVICE_RESPONSE_NORMAL) {
        udsc_recv_data_statis(udsc, size);
        log_hex_d("Recv response: ", data, size);
    }
    else {
        log_i("Recv response timeout");
    }

    /* ���һ���Ƿ������Ӧ������ */
    for ( ; stat == SERVICE_RESPONSE_NORMAL; ) {
        item = udsc_active_service_item(udsc);
        if(udsc_service_response_sid(data, size) == item->sid) {
           /* �ǵ�ǰ�������Ӧ��Ϣ */
            break; /* for */
        }       
        log_w("The response does not meet current service item expectations");
        log_w("Current service sid %02x", item->sid);
        log_hex_w(" ", data, size);
        return ;
    }
#ifdef __HAVE_UDS_PROTOCOL_ANALYSIS__
    if (udsc->uds_msg_parse_enable) {
        yuds_protocol_parse(data, size);
    }
#endif /* __HAVE_UDS_PROTOCOL_ANALYSIS__ */                    
    /* �յ�NRC 78hˢ��Ӧ��ʱ��ʱ�� */
    if (udsc_service_response_nrc(data, size) == \
        UDS_RESPONSE_CODES_RCRRP) {
        udsc_service_response_wait_refresh(udsc);
        /* ��ʱ��ˢ�¾Ϳ����˳��� */
        return ;
    }

    /* ��¼һ����Ӧ�������� */
    if (stat == SERVICE_RESPONSE_NORMAL && \
        data && size > 0) {
        item = udsc_active_service_item(udsc);
        if (item->response_byte == NULL) {
            item->response_byte = YByteArrayNew();
        }
        YByteArrayClear(item->response_byte);
        YByteArrayAppendNChar(item->response_byte, data, size);
    }
        
    udsc_service_response_wait_stop(udsc); /* �Ѿ��յ�Ӧ��ֹͣӦ��ʱ��ʱ�� */
    
    udsc_service_response_expect(udsc); /* �ȱ��Ӧ���Ƿ���Ҫ��� */
    udsc_curr_service_next(udsc); /* Ԥ�ȱ�ǲ���ִ�е�ǰ������ */

    /* ���Ӧ���Ƿ���Ͻ������� */
    udsc_service_match_finish_rule(udsc, stat, sa, ta, data, size);

    /* ���Ӧ���Ƿ����Ԥ��Ӧ�� */
    udsc_service_match_response_expect(udsc, stat, sa, ta, data, size);
    if (!udsc_service_response_meet_expect(udsc)) {
        udsc_active_service_item(udsc)->status = SERVICE_ITEM_FAILED;
        if (udsc_is_fail_abort(udsc)) {
            /* Ӧ�������ֹ���е���Ϸ���ִ��  */
            goto RESPONSE_FINISH;
        }
    }
        
    /* �����յ���Ӧ���ݣ�������Ӧ���� */
    if (stat == SERVICE_RESPONSE_NORMAL) {
        udsc_service_response_handler(udsc, sa, ta, data, size);
    }
    else {
        /* ��ʱ��Ӧ���� */
        udsc_service_response_timeout_handler(udsc);
    }

    /* �ж���Ӧ�Ƿ����Ҫ��Ͳ�����Ҫ����Ƿ��жϺ������������ִ�� */
    if (!udsc_service_response_meet_expect(udsc)) {
        udsc_active_service_item(udsc)->status = SERVICE_ITEM_FAILED;
        if (udsc_is_fail_abort(udsc)) {
            /* Ӧ������ն�ִ��  */
            goto RESPONSE_FINISH;
        }
    }

    udsc_active_service_item(udsc)->status = SERVICE_ITEM_SUCCESS;
    if (udsc_service_is_keep(udsc)) {
        /* ����ִ�е�ǰ����� */
        item = udsc_active_service_item(udsc);
        /* ������ʱ��ʱ������ʼִ����Ϸ����� */
        udsc_service_request_delay_trigger(udsc);
        udsc_curr_service_next(udsc); /* ���ñ�־ */
    }
    else {
        /* ������һ��ʹ�ܵ���Ϸ����� */
        udsc_next_service_item(udsc);
        while (udsc_service_isactive(udsc)) {
            item = udsc_active_service_item(udsc);
            /* �жϵ�ǰ������Ƿ���Ҫִ�� */
            if (item->isenable) {
                /* ������ʱ��ʱ������ʼִ����Ϸ����� */
                udsc_service_request_delay_trigger(udsc);
                break; /* break while */
            }
            else {
                log_i("service %02x is set to no need to execute", item->sid);
            }
            udsc_next_service_item(udsc); /*  */
        }
        /* ���е�������Ѿ������� */
        if (!udsc_service_isactive(udsc)) {
            log_i("All diagnostic service items are executed finish");
            if (udsc->udsc_task_end_cb) {
                /* �������������ִ�н����ص����������� */
                udsc->udsc_task_end_cb(udsc, fcode, udsc->udsc_task_end_arg, 0, 0, 0, 0);                
                udsc_run_time_total_statis(udsc);
                yudsc_runtime_data_statis_get(udsc, 0);
            }            
            udsc_services_stop(udsc); /* ֹͣ�������������ִ�� */
        }
    }
    
RESPONSE_FINISH:
    if (!udsc_service_response_meet_expect(udsc)) {
        /* Ӧ�𲻷���Ԥ�� */
        log_i("The service response is not as expected");
        if (udsc_is_fail_abort(udsc)) {
            log_i("diagnostic service items are executed error finish");
            if (udsc->udsc_task_end_cb) {
                if (stat == SERVICE_RESPONSE_NORMAL) {
                    fcode = UDSC_UNEXPECT_RESPONSE_FINISH; /* ���յ��������������Ԥ�����õ�ֵ */
                } else {
                    fcode = UDSC_TIMEOUT_RESPONSE_FINISH; /* ���Ӧ��ʱ */
                }
                log_i("UDS services finish callback");
                item = udsc_active_service_item(udsc);
                udsc->udsc_task_end_cb(udsc, fcode, udsc->udsc_task_end_arg, \
                    YByteArrayConstData(item->request_byte), YByteArrayCount(item->request_byte), data, size);
                udsc_run_time_total_statis(udsc);
                yudsc_runtime_data_statis_get(udsc, 0);
            }            
            udsc_services_stop(udsc); /* ֹͣ�������������ִ�� */
        }
    }

    return ;
}

void yudsc_service_response_finish(uds_client *udsc, service_response_stat stat, yuint32 sa, yuint32 ta, yuint8 *data, yuint32 size)
{
    udsc_service_response_finish(udsc, stat, sa, ta, data, size);
}

static void udsc_service_item_destroy(service_item **item)
{
    int arrindex = 0;

    if ((*item)) {
        if ((*item)->desc) {
            yfree((*item)->desc);
            (*item)->desc = NULL;
        }
        YByteArrayDelete((*item)->request_byte);
        YByteArrayDelete((*item)->expect_byte);
        for (arrindex = 0; arrindex < sizeof((*item)->expect_bytes) / sizeof((*item)->expect_bytes[0]) && \
                                        (*item)->expect_bytes[arrindex]; arrindex++) {
            YByteArrayDelete((*item)->expect_bytes[arrindex]);
        }        
        for (arrindex = 0; arrindex < sizeof((*item)->finish_bytes) / sizeof((*item)->finish_bytes[0]) && \
                                        (*item)->finish_bytes[arrindex]; arrindex++) {
            YByteArrayDelete((*item)->finish_bytes[arrindex]);
        }
        YByteArrayDelete((*item)->finish_byte);
        YByteArrayDelete((*item)->variable_byte);
        YByteArrayDelete((*item)->response_byte);
        if ((*item)->td_36.local_path) {
            yfree((*item)->td_36.local_path);
        }
        if ((*item)->rft_38) {
            if ((*item)->rft_38->filePathAndName) {
                yfree((*item)->rft_38->filePathAndName);
            }
            yfree((*item)->rft_38);
        }
        yfree((*item));
        (*item) = NULL;
    }
}

/* ���ڼ���ʵʱ�������� */
static int udsc_transfer_average_speed(yuint32 *preTime, int *totalByte, int *averageByte)
{
    yuint32 tv_sec = 0, tv_usec = 0;
    yuint32 currtime = 0;
    int intervalSecs = 0;

    /* ��ȡ��ǰʱ�� */
    ydm_tv_get(&tv_sec, &tv_usec);
    currtime = tv_sec;
    /* ��ȡ��ǰʱ�����һ�δ������ʵļ���ʱ��� */
    intervalSecs = currtime - *preTime;
    if (intervalSecs > 2) {
        if (intervalSecs > 3) {
            /* �м���������쳣�����ü������� */
            *averageByte = 0;
            *totalByte = 0;
            *preTime = currtime;
        } else {
            /* �������㴫������ */
            *totalByte -= (*averageByte * intervalSecs); /* һ��ʱ���ڴ���������ݼ�ȥ����ȥһ��ʱ����ƽ������������� */
            *preTime = *preTime + intervalSecs; /* ��¼��ǰ���㴫�����ʼ����ϵͳʱ�� */
            *averageByte = *totalByte / intervalSecs; /* ���㴫������ */
            if (*averageByte < 0) {
                /* �м���������쳣�����ü������� */
                *averageByte = 0;
                *totalByte = 0;
                *preTime = currtime;
            }
        }
    }
    else if (intervalSecs < 0) {
        /* �м���������쳣�����ü������� */
        *averageByte = 0;
        *totalByte = 0;
        *preTime = currtime;
    }
    
    if (*totalByte < 0) {
        /* �м���������쳣�����ü������� */
        *averageByte = 0;
        *totalByte = 0;
        *preTime = currtime;
    }

    return *averageByte;
}

static void udsc_service_handler_callback(struct ev_loop *loop, ev_timer *w, int revents)
{
    uds_client *udsc = container_of(w, uds_client, request_watcher); 
    ev_timer_stop(loop, w);
    udsc_service_request_handler(udsc);    
}
static void udsc_response_timeout_callback(struct ev_loop *loop, ev_timer *w, int revents)
{
    uds_client* udsc = container_of(w, uds_client, response_watcher);
    ev_timer_stop(loop, w);
    yuint8 data[8] = {0};
    udsc_service_response_finish(udsc, SERVICE_RESPONSE_TIMEOUT, 0, 0, data, 0);   
}
static void udsc_tester_present_callback(struct ev_loop *loop, ev_timer *w, int revents)
{
    uds_client* udsc = container_of(w, uds_client, testerPresent_watcher);
    if (udsc->udsc_request_transfer_cb) {
        udsc->udsc_request_transfer_cb(udsc->id, udsc->udsc_request_transfer_arg, (yuint8 *)"\x3e\x80", 2, udsc->tpsa, udsc->tpta, 0);
    }
}

static void udsc_td_36_progress_notify_callback(struct ev_loop *loop, ev_timer *w, int revents)
{
    uds_client* udsc = container_of(w, uds_client, td_36_progress_watcher);
    udsc_td_36_progress_notify(udsc);
}

void udsc_common_var_reset(uds_client *udsc)
{
    udsc->common_var.fileTransferCount = 0;
    udsc->common_var.fileTransferTotalSize = 0;
    udsc->common_var.fileSize = 0;
    udsc->common_var.maxNumberOfBlockLength = 0;
    udsc_service_36_td_file_cache_buff_free(udsc);
    udsc_service_36_td_file_close(udsc);    
    YByteArrayClear(udsc->common_var.key_byte);
    YByteArrayClear(udsc->common_var.seed_byte);
    udsc->common_var.p2_server_max = 0;
    udsc->common_var.p2_e_server_max = 0;
}

void udsc_reset(uds_client *udsc)
{
    int ii = 0;

    /* �Ƴ����е���Ϸ����� */
    for (ii = 0; ii < udsc->service_cnt; ii++) {
        service_item *item = udsc->service_items[ii];
        if (item) {
            udsc_service_item_destroy(&item);
            udsc->service_items[ii] = NULL;
        }
    }
    udsc->service_cnt = 0;

    udsc->isFailAbort = false; /* ������������������ʱ����ִֹ�з���������� */
    udsc_service_response_unexpect(udsc); /* Ӧ�����Ԥ��Ҫ�� */
    udsc_service_item_invalid_set(udsc); /* ��ǰ���д������Ϸ��������� service_items[sindex] */
    udsc_curr_service_next(udsc); /* �Ƿ����ִ�е�ǰ��Ϸ����Ĭ�ϲ�����֧�ֵ�ǰ������й�����ȷ�� */
    udsc->service_cnt = 0; /* ��Ϸ������� */
    udsc->udsc_request_transfer_arg = NULL;
    udsc->udsc_request_transfer_cb = NULL; /* �����Ϣ���ͺ���������Ҫ������������ʱ����� */
    udsc->udsc_task_end_arg = NULL;
    udsc->udsc_task_end_cb = NULL; /* ��Ͽͻ��˽������� */
    udsc->security_access_keygen_arg = NULL;
    udsc->security_access_keygen_cb = NULL;
    udsc->service_36_transfer_progress_arg = NULL;
    udsc->service_36_transfer_progress_cb = NULL;
    udsc->td_36_start_time = 0;
    udsc->td_36_progress_interval = 0;
    udsc_common_var_reset(udsc); /* ���ò������� */
    udsc->tpEnable = false; /* ʹ����������������� */
    udsc->isTpRefresh = false; /* �Ƿ�UDS����ˢ�¶�ʱ�� */
    udsc->tpInterval = 0; /* ���ͼ�� unit ms */
    udsc->tpta = 0; /* ���Ŀ�ĵ�ַ */
    udsc->tpsa = 0; /* ���Դ��ַ */

    if (udsc->udsc_name) {
        yfree(udsc->udsc_name);
        udsc->udsc_name = NULL;
    }
    udsc->isidle = true;
}

void yudsc_ev_loop_set(uds_client* udsc, struct ev_loop* loop)
{
    udsc->loop = loop;
}

static BOOLEAN udsc_service_item_capacity_init(uds_client *udsc, int size)
{
    int ii = 0;
    void *tp = NULL;

    for (ii = 0; ii < udsc->service_size; ii++) {
        if (udsc->service_items[ii]) {
            log_e("Service item capacity init failed");
            return false;
        }
    }

    if (udsc->service_items) {
        tp = udsc->service_items; 
        udsc->service_items = NULL;
    }
    size = size > SERVICE_ITEM_SIZE_MAX ? SERVICE_ITEM_SIZE_MAX : size;
    udsc->service_items = ycalloc(sizeof(service_item *), \
        size + 1 /* Ԥ��һ�������Զ����ʹ�� */);
    if (udsc->service_items == NULL) {
        udsc->service_items = tp;
    }
    else {
        udsc->service_size = size;
        yfree(tp);
    }

    return udsc->service_items ? true : false;
}

/* ��������ʼ��uds�ͻ��� */
uds_client *yudsc_create()
{
    uds_client *udsc = ymalloc(sizeof(*udsc));
    if (udsc == NULL) {
        return NULL;
    }
    memset(udsc, 0, sizeof(*udsc));
    
    udsc->loop = EV_DEFAULT; /* ʹ���¼�ѭ��Ĭ������ */
    /* �����������ʱ�� */
    ev_timer_init(&udsc->request_watcher, udsc_service_handler_callback, 0.0, 0.0);
    /* ����Ӧ��ʱ��ʱ�� */
    ev_timer_init(&udsc->response_watcher, udsc_response_timeout_callback, 0.0, 0.0);    
    /* ����������� */
    ev_timer_init(&udsc->testerPresent_watcher, udsc_tester_present_callback, 0.0, 0.0);
    udsc->sindex = -1; /* ��ǰ��Ҫִ�е������С��0��ʾû����Ҫִ�� */
    udsc->isFailAbort = false; /* Ĭ����Ϸ����������ֹ�������� */
    udsc->service_cnt = 0; /* ��Ϸ����б������ܺ� */
    /* ������Ϸ����б����� */
    udsc_service_item_capacity_init(udsc, SERVICE_ITEM_SIZE_DEF);
    if (udsc->service_items == 0) {
        log_e("calloc(sizeof(service_item *), udsc->service_size) error.");        
        yfree(udsc);
        return NULL;
    }

    udsc->common_var.seed_byte = YByteArrayNew();
    udsc->common_var.key_byte = YByteArrayNew();

    udsc->common_var.filefd = NULL;
    udsc->common_var.td_buff = NULL;
    udsc->common_var.ff_cache_buff = NULL;

    udsc_reset(udsc);

    /* ----------------����һЩ��ʼֵ-------------------- */
    yudsc_name_set(udsc, "uds client default");

    return udsc;
}

/* ��UDS�ͻ��˸�ԭ�ɸմ���ʱ���״̬ */
BOOLEAN yudsc_reset(uds_client *udsc) 
{
    if (udsc_service_isactive(udsc)) {
        log_w("The uds client is active and cannot be reset");
        return false;
    }
    udsc_reset(udsc);

    return true;
}

/* ����uds�ͻ��� */
BOOLEAN yudsc_destroy(uds_client *udsc)
{
    if (udsc_service_isactive(udsc)) {
        log_w("The uds client is active and cannot be destroyed");
        return false;
    }
        
    udsc_reset(udsc);

    if (udsc->service_items) {
        yfree(udsc->service_items);
        udsc->service_items = NULL;
    }
    
    YByteArrayDelete(udsc->common_var.seed_byte);
    YByteArrayDelete(udsc->common_var.key_byte);

    memset(udsc, 0, sizeof(*udsc));
    yfree(udsc);

    return true;
}

/*
    ��Ϸ������Ƿ���ִ����
*/
BOOLEAN yudsc_service_isactive(uds_client *udsc)
{
    if (udsc == NULL) {
        log_e("The fatal error parameter cannot be null.");
        return false;
    }

    return udsc_service_isactive(udsc);
}

BOOLEAN yudsc_runtime_data_statis_get(uds_client *udsc, runtime_data_statis_t *rt_data_statis)
{
    if (!udsc->runtime_statis_enable) {
        return false;
    }   

    if (rt_data_statis) {
        memcpy(rt_data_statis, &udsc->rt_data_statis, sizeof(*rt_data_statis));
    }
    log_i("Sent byte total     : %d byte", udsc->rt_data_statis.sent_total_byte);
    log_i("Recv byte total     : %d byte", udsc->rt_data_statis.recv_total_byte);
    log_i("Sent peak speed     : %d byte/s", udsc->rt_data_statis.sent_peak_speed);
    log_i("Recv peak speed     : %d byte/s", udsc->rt_data_statis.recv_peak_speed);
    log_i("Sent valley speed   : %d byte/s", udsc->rt_data_statis.sent_valley_speed);
    log_i("Recv valley speed   : %d byte/s", udsc->rt_data_statis.recv_valley_speed);
    log_i("Sent message number : %d", udsc->rt_data_statis.sent_num);
    log_i("Recv message number : %d", udsc->rt_data_statis.recv_num);
    log_i("Runtime total       : %d ms", udsc->rt_data_statis.run_time_total);
    
    return true;
}

/*
    ��ʼִ����Ϸ�����, ��������һ��ʹ�ܵ���Ϸ�����󷵻�true
*/
BOOLEAN yudsc_services_start(uds_client *udsc)
{
    BOOLEAN start_succ = false;
    unsigned int tv_sec = 0;
    unsigned int tv_usec = 0;
    int ii = 0;

    if (udsc_service_isactive(udsc)) {
        log_i("The UDS client is active and cannot be started repeatedly");
        return start_succ;
    }
    udsc_common_var_reset(udsc); /* ���ò������� */    

    /* ���֮ǰ�������Ӧ���� */
    for (ii = 0; ii < udsc->service_cnt && udsc->service_items[ii]; ii++) {
        if (udsc->service_items[ii]->response_byte) {
            YByteArrayDelete(udsc->service_items[ii]->response_byte);
            udsc->service_items[ii]->response_byte = NULL;
        }
        udsc->service_items[ii]->status = SERVICE_ITEM_WAIT;
    }

    udsc->sindex = 0; /* ��ʼ��������0��ʼ */
    while (udsc->sindex < udsc->service_cnt) {
        service_item *item = udsc->service_items[udsc->sindex];
        /* �жϵ�ǰ������Ƿ���Ҫִ�� */
        if (item && item->isenable) {
            /* ������ʱ��ʱ������ʼִ����Ϸ����� */
            udsc_service_request_delay_trigger(udsc);
            start_succ = true;
            break;
        }
        udsc->sindex++;
    }
    /* ��Ҫ������������������� */
    if (start_succ && udsc->tpEnable) {        
        udsc_service_tester_present_timer_start(udsc);
        if (udsc->isTpRefresh) {
            log_i("The tester present send cycle is set to diagnostic message refresh");
        }
    }
    /* ��ʼ��ͳ�Ʊ��� */
    if (udsc->runtime_statis_enable) {
        log_i("The UDS client running statistics start");
        memset(&udsc->rt_data_statis, 0, sizeof(udsc->rt_data_statis));
        udsc->rt_data_statis.recv_peak_speed = 0;
        udsc->rt_data_statis.recv_valley_speed = 0xffffffff;    
        udsc->rt_data_statis.sent_peak_speed = 0;
        udsc->rt_data_statis.sent_valley_speed = 0xffffffff;
        ydm_tv_get(&tv_sec, &tv_usec);
        udsc->run_start_time = tv_sec * 1000 + tv_usec / 1000; /* ת���ɺ��� */
        udsc->recv_pretime = tv_sec; 
        udsc->sent_pretime = tv_sec;
    }
    log_i("UDS client(%d) start runing", udsc->id);
    
    return start_succ;
}

/*
    ִֹͣ����Ϸ�����
*/
BOOLEAN yudsc_services_stop(uds_client *udsc)
{
    return udsc_services_stop(udsc);
}

/*
    ����һ���µ���Ϸ�����������������������ڵ�����
*/
service_item *yudsc_service_item_add(uds_client *udsc, char *desc)
{
    service_item *nitem = NULL;
    int desc_len = 0;

    if (!(udsc->service_cnt < udsc->service_size)) {
        /* �������б������޷�������� */
        log_w("The capacity of the service item has reached the maximum value => %d", udsc->service_size);
        return NULL;
    }
    nitem = ycalloc(sizeof(service_item), 1);
    if (nitem == NULL) {
        log_f("System call error(calloc)");
        return NULL;
    }
    memset(nitem, 0, sizeof(*nitem));
    /* ���򱣴浽������ */
    if (udsc->service_items[udsc->service_cnt] != NULL) {
        log_f("new service item added conflict");
        yfree(nitem);
        return NULL;
    }    
    udsc->service_items[udsc->service_cnt] = nitem;
    udsc->service_cnt++;
    nitem->isenable = true;
    nitem->tatype = PHYSICAL_ADDRESS;
    nitem->response_rule = NOT_SET_RESPONSE_EXPECT;
    nitem->finish_rule = FINISH_DEFAULT_SETTING;
    nitem->request_byte = YByteArrayNew();
    if (nitem->request_byte == NULL) {
        log_f("ByteArrayNew error");
    }
    nitem->expect_byte = YByteArrayNew();
    if (nitem->expect_byte == NULL) {
        log_f("ByteArrayNew error");
    }
    nitem->finish_byte = YByteArrayNew();
    if (nitem->finish_byte == NULL) {
        log_f("ByteArrayNew error");
    }
    nitem->variable_byte = YByteArrayNew();
    if (nitem->variable_byte == NULL) {
        log_f("ByteArrayNew error");
    }
    
    if (desc) {
        /* ���������ַ���s */
        nitem->desc = strdup(desc);
    }
    log_i("Add current service <%s> item count => %d", desc ? desc : " ", udsc->service_cnt);
    
    return nitem;
}

/*
    ɾ������Ϸ�����ᱻɾ����item������ʹ����
*/
void yudsc_service_item_del(uds_client *udsc, service_item *item)
{
    int ii = 0;
    BOOLEAN isfound = false;

    if (item == NULL) {
        return ;
    }

    for (ii = 0; udsc_service_item_index_valid(udsc, ii); ii++) {
        if (!isfound) {
            if (udsc->service_items[ii] == item) {
                isfound = true;
                log_d("Del item %d => %s", ii, item->desc);
                udsc_service_item_destroy(&item);
                udsc->service_items[ii] = udsc->service_items[ii + 1];
            }
        }
        else {
            log_d("move item %d ", ii);
            udsc->service_items[ii] = udsc->service_items[ii + 1];
        }
    }
    if (isfound) {
        udsc->service_cnt--;
    }
    log_i("Del current service %s item count: %d", item->desc ? item->desc : " ", udsc->service_cnt);

    return ;
 }

BOOLEAN yudsc_loop_stop(uds_client *udsc)
{
    udsc_services_stop(udsc);
        
    return true;
}

/* 
    ��ʼuds�ͻ��˵��߳�ѭ���������һ�ξͿ��� 
*/
BOOLEAN yudsc_thread_loop_start(uds_client *udsc)
{
    
    return true;
}

/* 
    ��ʼuds�ͻ��˵��¼�ѭ��������Ҫѭ�����õ�������������� 
*/
BOOLEAN yudsc_event_loop_start(uds_client *udsc)
{

    return true;
}

void yudsc_request_transfer_callback_set(uds_client *udsc, udsc_request_transfer_callback call, void *arg)
{
    udsc->udsc_request_transfer_cb = call;
    udsc->udsc_request_transfer_arg = arg;
}

void yudsc_task_end_callback_set(uds_client *udsc, udsc_task_end_callback call, void *arg)
{
    udsc->udsc_task_end_cb = call;
    udsc->udsc_task_end_arg = arg;
}

BOOLEAN yudsc_service_request_build(service_item *sitem)
{
    int n = 0;

    YByteArrayClear(sitem->request_byte);

    /* append sid */
    YByteArrayAppendChar(sitem->request_byte, sitem->sid);
    if (udsc_service_have_sub(sitem->sid)) {
        /* append subfunction */
        if (sitem->issuppress) {
            YByteArrayAppendChar(sitem->request_byte, sitem->sub | UDS_SUPPRESS_POS_RSP_MSG_IND_MASK);
        }
        else {
            YByteArrayAppendChar(sitem->request_byte, sitem->sub);
        }
    }

    /* append did */
    if (sitem->sid == UDS_SERVICES_RDBI || \
        sitem->sid == UDS_SERVICES_WDBI || \
        sitem->sid == UDS_SERVICES_RC) {
        YByteArrayAppendChar(sitem->request_byte, sitem->did >> 8 & 0xff);
        YByteArrayAppendChar(sitem->request_byte, sitem->did & 0xff);
    }
    else if (sitem->sid == UDS_SERVICES_RFT) {
        if (sitem->rft_38) {
            YByteArrayAppendChar(sitem->request_byte, sitem->rft_38->modeOfOperation);
            YByteArrayAppendChar(sitem->request_byte, sitem->rft_38->filePathAndNameLength >> 8 & 0xff);
            YByteArrayAppendChar(sitem->request_byte, sitem->rft_38->filePathAndNameLength & 0xff);
            if (sitem->rft_38->filePathAndName) {
                YByteArrayAppendNChar(sitem->request_byte, \
                    (unsigned char *)sitem->rft_38->filePathAndName, strlen(sitem->rft_38->filePathAndName));
            }
            YByteArrayAppendChar(sitem->request_byte, sitem->rft_38->dataFormatIdentifier);
            YByteArrayAppendChar(sitem->request_byte, sitem->rft_38->fileSizeParameterLength);
            for (n = sitem->rft_38->fileSizeParameterLength - 1; n >= 0; n--) {
                YByteArrayAppendChar(sitem->request_byte, (sitem->rft_38->fileSizeUnCompressed >> (8 * n)) & 0xff);
            }
            for (n = sitem->rft_38->fileSizeParameterLength - 1; n >= 0; n--) {
                YByteArrayAppendChar(sitem->request_byte, (sitem->rft_38->fileSizeCompressed >> (8 * n)) & 0xff);
            }
        }
    }
    else if (sitem->sid == UDS_SERVICES_RD) {
        YByteArrayAppendChar(sitem->request_byte, sitem->rd_34.dataFormatIdentifier);
        YByteArrayAppendChar(sitem->request_byte, sitem->rd_34.addressAndLengthFormatIdentifier);
        for (n = (sitem->rd_34.addressAndLengthFormatIdentifier >> 4 & 0x0f) - 1; n >= 0; n--) {
            YByteArrayAppendChar(sitem->request_byte, (sitem->rd_34.memoryAddress >> (8 * n)) & 0xff);
        }
        for (n = (sitem->rd_34.addressAndLengthFormatIdentifier & 0x0f) - 1; n >= 0; n--) {
            YByteArrayAppendChar(sitem->request_byte, (sitem->rd_34.memorySize >> (8 * n)) & 0xff);
        }
    }

    if (sitem->vb_callback) {
        sitem->vb_callback(sitem, sitem->variable_byte, sitem->vb_callback_arg);
    }
    
    /* append variable byte */
    YByteArrayAppendArray(sitem->request_byte, sitem->variable_byte);

    return true;
}

void yudsc_service_variable_byte_callback_set(service_item *sitem, service_variable_byte_callback vb_callback, void *arg)
{  
    sitem->vb_callback = vb_callback;
    sitem->vb_callback_arg = arg;
}

void yudsc_service_response_callback_set(service_item *sitem, service_response_callback response_callback, void *arg)
{   
    sitem->response_callback = response_callback;
    sitem->response_callback_arg = arg;
}

service_item *yudsc_curr_service_item(uds_client *udsc)
{
    if (!udsc_service_isactive(udsc)) {
        log_w("The UDS client is not started");
        return NULL;
    }

    return udsc_active_service_item(udsc);
}

void yudsc_service_sid_set(service_item *sitem, yuint8 sid)
{
    sitem->sid = sid;
}

yuint8 yudsc_service_sid_get(service_item *sitem)
{
    return sitem->sid;    
}

void yudsc_service_sub_set(service_item *sitem, yuint8 sub)
{
    sitem->sub = sub;
}

yuint8 yudsc_service_sub_get(service_item *sitem)
{
    return sitem->sub;
}

void yudsc_service_did_set(service_item *sitem, yuint16 did)
{
    sitem->did = did;
}

yuint16 yudsc_service_did_get(service_item *sitem)
{
    return sitem->did;
}

void yudsc_service_timeout_set(service_item *sitem, yint32 timeout)
{
    sitem->timeout = timeout;
    VAR_RANGE_LIMIT(sitem->timeout, UDS_SERVICE_TIMEOUT_MIN, UDS_SERVICE_TIMEOUT_MAX);
}

void yudsc_service_delay_set(service_item *sitem, yint32 delay)
{
    sitem->delay = delay;
    VAR_RANGE_LIMIT(sitem->delay, UDS_SERVICE_DELAY_MIN, UDS_SERVICE_DELAY_MAX);
}

void yudsc_service_suppress_set(service_item *sitem, BOOLEAN b)
{
    sitem->issuppress = b;
}

void yudsc_service_enable_set(service_item *sitem, BOOLEAN b)
{
    sitem->isenable = b;
}

void yudsc_service_expect_response_set(service_item *sitem, serviceResponseExpect rule, yuint8 *data, yuint32 size)
{
    int ii = 0;
    int hexlen = 0;
    char *hex = NULL;
    yuint8 binbuf[128] = {0};
    int binlen = 0;
    int arrcnt = 0;

    sitem->response_rule = rule;
    if (data && size > 0) {
        YByteArrayAppendNChar(sitem->expect_byte, data, size);
        for (ii = 0; ii < size; ii++) {
            if (ydm_common_is_hex_digit(data[ii])) {
                hexlen++;
                hex = hex ? hex : (char *)&data[ii];            
            }
            else {                
                if (hex && (binlen = ydm_common_nhex2bin(binbuf, hex, hexlen, sizeof(binbuf))) > 0) {
                    if (arrcnt < sizeof(sitem->expect_bytes) / sizeof(sitem->expect_bytes[0])) {
                        sitem->expect_bytes[arrcnt] = YByteArrayNew();
                        if (sitem->expect_bytes[arrcnt]) {
                            YByteArrayAppendNChar(sitem->expect_bytes[arrcnt], binbuf, binlen);                            
                            arrcnt++;
                            binlen = 0;
                            memset(binbuf, 0, sizeof(binbuf));
                        }
                    }
                }
                hexlen = 0;
                hex = NULL;
            }
        }
        if (hex && (binlen = ydm_common_nhex2bin(binbuf, hex, hexlen, sizeof(binbuf))) > 0) {
            log_hex_i("", binbuf, binlen);
            if (arrcnt < sizeof(sitem->expect_bytes) / sizeof(sitem->expect_bytes[0])) {
                sitem->expect_bytes[arrcnt] = YByteArrayNew();
                if (sitem->expect_bytes[arrcnt]) {
                    YByteArrayAppendNChar(sitem->expect_bytes[arrcnt], binbuf, binlen);                    
                    arrcnt++;
                    binlen = 0;
                    memset(binbuf, 0, sizeof(binbuf));                    
                }
            }
        }
    }
}

void yudsc_service_finish_byte_set(service_item *sitem, serviceFinishCondition rule, yuint8 *data, yuint32 size)
{
    int ii = 0;
    int hexlen = 0;
    char *hex = NULL;
    yuint8 binbuf[128] = {0};
    int binlen = 0;
    int arrcnt = 0;

    sitem->finish_rule = rule;
    if (data && size > 0) {
        YByteArrayAppendNChar(sitem->finish_byte, data, size);
        for (ii = 0; ii < size; ii++) {
            if (ydm_common_is_hex_digit(data[ii])) {
                hexlen++;
                hex = hex ? hex : (char *)&data[ii];            
            }
            else {                
                if (hex && (binlen = ydm_common_nhex2bin(binbuf, hex, hexlen, sizeof(binbuf))) > 0) {
                    if (arrcnt < sizeof(sitem->finish_bytes) / sizeof(sitem->finish_bytes[0])) {
                        sitem->finish_bytes[arrcnt] = YByteArrayNew();
                        if (sitem->finish_bytes[arrcnt]) {
                            YByteArrayAppendNChar(sitem->finish_bytes[arrcnt], binbuf, binlen);                            
                            arrcnt++;
                            binlen = 0;
                            memset(binbuf, 0, sizeof(binbuf));
                        }
                    }
                }
                hexlen = 0;
                hex = NULL;
            }
        }
        if (hex && (binlen = ydm_common_nhex2bin(binbuf, hex, hexlen, sizeof(binbuf))) > 0) {
            log_hex_i("", binbuf, binlen);
            if (arrcnt < sizeof(sitem->finish_bytes) / sizeof(sitem->finish_bytes[0])) {
                sitem->finish_bytes[arrcnt] = YByteArrayNew();
                if (sitem->finish_bytes[arrcnt]) {
                    YByteArrayAppendNChar(sitem->finish_bytes[arrcnt], binbuf, binlen);                    
                    arrcnt++;
                    binlen = 0;
                    memset(binbuf, 0, sizeof(binbuf));                    
                }
            }
        }
    }
}

void yudsc_service_key_set(service_item *sitem, YKey key_callback)
{
    sitem->ac_27.key_callback = key_callback;
}

void yudsc_service_key_generate(uds_client *udsc, yuint8 *key, yuint16 key_size)
{

}

void yudsc_security_access_keygen_callback_set(uds_client *udsc, security_access_keygen_callback call, void *arg)
{
    udsc->security_access_keygen_cb = call;
    udsc->security_access_keygen_arg = arg;
}

void yudsc_36_transfer_progress_callback_set(uds_client *udsc, service_36_transfer_progress_callback call, void *arg)
{
    udsc->service_36_transfer_progress_cb = call;
    udsc->service_36_transfer_progress_arg = arg;
}

void yudsc_doip_channel_bind(uds_client *udsc, void *doip_channel)
{
    udsc->doipc_channel = doip_channel;
}

void yudsc_doip_channel_unbind(uds_client *udsc)
{
    udsc->doipc_channel = 0;
}

void *yudsc_doip_channel(uds_client *udsc)
{
    return udsc->doipc_channel;
}
void yudsc_runtime_data_statis_enable(uds_client *udsc, BOOLEAN en)
{
    udsc->runtime_statis_enable = en;
}

BOOLEAN yudsc_service_item_capacity_init(uds_client *udsc, int size)
{
    return udsc_service_item_capacity_init(udsc, size);    
}

int yudsc_service_item_capacity(uds_client *udsc)
{
    return udsc->service_size;
}

int yudsc_security_access_key_set(uds_client *udsc, yuint8 *key, yuint16 key_size)
{
    service_item *item = NULL;

    if (key && udsc->common_var.key_byte) {
        YByteArrayAppendNChar(udsc->common_var.key_byte, key, key_size);
        item = udsc_active_service_item(udsc);
        /* �����ǰ��Ծ����27��key���󣬾�ֱ�Ӵ����������� */
        if (item) {
            if (item->sid == UDS_SERVICES_SA && item->sub % 2 == 0) {
                udsc_service_request_timer_now_trigger(udsc);
            }
        }
        return 0;
    }

    return -1;
}

int yudsc_tester_present_config(uds_client *udsc, yuint8 tpEnable, yuint8 isTpRefresh, yuint32 tpInterval, yuint32 tpta, yuint32 tpsa)
{
    udsc->tpEnable = tpEnable;
    udsc->isTpRefresh = isTpRefresh;
    udsc->tpInterval = tpInterval;
    udsc->tpta = tpta; 
    udsc->tpsa = tpsa;  

    return 0;
}

/* �����б��еķ�����ʧ���Ƿ���ֹ�������� */
int yudsc_fail_abort(uds_client *udsc, BOOLEAN b)
{
    udsc->isFailAbort = b;

    return 0;
}

int yudsc_id_set(uds_client *udsc, int id)
{
    udsc->id = id;

    return 0;
}

int yudsc_id(uds_client *udsc)
{
    return udsc->id;
}

int yudsc_idle_set(uds_client *udsc, BOOLEAN b)
{
    udsc->isidle = b;

    return 0;
}

BOOLEAN yudsc_is_idle(uds_client *udsc)
{
    return udsc->isidle;
}

int yudsc_td_36_progress_interval_set(uds_client *udsc, yuint32 interval)
{
    udsc->td_36_progress_interval = interval;
    VAR_RANGE_LIMIT(udsc->td_36_progress_interval, \
        TD_36_PROGRESS_NOTIFY_INTERVAL_MIN, \
        TD_36_PROGRESS_NOTIFY_INTERVAL_MAX);

    return 0;
}

int yudsc_name_set(uds_client *udsc, char *name)
{
    char udsc_name[64] = {0};

    if (name && strlen(name) > 0) {
        /* ignore */
    }
    else {
        snprintf(udsc_name, sizeof(udsc_name), "UC-%d", udsc->id);
        name = udsc_name;
    }
    
    if (udsc->udsc_name) {
        yfree(udsc->udsc_name);
        udsc->udsc_name = NULL;
    }
    udsc->udsc_name = strdup(name);

    return 0;
}

BOOLEAN yudsc_asc_record_enable(uds_client *udsc)
{
    return udsc->uds_asc_record_enable;
}

void yudsc_asc_record_enable_set(uds_client *udsc, BOOLEAN b)
{
    udsc->uds_asc_record_enable = b;
}

void yudsc_msg_parse_enable_set(uds_client *udsc, BOOLEAN b)
{
    udsc->uds_msg_parse_enable = b;
}
