#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "demo.h"
#include "yapi_dm_simple.h"

void YCALLBACK(uds_request_transfer)(YAPI_DM yapi, unsigned short udscid, const unsigned char *msg, unsigned int msg_len, unsigned int sa, unsigned int ta, unsigned int tatype);

int main(int argc, char *argv[])
{
    /* ����UDS�ͻ���API */
    YAPI_DM om_api = yapi_dm_create();
    if (om_api) {
        demo_printf("UDS client api create success. \n");
    }

    /* UDS�ͻ���API�����ɹ���λһ��UDS�ͻ��ˣ���ֹUDS�ͻ���API�쳣 */
    yapi_dm_master_reset(om_api);

    /* UDS�ͻ������ýṹ�� */
    udsc_create_config config;

    int udscid = yapi_dm_udsc_create(om_api, &config);
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
    service.sid = 0x22;
    /* DID $22 $2e $31�����ʹ�� */
    service.did = 0x1122;
    /* ��ʱ�೤ʱ���ִ�������Ϸ���unit ms */
    service.delay = 500;
    /* ������Ӧ��ʱʱ��unit ms */
    service.timeout = 100;
    /* ���һ��������� */
    if (yapi_dm_udsc_service_add(om_api, udscid, &service, 0) == 0) {
        demo_printf("UDS service config success. \n");
    }
    /* UDS��������ӽ������ͷ��ڴ� */
    yapi_byte_array_delete(service.variableByte);
    yapi_byte_array_delete(service.expectResponByte);
    yapi_byte_array_delete(service.finishByte);

    /* ================================================================= */
    /* ----------------------------------------------------------------- */
    int udscid2 = yapi_dm_udsc_create(om_api, &config);
    if (udscid2 >= 0) {
       demo_printf("UDS client create success => %d \n", udscid2);
    }

    memset(&service, 0, sizeof(service));
    /* ��Ϸ����еĿɱ����ݣ�UDS �ͻ��˽����� sid sub did������Զ�����UDS�������� */
    service.variableByte = yapi_byte_array_new();
    /* Ԥ�������Ӧ���ݣ������жϵ�ǰ��Ϸ���ִ���Ƿ����Ԥ�� */
    service.expectResponByte = yapi_byte_array_new();
    /* ��Ӧ����ƥ�����ݣ������жϵ�ǰ��Ϸ����Ƿ���Ҫ�ظ�ִ�� */
    service.finishByte = yapi_byte_array_new();
    /* �������Դ��ַ */
    service.sa = 0x77777777;
    /* �������Ŀ�ĵ�ַ */
    service.ta = 0x88888888;
    /* ��Ϸ���ID */
    service.sid = 0x22;
    /* DID $22 $2e $31�����ʹ�� */
    service.did = 0x1122;
    /* ��ʱ�೤ʱ���ִ�������Ϸ���unit ms */
    service.delay = 500;
    /* ������Ӧ��ʱʱ��unit ms */
    service.timeout = 100;
    /* ���һ��������� */
    if (yapi_dm_udsc_service_add(om_api, udscid2, &service, 0) == 0) {
        demo_printf("UDS service config success. \n");
    }
    /* UDS��������ӽ������ͷ��ڴ� */
    yapi_byte_array_delete(service.variableByte);
    yapi_byte_array_delete(service.expectResponByte);
    yapi_byte_array_delete(service.finishByte);

    udsc_runtime_config gconfig;
    memset(&gconfig, 0, sizeof(gconfig));
    /* M��������Ǳ�����õ� uds�ͻ���ͨ������ */
    yapi_dm_udsc_runtime_config(om_api, udscid2, &gconfig);
    
    /* M��������Ǳ�����õ� ����OTA MASTER����������Ĳ�ͨ��doIP����doCAN���� */    
    yevent_connect(om_api, udscid2, YEVENT(udsc_request_transfer), YCALLBACK(uds_request_transfer));
    /* M��������Ǳ�����õ� ����UDS�ͻ��˿�ʼִ��������� */
    yapi_dm_udsc_start(om_api, udscid2);
    
    /* ----------------------------------------------------------------- */
    /* ================================================================= */

    //udsc_runtime_config gconfig;
    memset(&gconfig, 0, sizeof(gconfig));
    /* M��������Ǳ�����õ� uds�ͻ���ͨ������ */
    yapi_dm_udsc_runtime_config(om_api, udscid, &gconfig);
    
    /* M��������Ǳ�����õ� ����OTA MASTER����������Ĳ�ͨ��doIP����doCAN���� */
    yevent_connect(om_api, udscid, YEVENT(udsc_request_transfer), YCALLBACK(uds_request_transfer));
    
    /* M��������Ǳ�����õ� ����UDS�ͻ��˿�ʼִ��������� */
    yapi_dm_udsc_start(om_api, udscid);

    /* ������ȡOTA MASTER������Ϣ */
    while (1) {
        fd_set readfds;
        struct timeval timeout;
        int sret = 0;
        int sockfd = yapi_dm_ipc_channel_fd(om_api);
    
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000;
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

void YCALLBACK(uds_request_transfer)(YAPI_DM yapi, unsigned short udscid, const unsigned char *msg, unsigned int msg_len, unsigned int sa, unsigned int ta, unsigned int tatype)
{
    /* ================================================================= */
    /* ----------------------------------------------------------------- */
    demo_printf("udscid => %d \n", udscid);
    /* ----------------------------------------------------------------- */
    /* ================================================================= */
    demo_printf("UDS SA: 0x%08X TA: 0x%08X \n", sa, ta);
    demo_hex_printf("UDS Service Request", msg, msg_len);
    /* ���������ͨ��doCAN����doIP����send msg to CAN/IP */
    /* doIP_send_diag_request(msg, msg_len) */
    /* doCAN_send_diag_request(msg, msg_len) */
}
