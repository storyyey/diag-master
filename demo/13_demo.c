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
void YCALLBACK(udsc_service_36_transfer_progress)(YAPI_DM yapi, unsigned short udscid, unsigned int file_size, unsigned int total_byte, unsigned int elapsed_time);

int main(int argc, char *argv[])
{
    /* ����UDS�ͻ���API */
    if (syapi_dm_create() == 0) {
        demo_printf("UDS client api create success. \n");
    }
    /* UDS�ͻ���API�����ɹ���λһ��UDS�ͻ��ˣ���ֹUDS�ͻ���API�쳣 */
    syapi_dm_master_reset();

    ydef_sudsc_create(ydef_udsc_cfg_service_item_capacity(1000)); 

    ydef_udsc_service_add_statement("Request Download");    
    ydef_udsc_service_base_set(0x34, 0x11223344, 0x55667788, ydef_udsc_timeout(100), \
                        ydef_udsc_34_rd_data_format_identifier(0), \
                        ydef_udsc_34_rd_address_and_length_format_identifier(0x44), \
                        ydef_udsc_34_rd_memory_address(1), \
                        ydef_udsc_34_rd_memory_size(1));
    ydef_udsc_service_file_local_path_set("./diag_master");
    ydef_udsc_service_add(syapi_dm(), syapi_dm_udsc(), 0);

    ydef_udsc_service_add_statement("Transfer Data");    
    ydef_udsc_service_base_set(0x36, 0x11223344, 0x55667788, ydef_udsc_timeout(100), ydef_udsc_max_number_of_block_length(300));
    ydef_udsc_service_file_local_path_set("./diag_master");
    ydef_udsc_service_add(syapi_dm(), syapi_dm_udsc(), 0);

    /* M��������Ǳ�����õ� uds�ͻ���ͨ������ */
    ydef_sudsc_runtime_config(ydef_udsc_runtime_fail_abort(1), \
                            ydef_udsc_runtime_tester_present_enable(1), \
                            ydef_udsc_runtime_td_36_notify_interval(1000), \ 
                            ydef_udsc_runtime_statis_enable(1));
    
    /* M��������Ǳ�����õ� ����OTA MASTER����������Ĳ�ͨ��doIP����doCAN���� */
    /* M��������Ǳ�����õ� ����OTA MASTER����������Ĳ�ͨ��doIP����doCAN���� */
    syevent_connect(YEVENT(udsc_request_transfer), YCALLBACK(uds_request_transfer));
    /* ================================================================= */
    /* ----------------------------------------------------------------- */
    /* ʹ��ota master���͹�������������key */
    syevent_connect(YEVENT(udsc_security_access_keygen), YCALLBACK(udsc_security_access_keygen));

    syevent_connect(YEVENT(udsc_task_end), YCALLBACK(udsc_task_end));

    syevent_connect(YEVENT(udsc_service_36_transfer_progress), YCALLBACK(udsc_service_36_transfer_progress));

    /* M��������Ǳ�����õ� ����UDS�ͻ��˿�ʼִ��������� */
    syapi_dm_udsc_start();

    /* ������ȡOTA MASTER������Ϣ */
    while (1) {
        sleep(1);
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
    syapi_dm_udsc_sa_key(level, "\x11\x22\x33\x44", 4);    
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
   // demo_printf("UDS SA: 0x%08X TA: 0x%08X \n", sa, ta);
   // demo_hex_printf("UDS Service Request", msg, msg_len);
    /* ���������ͨ��doCAN����doIP����send msg to CAN/IP */
    /* doIP_send_diag_request(msg, msg_len) */
    /* doCAN_send_diag_request(msg, msg_len) */

    /* ����ֻ��Ϊ�˷�����ʾ yapi_om_service_responseӦ�����յ����Ӧ���ʱ����� */
    /* ���յ������Ӧ�ˣ�����Ӧ���ݷ��͸�OTA MASTER */
    if (msg[0] == 0x34) {
       syapi_dm_service_response("\x74\x44\x11\x22\x33\x44", sizeof("\x74\x44\x11\x22\x33\x44") - 1, sa, ta, 0);
    }

    if (msg[0] == 0x36) {
       char remsg[2] = {0};
       
       remsg[0] = 0x36 | 0x40;
       remsg[1] = msg[1]; 
       syapi_dm_service_response(remsg, 2, sa, ta, 0);
    }
}

void YCALLBACK(udsc_task_end)(YAPI_DM yapi, unsigned short udscid, unsigned int rcode, const unsigned char *ind, unsigned int indl, const unsigned char *resp, unsigned int respl)
{
    demo_printf("------------ all service finish ------------ \n");
    runtime_data_statis_t statis;
    syapi_dm_udsc_rundata_statis(&statis);
    syapi_dm_udsc_start();
}

void YCALLBACK(udsc_service_36_transfer_progress)(YAPI_DM yapi, unsigned short udscid, unsigned int file_size, unsigned int total_byte, unsigned int elapsed_time)
{
    demo_printf("file_size => %d \n", file_size);
    demo_printf("total_byte => %d \n", total_byte);
    demo_printf("elapsed_time => %d \n", elapsed_time);
}


