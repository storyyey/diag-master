#ifndef __YAPI_DM_SIMPLE_H__
#define __YAPI_DM_SIMPLE_H__

#include "ycommon_type.h"

typedef void* YAPI_DM;

#define YEVENT(name) name##_yevent
#define YCALLBACK(name) name##_ycallback

/*  
 * ��UDS�ͻ�����Ҫ���������������ʱ��������ô˻ص�������  
 * ����ص�������ota master api������ʹ�ã�����ͨ��onip��oncan�����������  
 * @param arg �û�����
 * @param udscid uds�ͻ���ID
 * @param data �����Ϣ
 * @param size �����Ϣ����
 * @param sa �����ϢԴ��ַ
 * @param ta �����ϢĿ�ĵ�ַ
 * @param tatype Ŀ�ĵ�ַ���� phy/func
 */
void YEVENT(udsc_request_transfer)(YAPI_DM yapi, yuint16 udscid, const yuint8 *data, yuint32 size, yuint32 sa, yuint32 ta, yuint32 tatype);

/*  
 * ������UDS����ִ�н�������������µ������жϣ�����ô˻ص�������  
 * �˻ص���������֪ͨ����ִ�еĽ��״̬�� 
 * @param arg �û�����
 * @param udscid uds�ͻ���ID
 * @param rcode ���״̬�� 0��������������
 * @param ind ���һ�����������Ϣ
 * @param indl ���һ�����������Ϣ����
 * @param resp ���һ�������Ӧ��Ϣ
 * @param respl ���һ�������Ӧ��Ϣ����
 */
void YEVENT(udsc_task_end)(YAPI_DM yapi, yuint16 udscid, yuint32 rcode, const yuint8 *ind, yuint32 indl, const yuint8 *resp, yuint32 respl);

/*  
 * ����UDS����ִ�н����󣬻���ô˻ص�������  
 * �˻ص���������֪ͨ����UDS����ִ�еĽ��״̬�� 
 * @param arg �û�����
 * @param udscid uds�ͻ���ID
 * @param data �����Ӧ��Ϣ
 * @param size �����Ӧ��Ϣ����
 */
typedef void (*uds_service_result_callback)(YAPI_DM yapi, yuint16 udscid, const yuint8 *data, yuint32 size);
typedef void (*uds_service_result_scallback)(const yuint8 *data, yuint32 size);

/*  
 * ��27�����ȡ��seed�󣬻�ص��˺�����  
 * ota master api������ʹ�ô˺���������Կ����ʹ��yapi_om_udsc_sa_key����������Ӧ����Կ��  
 * @param arg �û�����
 * @param udscid uds�ͻ���ID
 * @param level ��ȫ�ȼ���Ҳ������������ʱ���ӹ���
 * @param seed ����
 * @param seed_size ���ӳ���
 */
void YEVENT(udsc_security_access_keygen)(YAPI_DM yapi, yuint16 udscid, yuint8 level, const yuint8 *seed, yuint32 seed_size);

void YEVENT(udsc_service_36_transfer_progress)(YAPI_DM yapi, yuint16 udscid, yuint32 file_size, yuint32 total_byte, yuint32 elapsed_time);

void YEVENT(diag_master_instance_destroy)(YAPI_DM yapi);

/* �����¼���ص����� */
void yevent_connect(YAPI_DM yapi, yuint16 udscid, void *event, void *callback);
void syevent_connect(void *event, void *callback);
/*
    ��Щ�ӿ����ڿ���OTA Master ������Ӧ����
*/
/* ---------- OTA master api ���еĽӿھ�Ϊͬ���ӿڣ������� --------- */

/* ��ȡapi���ó���ʱ�Ĵ����� */
extern int yapi_dm_errcode(YAPI_DM yapi);

/* ��ȡ���������� */
extern char *yapi_dm_errcode_str(int errcode);

/* �򿪻��߹رյ�����ϢĬ�Ϲر� */
extern void yapi_dm_debug_enable(int eb);

/* ������ʼ��OTA MASTER API */
extern YAPI_DM yapi_dm_create();
extern int syapi_dm_create();
/* ��ȡĬ�ϵ�ota master api */
extern YAPI_DM syapi_dm(); 

/* �ж�OTA MASTER API�Ƿ���Ч����true��ʾ��Ч */
extern int yapi_dm_is_valid(YAPI_DM yapi);
extern int syapi_dm_is_valid();

/* ����OTA MASTER API */
extern void yapi_dm_destroy(YAPI_DM yapi);
extern void syapi_dm_destroy();

/* ��ȡ��OTA MASTER���̼�ͨ�ŵ�socket���������ڼ����Ƿ��н���ͨ������ */
extern int yapi_dm_ipc_channel_fd(YAPI_DM yapi);
extern int syapi_dm_ipc_channel_fd();

/* ����OTA MASTER����һ��UDS�ͻ��ˣ��ɹ����طǸ���UDS�ͻ���ID��ʧ�ܷ���-1 */
extern int yapi_dm_udsc_create(YAPI_DM yapi, udsc_create_config *config);
extern int syapi_dm_udsc_create(udsc_create_config *config);
/* ��ȡĬ�ϵ�uds�ͻ���ID */
extern int syapi_dm_udsc();

/* ����OTA MASTER����һ��UDS�ͻ��� */
extern int yapi_dm_udsc_destroy(YAPI_DM yapi, yuint16 udscid);
extern int syapi_dm_udsc_destroy();

/* ����OTA MASTER����һ��DOIP�ͻ��ˣ�����UDS�ͻ�����DOIP�ͻ��˰�,
   �ɹ����طǸ���UDS�ͻ���ID��UDS�ͻ������ٵ�ͬʱ������DOIP�ͻ��ˣ�ʧ�ܷ���-1 */
extern int yapi_dm_doipc_create(YAPI_DM yapi, yuint16 udscid, doipc_config_t *config);
extern int syapi_dm_doipc_create(doipc_config_t *config);

/*  ����OTA MASTER����UDS �ͻ��˿�ʼִ��������� */
extern int yapi_dm_udsc_start(YAPI_DM yapi, yuint16 udscid);
extern int syapi_dm_udsc_start();

/* ��ȡָ��UDS�ͻ����Ƿ�����ִ�����񷵻�true��ʾ��������ִ���� */
extern int yapi_dm_udsc_is_active(YAPI_DM yapi, yuint16 udscid);
extern int syapi_dm_udsc_is_active();

/*  ����OTA MASTER��ֹUDS �ͻ���ִ��������� */
extern int yapi_dm_udsc_stop(YAPI_DM yapi, yuint16 udscid);  
extern int syapi_dm_udsc_stop();

/* ��λOTA MASTER��ֹOTA MASTER�����쳣���� */
extern int yapi_dm_master_reset(YAPI_DM yapi);
extern int syapi_dm_master_reset();

/* ����OTA MASTER�е�UDS���ˢд���� */
extern int yapi_dm_udsc_service_add(YAPI_DM yapi, yuint16 udscid, service_config *config, uds_service_result_callback call);
extern int syapi_dm_udsc_service_add(service_config *config, uds_service_result_scallback call);

/* UDS�ͻ��˵�һЩͨ������  ����3E��������ִ�ж��������� */
extern int yapi_dm_udsc_runtime_config(YAPI_DM yapi, yuint16 udscid, udsc_runtime_config *config);
extern int syapi_dm_udsc_runtime_config(udsc_runtime_config *config);

/* �����ɵ�key���͸�ota master */
extern int yapi_dm_udsc_sa_key(YAPI_DM yapi, yuint16 udscid, yuint8 level, const yuint8 *key, yuint32 key_size);
extern int syapi_dm_udsc_sa_key(yuint8 level, const yuint8 *key, yuint32 key_size);

/* UDS�������Ӧ���ķ��͸�OTA MASTER�е�UDS�ͻ��˴��� */
extern int yapi_dm_service_response(YAPI_DM yapi, yuint16 udscid, const yuint8 *data, yuint32 size, yuint32 sa, yuint32 ta, yuint32 tatype);
extern int syapi_dm_service_response(const yuint8 *data, yuint32 size, yuint32 sa, yuint32 ta, yuint32 tatype);
  
/* ��������������ӦID/��ַ */  
extern int yapi_dm_udsc_diag_id_storage(YAPI_DM yapi, yuint16 udscid, yuint32 diag_req_id, yuint32 diag_resp_id);  
extern int syapi_dm_udsc_diag_id_storage(yuint32 diag_req_id, yuint32 diag_resp_id);

/* ͨ�������ӦID/��ַ����UDS�ͻ���ID */  
extern int yapi_dm_udsc_index_by_resp_id(YAPI_DM yapi, yuint32 diag_resp_id);
extern int syapi_dm_udsc_index_by_resp_id(yuint32 diag_resp_id);
  
/* ͨ���������ID/��ַ����UDS�ͻ���ID */  
extern int yapi_dm_udsc_index_by_req_id(YAPI_DM yapi, yuint32 diag_req_id);
extern int syapi_dm_udsc_index_by_req_id(yuint32 diag_req_id);
  
/* ͨ��UDS�ͻ���ID���������ӦID/��ַ */  
extern yuint32 yapi_dm_udsc_resp_id(YAPI_DM yapi, yuint16 udscid);
extern yuint32 syapi_dm_udsc_resp_id();
  
/* ͨ��UDS�ͻ���ID�����������ID/��ַ */  
extern yuint32 yapi_dm_udsc_req_id(YAPI_DM yapi, yuint16 udscid);  
extern yuint32 syapi_dm_udsc_req_id();

/* ��ȡUDS�ͻ����������� */
extern int yapi_dm_udsc_rundata_statis(YAPI_DM yapi, yuint16 udscid, runtime_data_statis_t *statis);
extern int syapi_dm_udsc_rundata_statis(runtime_data_statis_t *statis);

/* ���ں�OTA MASTER���н��̼�ͨ�Ž����yapi_om_sockfd() ��select epoll �������ʹ�� */  
extern void yapi_dm_event_loop(YAPI_DM yapi);
extern void syapi_dm_event_loop();

// ����һ���µ�YByteArray���󲢷�����ָ��  
extern ybyte_array *yapi_byte_array_new();  
  
// �ͷ�YByteArray������ռ�õ��ڴ�  
extern void yapi_byte_array_delete(ybyte_array *arr);  
  
// ���YByteArray���������  
extern void yapi_byte_array_clear(ybyte_array *arr);  
  
// ����YByteArray����ĳ�������ָ��  
extern const yuint8 *yapi_byte_array_const_data(ybyte_array *arr);  
  
// ����YByteArray�����е�Ԫ������  
extern int yapi_byte_array_count(ybyte_array *arr);  
  
// ��YByteArray����ĩβ���һ���ַ�  
extern void yapi_byte_array_append_char(ybyte_array *dest, yuint8 c);  
  
// ��һ��YByteArray���������׷�ӵ���һ��YByteArray������  
extern void yapi_byte_array_append_array(ybyte_array *dest, ybyte_array *src);  
  
// ��YByteArray����ĩβ׷��һ���ַ����������  
extern void yapi_byte_array_append_nchar(ybyte_array *dest, yuint8 *c, yuint32 count);  
  
// �Ƚ�����YByteArray�����Ƿ����  
extern int yapi_byte_array_equal(ybyte_array *arr1, ybyte_array *arr2);  
  
// �Ƚ�һ��YByteArray�����һ���ַ�����������Ƿ����  
extern int yapi_byte_array_char_equal(ybyte_array *arr1, yuint8 *c, yuint32 count);

enum yapi_log_level {
    YAPI_LOG_DEBUG,
    YAPI_LOG_INFO,
    YAPI_LOG_WARNING,
    YAPI_LOG_ERROR,
    YAPI_LOG_ALL,
};
extern int yapi_om_log_print(enum yapi_log_level level, int (*printf)(const char *format, ...));

/* ------------------------------------------------------------ */
/* ��ݵĴ���UDS�ͻ��� */
#define udsc_cfg_service_item_capacity_(var) .service_item_capacity = (var)
#define udsc_cfg_udsc_name_(var) .udsc_name = (var)
#define udsc_create_(yapi, udscid, ...) \
    do {\
        udsc_create_config __udsc_cfg__; \
        __udsc_cfg__ = (udsc_create_config){.padding = 0, __VA_ARGS__}; \
        (udscid) = yapi_dm_udsc_create(yapi, &__udsc_cfg__); \
    } while (0)
    
#define sudsc_create_(...) \
    do {\
        udsc_create_config __udsc_cfg__; \
        __udsc_cfg__ = (udsc_create_config){.padding = 0, __VA_ARGS__}; \
        syapi_dm_udsc_create(&__udsc_cfg__); \
    } while (0)
/* ------------------------------------------------------------ */
/* ------------------------------------------------------------ */
/* ��ݵ�����UDS�ͻ��� */
#define udsc_runtime_fail_abort_(var) .isFailAbort = (var)
#define udsc_runtime_tester_present_enable_(var) .tester_present_enable = (var)
#define udsc_runtime_tester_present_refresh_(var) .is_tester_present_refresh = (var)
#define udsc_runtime_tester_present_interval_(var) .tester_present_interval = (var)
#define udsc_runtime_tester_present_ta_(var) .tester_present_ta = (var)    
#define udsc_runtime_tester_present_sa_(var) .tester_present_ta = (var)
#define udsc_runtime_td_36_notify_interval_(var) .td_36_notify_interval = (var)
#define udsc_runtime_statis_enable_(var) .runtime_statis_enable = (var)
#define udsc_runtime_uds_msg_parse_enable_(var) .uds_msg_parse_enable = (var)
#define udsc_runtime_uds_asc_record_enable_(var) .uds_asc_record_enable = (var)
#define udsc_runtime_config_(yapi, udscid, ...) \
    do {\
        udsc_runtime_config __udsc_runtimel_cfg__; \
        __udsc_runtimel_cfg__ = (udsc_runtime_config){.padding = 0, __VA_ARGS__}; \
        yapi_dm_udsc_runtime_config(yapi, udscid, &__udsc_runtimel_cfg__); \
    } while (0)
    
#define sudsc_runtime_config_(...) \
    do {\
        udsc_runtime_config __udsc_runtimel_cfg__; \
        __udsc_runtimel_cfg__ = (udsc_runtime_config){.padding = 0, __VA_ARGS__}; \
        syapi_dm_udsc_runtime_config(&__udsc_runtimel_cfg__); \
    } while (0)
/* ------------------------------------------------------------ */

/* ��ݵ�������Ϸ����� */
#define udsc_service_add_statement_(des) \
    do { \
        service_config __service_cfg__; \
        char *__cfg_des__ = des; \
        int __cfg_priority__ = 0; \
        memset(&__service_cfg__, 0, sizeof(__service_cfg__));
/* ------------------------------------------------------------ */
#define udsc_sub_(var) .sub = (var)
#define udsc_did_(var) .did = (var)
#define udsc_delay_(var) .delay = (var)
#define udsc_timeout_(var) .timeout = (var)
#define udsc_suppress_(var) .issuppress = (var)
#define udsc_tatype_(var) .tatype = (var)
#define udsc_expect_respon_rule_(var) .expectRespon_rule = (var)
#define udsc_finish_rule_(var) .finish_rule = (var)
#define udsc_finish_num_max_(var) .finish_num_max = (var)
#define udsc_34_rd_data_format_identifier_(var) .service_34_rd.data_format_identifier = (var)
#define udsc_34_rd_address_and_length_format_identifier_(var) .service_34_rd.address_and_length_format_identifier = (var)
#define udsc_34_rd_memory_address_(var) .service_34_rd.memory_address = (var)
#define udsc_34_rd_memory_size_(var) .service_34_rd.memory_size = (var)
#define udsc_38_rft_mode_of_operation_(var) .service_38_rft.mode_of_operation = (var)
#define udsc_38_rft_file_path_and_name_length_(var) .service_38_rft.file_path_and_name_length = (var)
#define udsc_38_rft_data_format_identifier_(var) .service_38_rft.data_format_identifier = (var)
#define udsc_38_rft_file_size_parameter_length_(var) .service_38_rft.file_size_parameter_length = (var)
#define udsc_38_rft_file_size_un_compressed_(var) .service_38_rft.file_size_uncompressed = (var)
#define udsc_38_rft_file_size_compressed_(var) .service_38_rft.file_size_compressed = (var)
#define udsc_max_number_of_block_length_(var) .max_number_of_block_length = (var)
/* ------------------------------------------------------------ */
#define udsc_service_base_set_(_sid, _sa, _ta, ...) \
        ++__cfg_priority__; \
        __service_cfg__ = (service_config){ \
                                            .sid = (_sid), \
                                            .sa = (_sa), \
                                            .ta = (_ta), \
                                            .variableByte = NULL, \
                                            .expectResponByte = NULL, \
                                            .finishByte = NULL, \
                                            __VA_ARGS__}; \
        __service_cfg__.variableByte = yapi_byte_array_new(); \
        __service_cfg__.expectResponByte = yapi_byte_array_new(); \
        __service_cfg__.finishByte = yapi_byte_array_new(); \
        if (__cfg_des__) { \
            snprintf((char *)__service_cfg__.desc, sizeof(__service_cfg__.desc), "%s", (char *)__cfg_des__); \
        }
#define udsc_service_38_rft_file_path_and_name_set_(var) \
        if ((var)) { \
            snprintf((char *)__service_cfg__.service_38_rft.file_path_and_name, \
                sizeof(__service_cfg__.service_38_rft.file_path_and_name), "%s", (char *)(var)); \
        } 
#define udsc_service_file_local_path_set_(var) \
        if ((var)) { \
            snprintf((char *)__service_cfg__.local_path, \
                sizeof(__service_cfg__.local_path), "%s", (char *)(var)); \
        }  
#define udsc_service_variable_byte_set_(d, size) \
        if (__cfg_priority__ > 0) { \
            yapi_byte_array_append_nchar(__service_cfg__.variableByte, (yuint8 *)(d), (yuint32)(size)); \
        }
#define udsc_service_expect_respon_byte_set_(d, size) \
        if (__cfg_priority__ > 0) { \
            yapi_byte_array_append_nchar(__service_cfg__.expectResponByte, (yuint8 *)(d), (yuint32)(size)); \
        }
#define udsc_service_finish_byte_set_(d, size) \
        if (__cfg_priority__ > 0) { \
            yapi_byte_array_append_nchar(__service_cfg__.finishByte, (yuint8 *)(d), (yuint32)(size)); \
        }
#define udsc_service_add_(om_api, udscid, call) \
        if (__cfg_priority__ > 0) { \
            yapi_dm_udsc_service_add((om_api), (udscid), &__service_cfg__, (call)); \
            yapi_byte_array_delete(__service_cfg__.variableByte); \
            yapi_byte_array_delete(__service_cfg__.expectResponByte); \
            yapi_byte_array_delete(__service_cfg__.finishByte); \
        } \
    } while (0 > 100)

/* ����Ϊ�˴��뿴��������һЩ�����������ٶ�������Щ�� */
#define ydef_udsc_cfg_service_item_capacity(var)                     udsc_cfg_service_item_capacity_(var)
#define ydef_udsc_cfg_udsc_name(var)                                 udsc_cfg_udsc_name_(var)
#define ydef_udsc_create(yapi, udscid, ...)                          udsc_create_(yapi, udscid, __VA_ARGS__)
#define ydef_sudsc_create(...)                                       sudsc_create_(__VA_ARGS__)

#define ydef_udsc_service_add_statement(des)                         udsc_service_add_statement_(des)
    #define ydef_udsc_sub(var)                                           udsc_sub_(var)
    #define ydef_udsc_did(var)                                           udsc_did_(var)
    #define ydef_udsc_delay(var)                                         udsc_delay_(var)
    #define ydef_udsc_timeout(var)                                       udsc_timeout_(var)
    #define ydef_udsc_suppress(var)                                      udsc_suppress_(var)
    #define ydef_udsc_tatype(var)                                        udsc_tatype_(var)
    #define ydef_udsc_expect_respon_rule(var)                            udsc_expect_respon_rule_(var)
    #define ydef_udsc_finish_rule(var, num)                              udsc_finish_rule_(var),udsc_finish_num_max_(num)
    #define ydef_udsc_34_rd_data_format_identifier(var)                  udsc_34_rd_data_format_identifier_(var)
    #define ydef_udsc_34_rd_address_and_length_format_identifier(var)    udsc_34_rd_address_and_length_format_identifier_(var)
    #define ydef_udsc_34_rd_memory_address(var)                          udsc_34_rd_memory_address_(var)
    #define ydef_udsc_34_rd_memory_size(var)                             udsc_34_rd_memory_size_(var)
    #define ydef_udsc_38_rft_mode_of_operation(var)                      udsc_38_rft_mode_of_operation_(var)
    #define ydef_udsc_38_rft_file_path_and_name_length(var)              udsc_38_rft_file_path_and_name_length_(var)
    #define ydef_udsc_38_rft_data_format_identifier(var)                 udsc_38_rft_data_format_identifier_(var)
    #define ydef_udsc_38_rft_file_size_parameter_length(var)             udsc_38_rft_file_size_parameter_length_(var)
    #define ydef_udsc_38_rft_file_size_un_compressed(var)                udsc_38_rft_file_size_un_compressed_(var)
    #define ydef_udsc_38_rft_file_size_compressed(var)                   udsc_38_rft_file_size_compressed_(var)
    #define ydef_udsc_max_number_of_block_length(var)                    udsc_max_number_of_block_length_(var)
#define ydef_udsc_service_base_set(_sid, _sa, _ta, ...)              udsc_service_base_set_(_sid, _sa, _ta, __VA_ARGS__)
#define ydef_udsc_service_38_rft_file_path_and_name_set(var)         udsc_service_38_rft_file_path_and_name_set_(var) 
#define ydef_udsc_service_file_local_path_set(var)                   udsc_service_file_local_path_set_(var)
#define ydef_udsc_service_variable_byte_set(d, size)                 udsc_service_variable_byte_set_(d, size)

/* ����ʮ�������ַ���,����������÷�ʮ�������ַ����ָ���磺7101334400/7101334401/7101334402 */
#define ydef_udsc_service_expect_respon_byte_set(d)            udsc_service_expect_respon_byte_set_(d, strlen((const char *)d))

/* ����ʮ�������ַ���,����������÷�ʮ�������ַ����ָ���磺7101334400/7101334401/7101334402 */
#define ydef_udsc_service_finish_byte_set(d)                   udsc_service_finish_byte_set_(d, strlen((const char *)d))
#define ydef_udsc_service_add(om_api, udscid, call)                  udsc_service_add_(om_api, udscid, call)

#define ydef_udsc_runtime_fail_abort(var)                            udsc_runtime_fail_abort_(var)
#define ydef_udsc_runtime_tester_present_enable(var)                 udsc_runtime_tester_present_enable_(var)
#define ydef_udsc_runtime_tester_present_refresh(var)                udsc_runtime_tester_present_refresh_(var)
#define ydef_udsc_runtime_tester_present_interval(var)               udsc_runtime_tester_present_interval_(var)
#define ydef_udsc_runtime_tester_present_ta(var)                     udsc_runtime_tester_present_ta_(var)    
#define ydef_udsc_runtime_tester_present_sa(var)                     udsc_runtime_tester_present_sa_(var)
#define ydef_udsc_runtime_td_36_notify_interval(var)                 udsc_runtime_td_36_notify_interval_(var)
#define ydef_udsc_runtime_statis_enable(var)                         udsc_runtime_statis_enable_(var)
#define ydef_udsc_runtime_uds_msg_parse_enable(var)                  udsc_runtime_uds_msg_parse_enable_(var)
#define ydef_udsc_runtime_uds_asc_record_enable(var)                 udsc_runtime_uds_asc_record_enable_(var)
#define ydef_udsc_runtime_config(yapi, udscid, ...)                  udsc_runtime_config_(yapi, udscid, __VA_ARGS__)
#define ydef_sudsc_runtime_config(...)                               sudsc_runtime_config_(__VA_ARGS__)
#endif /* __YAPI_DM_SIMPLE_H__ */
