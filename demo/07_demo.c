#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <winsock2.h>  
#include <ws2tcpip.h>
#endif /* _WIN32 */
#include <sys/types.h>
#ifdef __linux__
#include <sys/select.h>
#include <sys/socket.h>
#endif /* __linux__ */

#include "demo.h"
#include "yapi_dm_simple.h"

void YCALLBACK(udsc_security_access_keygen)(YAPI_DM yapi, unsigned short udscid, unsigned char level, const unsigned char *data, unsigned int size);
void YCALLBACK(uds_request_transfer)(YAPI_DM yapi, unsigned short udscid, const unsigned char *msg, unsigned int msg_len, unsigned int sa, unsigned int ta, unsigned int tatype);
void yuds_request_seed_result_callback(void *arg, unsigned short id, const unsigned char *data, unsigned int size);
void YCALLBACK(udsc_task_end)(YAPI_DM yapi, unsigned short udscid, unsigned int rcode, const unsigned char *ind, unsigned int indl, const unsigned char *resp, unsigned int respl);

int main(int argc, char *argv[])
{
    /* ����UDS�ͻ���API */
    YAPI_DM om_api = yapi_dm_create();
    if (om_api) {
        demo_printf("UDS client api create success. \n");
    }

    sleep(1);

    // ydm_debug_enable(1);

    /* UDS�ͻ���API�����ɹ���λһ��UDS�ͻ��ˣ���ֹUDS�ͻ���API�쳣 */
    yapi_dm_master_reset(om_api);

    int udscid = 0;
    udsc_create_config config;

    int ii = 0;
    for (ii = 0; ii < 20; ii++) {
        /* UDS�ͻ������ýṹ�� */
        config.service_item_capacity = 200;
        udscid = yapi_dm_udsc_create(om_api, &config);
        if (udscid >= 0) {
           demo_printf("UDS client create success => %d \n", udscid);
        }
    }
    for (ii = 0; ii < 20; ii++) {
        if (yapi_dm_udsc_destroy(om_api, ii) == 0) {
           demo_printf("UDS client destroy success => %d \n", ii);
        }
    }
    
    udscid = yapi_dm_udsc_create(om_api, &config);
    if (udscid >= 0) {
       demo_printf("UDS client create success => %d \n", udscid);
    }

    /* UDS�������ýṹ�� */
    service_config service;

    memset(&service, 0, sizeof(service));
    /* ��Ϸ����еĿɱ����ݣ�UDS �ͻ��˽����� sid sub did������Զ�����UDS�������� */
    service.variableByte = yapi_byte_array_new();
    /* Ԥ�������Ӧ���ݣ������жϵ�ǰ��Ϸ���ִ���Ƿ����Ԥ�� */
    service.expectResponByte = yapi_byte_array_new();
    /* ��Ӧ����ƥ�����ݣ������жϵ�ǰ��Ϸ����Ƿ���Ҫ�ظ�ִ�� */
    service.finishByte = yapi_byte_array_new();
    /* �������Դ��ַ */
    service.sa = 0x11223344;
    /* �������Ŀ�ĵ�ַ */
    service.ta = 0x55667788;
    /* ��Ϸ���ID */
    service.sid = 0x27;
    service.sub = 0x09;
    /* ��ʱ�೤ʱ���ִ�������Ϸ���unit ms */
    service.delay = 500;
    /* ������Ӧ��ʱʱ��unit ms */
    service.timeout = 100;
    /* ע�������Ӧ��������ע�������������OTA MASTER�Ὣ�յ��������Ӧת��������������� */
    /* ���һ��������� */
    if (yapi_dm_udsc_service_add(om_api, udscid, &service, yuds_request_seed_result_callback) == 0) {
        demo_printf("UDS service config success. \n");
    }

    /* �������Դ��ַ */
    service.sa = 0x11223344;
    /* �������Ŀ�ĵ�ַ */
    service.ta = 0x55667788;
    /* ��Ϸ���ID */
    service.sid = 0x27;
    service.sub = 0x0a;
    /* ��ʱ�೤ʱ���ִ�������Ϸ���unit ms */
    service.delay = 500;
    /* ������Ӧ��ʱʱ��unit ms */
    service.timeout = 100;
    service.rr_callid = 0;
    /* ���һ��������� */
    if (yapi_dm_udsc_service_add(om_api, udscid, &service, 0) == 0) {
        demo_printf("UDS service config success. \n");
    }
    
    /* UDS��������ӽ������ͷ��ڴ� */
    yapi_byte_array_delete(service.variableByte);
    yapi_byte_array_delete(service.expectResponByte);
    yapi_byte_array_delete(service.finishByte);
    
    udsc_runtime_config gconfig;
    memset(&gconfig, 0, sizeof(gconfig));
    /* M��������Ǳ�����õ� uds�ͻ���ͨ������ */
    yapi_dm_udsc_runtime_config(om_api, udscid, &gconfig);
    
    /* M��������Ǳ�����õ� ����OTA MASTER����������Ĳ�ͨ��doIP����doCAN���� */
    yevent_connect(om_api, udscid, YEVENT(udsc_request_transfer), YCALLBACK(uds_request_transfer));
    /* ================================================================= */
    /* ----------------------------------------------------------------- */
    /* ʹ��ota master���͹�������������key */
    yevent_connect(om_api, udscid, YEVENT(udsc_security_access_keygen), YCALLBACK(udsc_security_access_keygen));
    /* ----------------------------------------------------------------- */
    /* ================================================================= */    
    /* M��������Ǳ�����õ� ����UDS�ͻ��˿�ʼִ��������� */
    yapi_dm_udsc_start(om_api, udscid);

    /* ������ȡOTA MASTER������Ϣ */
    while (1) {
        continue;
        fd_set readfds;
        struct timeval timeout;
        int sret = 0;
        int sockfd = yapi_dm_ipc_channel_fd(om_api);
    
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000 * 100;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        if ((sret = select(sockfd + 1, &readfds, NULL, NULL, &timeout)) > 0 &&\
            FD_ISSET(sockfd, &readfds)) {
            /* M��������Ǳ�����õĽ���OTA MASTER���̼�ͨ������ */
            yapi_dm_event_loop(om_api);
        }
    }
    
    return 0;
}

void YCALLBACK(udsc_security_access_keygen)(YAPI_DM yapi, unsigned short udscid, unsigned char level, const unsigned char *seed, unsigned int seed_size)
{
    /* ================================================================= */
    /* ----------------------------------------------------------------- */
    demo_printf("Seed level => %02X \n", level);
    demo_hex_printf("UDS Service seed", seed, seed_size);
    /* seed_generation_key(seed, seed_size) */
    /* ����ͨ����������KEY,�ٽ�KEY���͸�ota master */
    yapi_dm_udsc_sa_key(yapi, udscid, level, "\x11\x22\x33\x44", 4);    
    /* ----------------------------------------------------------------- */
    /* ================================================================= */    
}

void yuds_request_seed_result_callback(void *arg, unsigned short id, const unsigned char *data, unsigned int size)
{
    /* ������յ�������UDS��Ӧ���ģ����û������������Ӧ���ĵĻ����ﲻ�ᴥ������������ʱ֮��� */
    demo_hex_printf("UDS Service Response", data, size);    
}

void YCALLBACK(uds_request_transfer)(YAPI_DM yapi, unsigned short udscid, const unsigned char *msg, unsigned int msg_len, unsigned int sa, unsigned int ta, unsigned int tatype)
{
    demo_printf("UDS SA: 0x%08X TA: 0x%08X \n", sa, ta);
    demo_hex_printf("UDS Service Request", msg, msg_len);
    /* ���������ͨ��doCAN����doIP����send msg to CAN/IP */
    /* doIP_send_diag_request(msg, msg_len) */
    /* doCAN_send_diag_request(msg, msg_len) */

    /* ����ֻ��Ϊ�˷�����ʾ yapi_om_service_responseӦ�����յ����Ӧ���ʱ����� */
    /* ���յ������Ӧ�ˣ�����Ӧ���ݷ��͸�OTA MASTER */
    if (msg[0] == 0x27 && msg[1] == 0x09) {
       yapi_dm_service_response(yapi, udscid, "\x67\x09\x93\x02\x00\x11", sizeof("\x67\x09\x93\x02\x00\x11") - 1, sa, ta, 0);
    }
    else if (msg[0] == 0x27 && msg[1] == 0x0a) {
       yapi_dm_service_response(yapi, udscid, "\x67\x0a", sizeof("\x67\x0a") - 1, sa, ta, 0);
    }
}


