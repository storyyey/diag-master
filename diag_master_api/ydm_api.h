#ifndef __YDM_API_H__
#define __YDM_API_H__

#include "ycommon_type.h"

struct ydiag_master_api;
typedef struct ydiag_master_api ydiag_master_api;

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
typedef void (*udsc_request_transfer_callback)(void *arg, yuint16 udscid, const yuint8 *data, yuint32 size, yuint32 sa, yuint32 ta, yuint32 tatype);

/*  
 * ������UDS����ִ�н�������������µ������жϣ�����ô˻ص�������  
 * �˻ص���������֪ͨ����ִ�еĽ��״̬�� 
 * @param arg �û�����
 * @param udscid uds�ͻ���ID
 * @param rcode ���״̬�� 0��������������
 * @param ind ���������Ϣ
 * @param indl ���������Ϣ����
 * @param resp �����Ӧ��Ϣ
 * @param respl �����Ӧ��Ϣ����
 */
typedef void (*udsc_task_end_callback)(void *arg, yuint16 udscid, yuint32 rcode, const yuint8 *ind, yuint32 indl, const yuint8 *resp, yuint32 respl);

/*  
 * ����UDS����ִ�н����󣬻���ô˻ص�������  
 * �˻ص���������֪ͨ����UDS����ִ�еĽ��״̬�� 
 * @param arg �û�����
 * @param udscid uds�ͻ���ID
 * @param data �����Ӧ��Ϣ
 * @param size �����Ӧ��Ϣ����
 */
typedef void (*uds_service_result_callback)(void *arg, yuint16 udscid, const yuint8 *data, yuint32 size);

/*  
 * ��27�����ȡ��seed�󣬻�ص��˺�����  
 * ota master api������ʹ�ô˺���������Կ����ʹ��yom_api_udsc_sa_key����������Ӧ����Կ��  
 * @param arg �û�����
 * @param udscid uds�ͻ���ID
 * @param level ��ȫ�ȼ���Ҳ������������ʱ���ӹ���
 * @param seed ����
 * @param seed_size ���ӳ���
 */
typedef void (*udsc_security_access_keygen_callback)(void *arg, yuint16 udscid, yuint8 level, const yuint8 *seed, yuint32 seed_size);

typedef void (*uds_service_36_transfer_progress_callback)(void *arg, yuint16 udscid, yuint32 file_size, yuint32 total_byte, yuint32 elapsed_time);

typedef void (*diag_master_instance_destroy_callback)(void *arg/* �û�����ָ�� */);

/*
    ��Щ�ӿ����ڿ���OTA Master ������Ӧ����
*/
/* ---------- DIAG master api ���еĽӿھ�Ϊͬ���ӿڣ������� --------- */

/* ��ȡapi���ó���ʱ�Ĵ����� */
extern int ydm_api_errcode(ydiag_master_api *dm_api);

/* ��ȡ���������� */
extern char *ydm_api_errcode_str(int errcode);

/* �򿪻��߹رյ�����ϢĬ�Ϲر� */
extern void ydm_debug_enable(int eb);

/* ������ʼ��DIAG MASTER API */
extern ydiag_master_api *ydm_api_create();

/* �ж�DIAG MASTER API�Ƿ���Ч����true��ʾ��Ч */
extern int ydm_api_is_valid(ydiag_master_api *dm_api);

/* ����DIAG MASTER API */
extern int ydm_api_destroy(ydiag_master_api *dm_api);

/* ��ȡ��DIAG MASTER���̼�ͨ�ŵ�socket���������ڼ����Ƿ��н���ͨ������ */
extern int ydm_api_sockfd(ydiag_master_api *oa_api);

/* ����DIAG MASTER����һ��UDS�ͻ��ˣ��ɹ����طǸ���UDS�ͻ���ID��ʧ�ܷ���-1 */
extern int ydm_api_udsc_create(ydiag_master_api *dm_api, udsc_create_config *config);

/* ����DIAG MASTER����һ��UDS�ͻ��� */
extern int ydm_api_udsc_destroy(ydiag_master_api *dm_api, yuint16 udscid);

/* ����DIAG MASTER����һ��DOIP�ͻ��ˣ�����UDS�ͻ�����DOIP�ͻ��˰�,
   �ɹ����طǸ���UDS�ͻ���ID��UDS�ͻ������ٵ�ͬʱ������DOIP�ͻ��ˣ�ʧ�ܷ���-1 */
extern int ydm_api_doipc_create(ydiag_master_api *dm_api, yuint16 udscid, doipc_config_t *config);

/* ����DIAG MASTER����UDS �ͻ��˿�ʼִ��������� */
extern int ydm_api_udsc_start(ydiag_master_api *dm_api, yuint16 udscid);

/* ��ȡָ��UDS�ͻ����Ƿ�����ִ�����񷵻�true��ʾ��������ִ���� */
extern int ydm_api_udsc_is_active(ydiag_master_api *dm_api, yuint16 udscid);

/*  ����DIAG MASTER��ֹUDS �ͻ���ִ��������� */
extern int ydm_api_udsc_stop(ydiag_master_api *dm_api, yuint16 udscid);    

/* ��λDIAG MASTER��ֹDIAG MASTER�����쳣���� */
extern int ydm_api_master_reset(ydiag_master_api *dm_api);

/* ����DIAG MASTER�е�UDS���ˢд���� */
extern int ydm_api_udsc_service_config(ydiag_master_api *dm_api, yuint16 udscid, service_config *config);

/* UDS�ͻ��˵�һЩͨ������  ����3E��������ִ�ж��������� */
extern int ydm_api_udsc_runtime_config(ydiag_master_api *dm_api, yuint16 udscid, udsc_runtime_config *config);

/* �����ɵ�key���͸�ota master */
extern int ydm_api_udsc_sa_key(ydiag_master_api *dm_api, yuint16 udscid, yuint8 level, const yuint8 *key, yuint32 key_size);

/* UDS�������Ӧ���ķ��͸�DIAG MASTER�е�UDS�ͻ��˴��� */
extern int ydm_api_service_response(ydiag_master_api *dm_api, yuint16 udscid, const yuint8 *data, yuint32 size, yuint32 sa, yuint32 ta, yuint32 tatype);

/* ����DIAG MASTER�����������Ҫ����ʱ�����������ͻص����� */
extern int ydm_api_udsc_request_transfer_callback_set(ydiag_master_api *dm_api, yuint16 udscid, udsc_request_transfer_callback call, void *arg);

/* ����DIAG MASTER����Ͽͻ���������������ִ����ɺ�������ص����� */  
extern int ydm_api_udsc_task_end_callback_set(ydiag_master_api *dm_api, yuint16 udscid, udsc_task_end_callback call, void *arg);  
  
/* ���õ����������������Ӧ����Ļص������� */  
extern int ydm_api_udsc_service_result_callback_set(ydiag_master_api *dm_api, yuint16 udscid, uds_service_result_callback call, void *arg);  
  
/* ͨ��27�����ȡ����������key�Ļص��������� */  
extern int ydm_api_udsc_security_access_keygen_callback_set(ydiag_master_api *dm_api, yuint16 udscid, udsc_security_access_keygen_callback call, void *arg);  

extern int ydm_api_udsc_service_36_transfer_progress_callback_set(ydiag_master_api *dm_api, yuint16 udscid, uds_service_36_transfer_progress_callback call, void *arg);

extern int ydm_api_diag_master_instance_destroy_callback_set(ydiag_master_api *dm_api, diag_master_instance_destroy_callback call, void *arg);

/* ��������������ӦID/��ַ */  
extern int ydm_api_udsc_diag_id_storage(ydiag_master_api *dm_api, yuint16 udscid, yuint32 diag_req_id, yuint32 diag_resp_id);  
  
/* ͨ�������ӦID/��ַ����UDS�ͻ���ID */  
extern int ydm_api_udsc_index_by_resp_id(ydiag_master_api *dm_api, yuint32 diag_resp_id);  
  
/* ͨ���������ID/��ַ����UDS�ͻ���ID */  
extern int ydm_api_udsc_index_by_req_id(ydiag_master_api *dm_api, yuint32 diag_req_id);  
  
/* ͨ��UDS�ͻ���ID���������ӦID/��ַ */  
extern yuint32 ydm_api_udsc_resp_id(ydiag_master_api *dm_api, yuint16 udscid);  
  
/* ͨ��UDS�ͻ���ID�����������ID/��ַ */  
extern yuint32 ydm_api_udsc_req_id(ydiag_master_api *dm_api, yuint16 udscid);  

/* ��ȡUDS�ͻ����������� */
extern int ydm_api_udsc_rundata_statis(ydiag_master_api *dm_api, yuint16 udscid, runtime_data_statis_t *statis);

/* ���ں�DIAG MASTER���н��̼�ͨ�Ž����yom_api_sockfd() ��select epoll �������ʹ�� */  
extern void ydm_api_request_event_loop(ydiag_master_api *dm_api);

/* �����ն�����Զ��ָ������� */
extern int ydm_api_terminal_control_service_create(ydiag_master_api *dm_api, terminal_control_service_info_t *trinfo);

// ����һ���µ�YByteArray���󲢷�����ָ��  
extern ybyte_array *y_byte_array_new();  
  
// �ͷ�YByteArray������ռ�õ��ڴ�  
extern void y_byte_array_delete(ybyte_array *arr);  
  
// ���YByteArray���������  
extern void y_byte_array_clear(ybyte_array *arr);  
  
// ����YByteArray����ĳ�������ָ��  
extern const yuint8 *y_byte_array_const_data(ybyte_array *arr);  
  
// ����YByteArray�����е�Ԫ������  
extern int y_byte_array_count(ybyte_array *arr);  
  
// ��YByteArray����ĩβ���һ���ַ�  
extern void y_byte_array_append_char(ybyte_array *dest, yuint8 c);  
  
// ��һ��YByteArray���������׷�ӵ���һ��YByteArray������  
extern void y_byte_array_append_array(ybyte_array *dest, ybyte_array *src);  
  
// ��YByteArray����ĩβ׷��һ���ַ����������  
extern void y_byte_array_append_nchar(ybyte_array *dest, yuint8 *c, yuint32 count);  
  
// �Ƚ�����YByteArray�����Ƿ����  
extern int y_byte_array_equal(ybyte_array *arr1, ybyte_array *arr2);  
  
// �Ƚ�һ��YByteArray�����һ���ַ�����������Ƿ����  
extern int y_byte_array_char_equal(ybyte_array *arr1, yuint8 *c, yuint32 count);

extern int ydm_api_logd_print_set(int (*printf)(const char *format, ...));

extern int ydm_api_loge_print_set(int (*printf)(const char *format, ...));

extern int ydm_api_logw_print_set(int (*printf)(const char *format, ...));

extern int ydm_api_logi_print_set(int (*printf)(const char *format, ...));

#endif /* __YDM_API_H__ */
