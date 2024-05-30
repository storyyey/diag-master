#ifndef __YUDS_CLIENT_H__
#define __YUDS_CLIENT_H__
/*
  1.��ӵ���Ϸ�������򱣴��������� | 1.����uds�ͻ���ִ������                                                         |
    yudsc_service_item_add()         |   yudsc_services_start()                            |
    [ service 1 ]                      |                                                       |
          |                            | 2.uds�ͻ��˵�������Ҫ�������¼�����ʱ��                                |
          V                            |   request_watcher                                     |
    [ service 2 ]                      |   ���ڿ������������Ϣ�ķ��ͣ���ʱ������                                 |
          |                            |   �ص������ڽ��������������Ϣ����ʱ��ʱ                                 |
          V                            |   ���������ڷ�������ǰ�ȴ���ã�����0����                                |
    [ service 3 ]                      |   �������͡�
          |                            |   
          V                            |   response_watcher
    [ service 4 ]                      |   �����Ӧ��ʱ��ʱ�����ڳɹ�����������Ӧ��
          |                            |   �����������ʱ����yudsc_service_response_finish()
          V                            |   �յ�������Ӧ�󽫹رն�ʱ�������߳�ʱ��ǵ�
    [ service 5 ]                      |   ǰ��Ϸ���ʱ��Ȼ��������һ����Ϸ���
          |                            |   ����request_watcher��ʱ����
          V                            |   ��˷���ֱ��������Ϸ���ִ�н�����
    [ service 6 ]                      |   
                                       |   yudsc_services_start()
                                       |   request_watcher��ʱ������
                                       |              |
                                       |              V
                                       |   request_watcher��ʱ����    
                                       |              |
                                       |              V
                                       |          �����������                                            
                                       |              |�ɹ�
                                       |              V
                                       |   response_watcher��ʱ������ ->��ʱ�����Ӧ��ʱ
                                       |              |�յ���Ӧ              |
                                       |              V                  V
                                       |       ��һ����Ϸ�����          <--------
                                       |    request_watcher��ʱ������
*/

/* 27��������key buff��󳤶� */
#define SA_KEY_SEED_SIZE (64)

/* 36��������ش����� */
#define TD_RETRANS_NUM_MAX (2)

#define DSC_10_RESPONSE_FIX_LEN (6) 

#define NEGATIVE_RESPONSE_FIX_SIZE (3)
#define NEGATIVE_RESPONSE_ID (0x7f)

/* ���ļ�С��һ����ʱ��ֱ��ȫ�������ļ����ڴ��� */
#define FIRMWARE_FILE_CACHE_BUFF_MAX (1024 * 1024)

typedef enum service_response_stat {
    SERVICE_RESPONSE_NORMAL,
    SERVICE_RESPONSE_TIMEOUT,
} service_response_stat;

typedef struct service_36_td {
#define MAX_NUMBER_OF_BLOCK_LENGTH (10000) /* �豸֧�ֵ���������ݿ鳤�� */
    yuint32 max_number_of_block_length; /* �����鳤�ȣ����ⲿ���õ� */    
    char *local_path;
    yuint8 rt_cnt; /* �ش������ƴ� */
    boolean is_retrans; /* ��ǰ�Ƿ����ش� */
} service_36_td;

typedef struct service_34_rd {
    yuint8 data_format_identifier;
    yuint8 address_and_length_format_identifier;
    yuint32 memory_address;
    yuint32 memory_size;
} service_34_rd;

typedef struct service_38_rft {
    yuint8 mode_of_operation;
    yuint16 file_path_and_name_length;
    char *file_path_and_name;
    yuint8 data_format_identifier;
    yuint8 file_size_parameter_length;
    yuint32 file_size_uncompressed;
    yuint32 file_size_compressed;
} service_38_rft;

typedef int (*ykey)(
    const unsigned char*    iSeedArray,        // seed����
    unsigned short          iSeedArraySize,    // seed�����С
    unsigned int            iSecurityLevel,    // ��ȫ���� 1,3,5...
    const char*             iVariant,          // ��������, ������Ϊ��
    unsigned char*          iKeyArray,         // key����, �ռ����û�����
    unsigned short*         iKeyArraySize      // key�����С, ��������ʱΪkey����ʵ�ʴ�С
);

typedef struct service_27_ac {
    ykey key_callback;
} service_27_ac;

typedef void (*service_variable_byte_callback)(void *item, ybyte_array *variable_byte, void *arg);

typedef void (*service_response_callback)(void *item, const yuint8 *data, yuint32 len, void *arg);

enum service_item_status {
    SERVICE_ITEM_WAIT, /* �ȴ�ִ�� */
    SERVICE_ITEM_RUN, /* ����ִ���� */
    SERVICE_ITEM_TIMEOUT, /* ��Ӧ��ʱ */
    SERVICE_ITEM_FAILED, /* ���� */
    SERVICE_ITEM_SUCCESS, /* ��Ԥ��ִ�н��� */
};

/* ����ṹ���ڵı���ֵ�����������Ϸ�����ʱ��ʼ����ֵ�ģ�����ִ�й����в��޸����б���ֵ */
typedef struct service_item {
    enum service_item_status status;
    boolean isenable;
    yuint8 sid; /* ��Ϸ���ID */
    yuint8 sub; /* ��Ϸ����ӹ��ܣ�option */
    yuint16 did; /* ���ݱ�ʶ����option */
#define UDS_SERVICE_DELAY_MIN (0) /* unit ms */
#define UDS_SERVICE_DELAY_MAX (60000) /* unit ms */
    yint32 delay; /* ��������ʱʱ�䣬��ʱʱ�䵽���Żᴦ������������ unit ms */
#define UDS_SERVICE_TIMEOUT_MIN (10) /* unit ms */
#define UDS_SERVICE_TIMEOUT_TYPICAL_VAL (50) /* unit ms */
#define UDS_SERVICE_TIMEOUT_MAX (20000) /* unit ms */
    yint32 timeout; /* Ӧ��ȴ���ʱʱ�� unit ms */
    boolean issuppress; /* �Ƿ���������Ӧ */
    yuint32 tatype; /* Ŀ�ĵ�ַ���� */
    yuint32 ta; /* ���Ŀ�ĵ�ַ */
    yuint32 sa; /* ���Դ��ַ */
    ybyte_array *request_byte; /* �������� */
    ybyte_array *variable_byte; /* �ǹ̶���ʽ�����ݣ�����31 2e������治�̶������� */
    service_variable_byte_callback vb_callback; /* �ڹ�����������ʱ����� */
    void *vb_callback_arg;
    service_response_callback response_callback; /* ���յ�Ӧ���ʱ����� */
    void *response_callback_arg;
    service_36_td td_36; /* 36��������,��ѡ���� */
    service_34_rd rd_34; /* 34��������,��ѡ���� */
    service_38_rft *rft_38; /* 38��������,��ѡ���� */
    service_27_ac ac_27;
    
    service_response_expect response_rule; /* Ԥ����Ӧ���� */ 
    ybyte_array *expect_byte; /* Ԥ����Ӧ�ַ� */
    ybyte_array *expect_bytes[64]; /* ֧�ֶ��Ԥ����Ӧ�ַ� */

    service_finish_condition finish_rule; /* �������� */
    ybyte_array *finish_byte; /* ������Ӧ�ַ� */    
    ybyte_array *finish_bytes[64]; /* ֧�ֶ��������Ӧ�ַ� */
    
    yuint32 finish_try_num; /* �������������� */
    yuint32 finish_num_max; /* ��������Դ���������������������Ƿ���Ͻ���������������������� */
    yuint32 finish_time_max; /* �������ȴ�ʱ�䣬�������ʱ�䲻���Ƿ���Ͻ���������������������� */

    ybyte_array *response_byte; /* ��¼һ����Ӧ���� */
        
    yuint32 rr_callid; /* ota master api �˵��������ص�����ID������0����Ч */
    char *desc; /* ��Ϸ�����������Ϣ */
} service_item;

typedef enum udsc_finish_stat {
    UDSC_NORMAL_FINISH = 0, /* �������� */
    UDSC_SENT_ERROR_FINISH, /* �������������� */
    UDSC_UNEXPECT_RESPONSE_FINISH, /* ��Ԥ��Ӧ�������� */
    UDSC_TIMEOUT_RESPONSE_FINISH, /* ��ʱӦ�������� */
} udsc_finish_stat;

/* ��Ϸ�������ص����� */
typedef yint32 (*udsc_request_transfer_callback)(yuint16 id, void *arg, const yuint8 *data, yuint32 size, yuint32 sa, yuint32 ta, yuint32 tatype);

/* ��������������� */
typedef void (*udsc_task_end_callback)(void *udsc, udsc_finish_stat stat, void *arg/* �û�����ָ�� */, const yuint8 *ind, yuint32 indl, const yuint8 *resp, yuint32 respl);

/* ������������ */
typedef void (*security_access_keygen_callback)(void *arg/* �û�����ָ�� */, yuint16 id, yuint8 level, yuint8 *seed, yuint16 seed_size);

/* ����36��������� */
typedef void (*service_36_transfer_progress_callback)(void *arg/* �û�����ָ�� */, yuint16 id, yuint32 file_size, yuint32 total_byte, yuint32 elapsed_time);

typedef struct uds_client {
    boolean isidle; /* �Ƿ�ʹ�� */
    yuint16 id; /* uds�ͻ���ID */
    char *udsc_name; /* uds�ͻ����� */
    char dm_name[8]; /* diag master���� */
    struct ev_loop *loop; /* ���¼�ѭ���ṹ�� */
    ev_timer request_watcher; /* �������������ʱ�� */
    ev_timer response_watcher; /* �������Ӧ��ʱ��ʱ�� */
    ev_timer testerPresent_watcher; /* �������������ʱ�� */
    ev_timer td_36_progress_watcher; /* �������ͨ�涨ʱ�� */
    boolean isFailAbort; /* ������������������ʱ����ִֹ�з���������� */
    boolean response_meet_expect; /* ���й����б�־λ��Ӧ�����Ԥ��Ҫ�� */
    yint32 sindex; /* ��ǰ���д������Ϸ��������� service_items[sindex] */
    boolean iskeep; /* �Ƿ����ִ�е�ǰ��Ϸ����Ĭ�ϲ�����֧�ֵ�ǰ������й�����ȷ�� */
    yuint32 service_cnt; /* ��Ϸ������� */
    yuint32 service_size; /* service_items �Ĵ�С */
    service_item **service_items; /* ��Ϸ������б� */
    void *udsc_request_transfer_arg;
    udsc_request_transfer_callback udsc_request_transfer_cb; /* �����Ϣ���ͺ���������Ҫ������������ʱ����� */
    void *udsc_task_end_arg;
    udsc_task_end_callback udsc_task_end_cb; /* ��Ͽͻ��˽������� */    
    void *security_access_keygen_arg;    
    security_access_keygen_callback security_access_keygen_cb; /* ����Ҫ����key��ʱ��ص�������� */    
    void *service_36_transfer_progress_arg;    
    service_36_transfer_progress_callback service_36_transfer_progress_cb;    
    /* ������Ϸ�����֮�乲������ */
    struct common_variable {
        yuint8 file_transfer_count; /* 36���������к� */
        yuint32 file_transfer_total_size; /* 36�������ļ����ֽ��� */
        yuint32 file_size; /* �ļ���С */
        yuint8 *td_buff; /* 36������buff */
        yuint8 *ff_cache_buff; /* 36�����ȡ�ļ�ʹ�õĻ����С */
        void *filefd; /* �򿪵��ļ�������34 36 37 38 ����ʹ�� */
        yuint32 max_number_of_block_length; /* �����鳤�ȣ���34,38�������Ӧ�õ���ʵ��36����ʱʹ����������ĳ���ֵ��������������� */
        ybyte_array *seed_byte;
        ybyte_array *key_byte; /* ������seed�����ɵ�key */
        yuint16 p2_server_max;
        yuint16 p2_e_server_max;
    } common_var;
    boolean runtime_statis_enable; /* ʹ������ͳ�� */
    /* ------------һЩ��Ҫ��¼���м����start-------------- */
    yuint32 run_start_time;
    yuint32 recv_pretime;
    int recv_total_byte;
    int recv_average_byte;
    yuint32 sent_pretime;
    int sent_total_byte;
    int sent_average_byte;
    /* ------------һЩ��Ҫ��¼���м����end-------------- */
    /* ����ʱ����ͳ�� */
    struct runtime_data_statis_s rt_data_statis;
    /* ������������� */
    boolean tester_present_enable; /* ʹ����������������� */
    boolean is_tester_present_refresh; /* �Ƿ�UDS����ˢ�¶�ʱ�� */
    yuint32 tester_present_interval; /* ���ͼ�� unit ms */
    yuint32 tester_present_ta; /* ���Ŀ�ĵ�ַ */
    yuint32 tester_present_sa; /* ���Դ��ַ */ 
    void *doipc_channel; /* doip����ͨ�� */

    yuint32 td_36_start_time; 
 #define TD_36_PROGRESS_NOTIFY_INTERVAL_MIN (10) /* ms */
 #define TD_36_PROGRESS_NOTIFY_INTERVAL_MAX (10000) /* ms */
    yuint32 td_36_progress_interval;

    boolean uds_msg_parse_enable; /* UDS��Ϣ����ʹ�ܣ���������Щ��������� */
    boolean uds_asc_record_enable; /* UDS��Ϣ��¼����������Щ��������� */
} uds_client;

/*
    �����ڲ������ uds_client * �� service_item * ָ���Ƿ�Ϊ�գ��ڴ���ǰ���
*/

/*
    ����һ��UDS �ͻ��ˣ���ECU����ʱ���Դ������ECU�ͻ���
*/
extern uds_client *yudsc_create();

/*
    �����������ǰ��ȷ��û���¼�ѭ����û�����ҵ����ִ��
    ����UDS �ͻ��ˣ������������ִ�л����¼�ѭ��û��ֹͣ��ʱ�򷵻ش���
*/
extern boolean yudsc_destroy(uds_client *udsc);

/*
    �¼�ѭ������Ҫѭ����������������ܽ����¼�ѭ��
    �� yudsc_thread_loop_start ��ѡһ����
*/
extern boolean yudsc_event_loop_start(uds_client *udsc);

/*
    ���߳��ڽ����¼�ѭ����ֻ�õ���һ�ξ��У���Ҫ��ע�߳���Դ��������
    �� yudsc_event_loop_start ��ѡһ����
*/
extern boolean yudsc_thread_loop_start(uds_client *udsc);

/*
    ֹͣ�¼�ѭ��
*/
extern boolean yudsc_loop_stop(uds_client *udsc);

/*
    �Ƿ������������ִ����
*/
extern boolean yudsc_service_isactive(uds_client *udsc);

/*
    ��ʼִ���������
*/
extern boolean yudsc_services_start(uds_client *udsc);

/*
    ִֹͣ���������
*/
extern boolean yudsc_services_stop(uds_client *udsc);

/*
    ���������ִ�г����Ƿ���ִֹ���������
*/
extern void yudsc_service_fail_abort(uds_client *udsc, boolean b);

/*
    ����һ����Ϸ�����Ѿ����뵽��Ϸ���ִ�б���
*/
extern service_item *yudsc_service_item_add(uds_client *udsc, char *desc);

/*
    ����Ϸ���ִ�б���ɾ��һ����Ϸ��������������ʹ����
*/
extern void yudsc_service_item_del(uds_client *udsc, service_item *item);

/*
    ���ݽṹ���ڵ���������������
*/
extern boolean yudsc_service_request_build(service_item *sitem);

/*
    ������������ͻص�����
*/
extern void yudsc_request_transfer_callback_set(uds_client *udsc, udsc_request_transfer_callback call, void *arg);

/*
    ����������Ϸ���ִ�н�����Ļص�������
*/
extern void yudsc_task_end_callback_set(uds_client *udsc, udsc_task_end_callback call, void *arg);

extern void yudsc_service_variable_byte_callback_set(service_item *sitem, service_variable_byte_callback vb_callback, void *arg);

extern void yudsc_service_response_callback_set(service_item *sitem, service_response_callback response_callback, void *arg);

extern void yudsc_service_response_finish(uds_client *udsc, service_response_stat stat, yuint32 sa, yuint32 ta, yuint8 *data, yuint32 size);

extern service_item *yudsc_curr_service_item(uds_client *udsc);

extern boolean yudsc_reset(uds_client *udsc); 

extern void yudsc_service_sid_set(service_item *sitem, yuint8 sid);

extern yuint8 yudsc_service_sid_get(service_item *sitem);

extern void yudsc_service_sub_set(service_item *sitem, yuint8 sub);

extern yuint8 yudsc_service_sub_get(service_item *sitem);

extern void yudsc_service_did_set(service_item *sitem, yuint16 did);

extern yuint16 yudsc_service_did_get(service_item *sitem);

extern void yudsc_service_delay_set(service_item *sitem, yint32 delay);

extern void yudsc_service_timeout_set(service_item *sitem, yint32 timeout);

extern void yudsc_service_suppress_set(service_item *sitem, boolean b);

extern void yudsc_service_enable_set(service_item *sitem, boolean b);

extern void yudsc_service_expect_response_set(service_item *sitem, service_response_expect rule, yuint8 *data, yuint32 size);

extern void yudsc_service_finish_byte_set(service_item *sitem, service_finish_condition rule, yuint8 *data, yuint32 size);

extern void yudsc_service_key_set(service_item *sitem, ykey key_callback);

extern void yudsc_ev_loop_set(uds_client* udsc, struct ev_loop* loop);

extern void yudsc_service_key_generate(uds_client *udsc, yuint8 *key, yuint16 key_size);

extern void yudsc_security_access_keygen_callback_set(uds_client *udsc, security_access_keygen_callback call, void *arg);

extern void yudsc_36_transfer_progress_callback_set(uds_client *udsc, service_36_transfer_progress_callback call, void *arg);

extern void yudsc_doip_channel_bind(uds_client *udsc, void *doip_channel);

extern void yudsc_doip_channel_unbind(uds_client *udsc);

extern void *yudsc_doip_channel(uds_client *udsc);

extern boolean yudsc_runtime_data_statis_get(uds_client *udsc, runtime_data_statis_t *rt_data_statis);

extern void yudsc_runtime_data_statis_enable(uds_client *udsc, boolean en);

extern boolean yudsc_service_item_capacity_init(uds_client *udsc, int size);

extern int yudsc_service_item_capacity(uds_client *udsc);

extern int yudsc_security_access_key_set(uds_client *udsc, yuint8 *key, yuint16 key_size);

extern int yudsc_tester_present_config(uds_client *udsc, yuint8 tester_present_enable, yuint8 is_tester_present_refresh, yuint32 tester_present_interval, yuint32 tester_present_ta, yuint32 tester_present_sa);

extern int yudsc_fail_abort(uds_client *udsc, boolean b);

extern int yudsc_id_set(uds_client *udsc, int id);

extern int yudsc_id(uds_client *udsc);

extern int yudsc_idle_set(uds_client *udsc, boolean b);

extern boolean yudsc_is_idle(uds_client *udsc);

extern int yudsc_td_36_progress_interval_set(uds_client *udsc, yuint32 interval);

extern int yudsc_name_set(uds_client *udsc, char *name);

extern boolean yudsc_asc_record_enable(uds_client *udsc);

extern void yudsc_asc_record_enable_set(uds_client *udsc, boolean b);

extern void yudsc_msg_parse_enable_set(uds_client *udsc, boolean b);

#endif /* __YUDS_CLIENT_H__ */
