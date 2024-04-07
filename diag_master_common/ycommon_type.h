#ifndef __YCOMMON_TYPE_H__
#define __YCOMMON_TYPE_H__

#include "ydm_types.h"

/* ---------------------------------------------��Ӧ������------------------------------------------------ */
#define DM_ERR_NO                   (0)
#define DM_ERR_SERVICE_REQUEST      (1) /* ���������ʧ�� */
#define DM_ERR_START_SCRIPT_FAILED  (2) /* ������Ϸ���ű�ʧ�� */
#define DM_ERR_NOT_FOUND_FILE       (3) /* δ���ֽű��ļ� */
#define DM_ERR_UNABLE_PARSE_FILE    (4) /* �޷������ű��ļ� */
#define DM_ERR_UNABLE_PARSE_CONFIG  (5) /* �޷������ű����� */
#define DM_ERR_NOT_FOUND_SCRIPT     (6) /* δ���ֽű� */
#define DM_ERR_UDSC_CREATE          (7) /* uds�ͻ��˴���ʧ�� */
#define DM_ERR_UDSC_INVALID         (8) /* ��Ͽͻ���ID��Ч */
#define DM_ERR_UDSC_RUNING          (9) /* udsc�ͻ����������޷����� */
#define DM_ERR_SHM_OUTOF            (10) /* �����ڴ�ռ䲻�� */
#define DM_ERR_REPLY_TIMEOUT        (11) /* Ӧ��ʱ */
#define DM_ERR_UDS_RESPONSE_TIMEOUT (12) /* ���Ӧ��ʱ */
#define DM_ERR_UDS_RESPONSE_UNEXPECT (13) /* ��Ϸ�Ԥ����Ӧ */
#define DM_ERR_UDS_RESPONSE_OVERLEN  (14) /* ��Ͻ�����ȹ���������Buff���� */
#define DM_ERR_UDS_SERVICE_ADD       (15) /* UDS���������ʧ�� */
#define DM_ERR_DIAG_MASTER_MAX        (16) /* ����OTA MASTER����ota master�ﵽ���������� */
#define DM_ERR_OMAPI_UNKNOWN         (17) /* ����OTA MASTER����δ֪ota master api */
#define DM_ERR_DIAG_MASTER_CREATE     (18) /* ����OTA MASTER����ota master����ʧ�� */
#define DM_ERR_DOIPC_INVALID         (19) /* doip�ͻ���ID��Ч */
#define DM_ERR_DOIPC_CREATE_FAILED   (20) /* doip�ͻ��˴���ʧ�� */
#define DM_ERR_UDSC_RUNDATA_STATIS   (21) /* UDS�ͻ�������ͳ������δ���� */
#define DM_ERR_TCS_OUT_NUM_MAX       (22) /* �ն˿��Ʒ��񴴽��������ֵ */
#define DM_ERR_TCS_EXIST             (23) /* ��ͬ���ն˿��Ʒ��� */ 
#define DM_ERR_TCS_CREATE_FAILED     (24) /* �ն˿��Ʒ��񴴽�ʧ�� */ 
#define DM_ERR_TCS_INVALID           (25) /* �ն˿��Ʒ�����Ч */   

/* ------------------------------------------------------------------------------------------------------- */

struct YByteArray;
typedef struct YByteArray YByteArray;

typedef enum serviceResponseExpect {
    NOT_SET_RESPONSE_EXPECT = 0, /* δ����Ԥ����Ӧ */
    POSITIVE_RESPONSE_EXPECT, /* Ԥ������Ӧ */
    NEGATIVE_RESPONSE_EXPECT, /* Ԥ�ڸ���Ӧ */
    MATCH_RESPONSE_EXPECT, /* У��Ԥ����Ӧ */
    NO_RESPONSE_EXPECT, /* Ԥ������Ӧ */
} serviceResponseExpect;

typedef enum serviceFinishCondition {
    FINISH_DEFAULT_SETTING = 0, /* Ĭ������ */
    FINISH_EQUAL_TO, /* ���ڽ�����Ӧ�ַ� */
    FINISH_UN_EQUAL_TO, /* �����ڽ�����Ӧ�ַ� */
    FINISH_EQUAL_TO_POSITIVE_RESPONSE, /* ��������Ӧ���� */
    FINISH_EQUAL_TO_NEGATIVE_RESPONSE, /* ���ڸ���Ӧ���� */
    FINISH_EQUAL_TO_NO_RESPONSE, /* ����Ӧ���� */
    FINISH_EQUAL_TO_NORMAL_RESPONSE, /* ����Ӧ���� */
} serviceFinishCondition;
    
typedef struct service_config {
    /* ��Ϸ���ID */
    yuint8 sid;
    /* ����ӹ��� */
    yuint8 sub; 
    /* ������ݱ�ʶ�� */
    yuint16 did; 
    /* ��Ϸ�������ǰ����ʱʱ�� */
    yuint32 delay; 
    /* ��Ϸ�����Ӧ��ʱʱ�� */
    yuint32 timeout; 
    /* �Ƿ���������Ӧ */
    yuint8 issuppress;
    /* Ŀ���ַ���� */
    yuint32 tatype;
    /* Ŀ���ַ */
    yuint32 ta; 
    /* Դ��ַ */
    yuint32 sa; 
    /* ��Ϸ����еĿɱ����ݣ�UDS �ͻ��˽����� sid sub did������Զ�����UDS�������� */
    YByteArray *variableByte; 
    /* Ԥ�������Ӧ���ݣ������жϵ�ǰ��Ϸ���ִ���Ƿ����Ԥ�� */
    YByteArray *expectResponByte; 
    /* Ԥ����Ӧ���� */
    yuint32 expectRespon_rule;
    /* ��Ӧ����ƥ�����ݣ������жϵ�ǰ��Ϸ����Ƿ���Ҫ�ظ�ִ�� */
    YByteArray *finishByte;  
    /* ��Ӧ�������� */
    yuint32 finish_rule; 
    /* ��Ӧ�������ƥ����� */
    yuint32 finish_num_max;
    /* �������ص�������ID, ����0������Чֵ */
    yuint32 rr_callid; 
    struct  {
        yuint8 dataFormatIdentifier;
        yuint8 addressAndLengthFormatIdentifier;
        yuint32 memoryAddress;
        yuint32 memorySize;
    } service_34_rd;
    struct  {
        yuint8 modeOfOperation;
        yuint16 filePathAndNameLength;
        char filePathAndName[256];
        yuint8 dataFormatIdentifier;
        yuint8 fileSizeParameterLength;
        yuint32 fileSizeUnCompressed;
        yuint32 fileSizeCompressed;
    } service_38_rft;
    /* 36���������ݿ���󳤶� */
    yuint32 maxNumberOfBlockLength;
    /* ����ˢд�ļ�·�� */
    char local_path[256]; 
    /* ��Ϸ������������Ϣ */
    char desc[256]; 
} service_config; 

typedef struct udsc_runtime_config {    
    int padding; /*  */
    /* ������������������ʱ����ִֹ�з���������� */
    yuint8 isFailAbort;
    /* ʹ����������������� */
    yuint8 tpEnable; 
    /* �Ƿ�UDS����ˢ�¶�ʱ�� */
    yuint8 isTpRefresh;
    /* ���ͼ�� unit ms */
    yuint32 tpInterval;
    /* ���Ŀ�ĵ�ַ */
    yuint32 tpta;
    /* ���Դ��ַ */
    yuint32 tpsa;
    /* 36���������ͨ���� */
    yuint32 td_36_notify_interval;    
    /* UDS�ͻ�������ʱ����ͳ�� */ 
    yuint8 runtime_statis_enable;    
    BOOLEAN uds_msg_parse_enable; /* UDS��Ϣ����ʹ�ܣ���������Щ��������� */
    BOOLEAN uds_asc_record_enable; /* UDS��Ϣ��¼����������Щ��������� */
} udsc_runtime_config;

typedef struct udsc_create_config {
    int padding; /*  */
    /* ֧����������������Ϸ����� */
    yuint32  service_item_capacity;
    char udsc_name[256];    
} udsc_create_config;

typedef struct doipc_config_s {
    /* doip�汾�� */
    yuint8 ver;
    /* doip�ͻ���Դ��ַ */
    yuint16 sa_addr;
    /* doip�ͻ���IP��ַ */
    yuint32 src_ip;
    /* doip�ͻ��˶˿ں� */
    yuint16 src_port;
    /* doip������IP��ַ */
    yuint32 dst_ip;
    /* doip�������˿ں� */
    yuint16 dst_port;
    /* ���ջ��������� */
    yuint32 rxlen;
    /* ���ͻ��������� */
    yuint32 txlen;
} doipc_config_t;

/* ����ʱ����ͳ�� */
typedef struct runtime_data_statis_s {
    yuint32 sent_total_byte; /* �ܹ������˶����ֽ����� */
    yuint32 recv_total_byte; /* �ܹ������˶����ֽ����� */
    yuint32 sent_peak_speed; /* ��������ٶ� byte/s */
    yuint32 recv_peak_speed; /* ��������ٶ� byte/s */
    yuint32 sent_valley_speed; /* ��������ٶ� byte/s */
    yuint32 recv_valley_speed; /* ��������ٶ� byte/s */
    yuint32 run_time_total; /* �ܹ�������ʱ�䵥λ�� */
    yuint32 sent_num; /* �ܹ������˶�������Ϸ�����Ϣ */
    yuint32 recv_num; /* �ܹ������˶�������Ϸ�����Ϣ */
} runtime_data_statis_t;

typedef struct terminal_control_service_info_s {
    /*
       �豸��ʶ BCD[10] ���ݲ��㣬����ǰ��������0
    */
    yuint8 devid[10];
    /*
       ��ʾ�ն˰�װ�������ڵ�ʡ��0��������ƽ̨ȡĬ��ֵ��
       ʡ��ID����GB/T2260�涨���������������λ�е�ǰ��λ
    */
    yuint16 provinceid; /* ʡ��ID */
    /*
       ��ʾ�ն˰�װ�������ڵ����������0������
       ��ƽ̨ȡĬ��ֵ��ʡ��ID����GB/T2260�涨���������������λ�еĺ���λ
    */
    yuint16 cityid; /* ������ID */
    /*
       TBOX������
    */
    yuint8 manufacturer[11]; /* ������ID */
    /*
       TBOX�ͺ�
    */
    yuint8 model[30]; /* �ն��ͺ� */
    /*
       �ն�ID
    */
    yuint8 terid[30]; /* �ն�ID */
    /*
        ������ɫ
    */
    yuint8 color;
    /*
        VIN
    */
    char vin[17]; 
    /*
        IMEI
    */
    char imei[15];
    /*
        ������ip��ַ��������
    */
    char srv_addr[256];
    /*
        �������˿�
    */
    short srv_port;

    /* ����汾 */
    yuint8 app_version[20]; 
} terminal_control_service_info_t;

#endif /* __YCOMMON_TYPE_H__ */
