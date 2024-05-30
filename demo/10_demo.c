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

int main(int argc, char *argv[])
{
    /* ����UDS�ͻ���API */
    YAPI_DM om_api = yapi_dm_create();
    if (om_api) {
        demo_printf("UDS client api create success. \n");
    }
    
    // ydm_debug_enable(1);

    /* UDS�ͻ���API�����ɹ���λһ��UDS�ͻ��ˣ���ֹUDS�ͻ���API�쳣 */
    yapi_dm_master_reset(om_api);

    while (1) {
        /* UDS�ͻ������ýṹ�� */
        udsc_create_config config;

        config.service_item_capacity = 100000;
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
        service.sid = 0x38;
        service.service_38_rft.mode_of_operation = 0x01;
        service.service_38_rft.file_path_and_name_length = 10;
        snprintf(service.service_38_rft.file_path_and_name, \
            sizeof(service.service_38_rft.file_path_and_name), "%s", "1111111111111111111111111");
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
        yapi_dm_udsc_destroy(om_api, udscid);
    }
    
    /* ������ȡOTA MASTER������Ϣ */
    while (1) {
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
