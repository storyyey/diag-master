#ifndef __YDM_IPC_COMMON_H__
#define __YDM_IPC_COMMON_H__

#include "ycommon_type.h"

#ifdef __linux__
#define DM_UNIX_SOCKET_PATH_PREFIX "/tmp/yom_uds_"
#define OMS_UNIX_SOCKET_PATH "/tmp/yoms_uds"
#define DM_API_UNIX_SOCKET_PATH_PREFIX "/tmp/yom_api_uds_"
#define DM_API_UNIX_SOCKET_CALL_PATH_PREFIX "/tmp/yom_api_call_uds_"
#endif /* __linux__ */

#ifdef _WIN32
#define DMS_UDS_SOCKET_PATH "127.0.0.1"
#define DMS_UDS_SOCKET_PORT (37777)
#define DMS_UDS_SOCKET_PORT_BASE (38888)

#define DM_UDS_SOCKET_PATH "127.0.0.1"
#define DM_UDS_SOCKET_PORT_BASE (47777)
#endif /* _WIN32S */

/* ���֧�ִ������ٸ�UDS�ͻ��� */
#define DM_UDSC_CAPACITY_MAX (20) 

/* ���֧�ִ������ٸ�DOIP�ͻ��� */
#define DM_DOIPC_CAPACITY_MAX (DM_UDSC_CAPACITY_MAX) 

/* ipcͨ����Ϣ��Чʱ�䣬��ֹʧЧ����ϢӰ����������unitms */
#define IPC_MSG_VALID_TIME (1000) 

/* ��Ϸ�����֧����ӵ�Ĭ��������������� */
#define SERVICE_ITEM_SIZE_DEF (500)
#define SERVICE_ITEM_SIZE_MAX (10000)

/* IPCͨ�ŵ��շ����������� */
#define IPC_RX_BUFF_SIZE  (10240)
#define IPC_TX_BUFF_SIZE  (10240)

/* ipc ͨ��ʱ���unix���׽���·������ */
#define IPC_PATH_LEN (256)

/* �Ƿ�ʹ��ϵͳʱ����ΪIPC��Ϣ��ʱ�������Ƶ����ϵͳʱ�亯�����ܴ����쳣 */
#define IPC_USE_SYSTEM_TIME_STAMP 0,0 
#define IPC_USE_USER_TIME_STAMP 0x12345678,0x12345678

/* δ�յ���Ӧʱ�����Դ��� */
#define IPC_SYNC_MSG_TRYNUM (3)

/* ֱ���ڴ��ַȡint���� */
#define TYPE_CAST_UINT(mm) (*(yuint32 *)((yuint8 *)(mm)))

/* ֱ���ڴ��ַȡshort���� */
#define TYPE_CAST_USHORT(mm) (*(yuint16 *)((yuint8 *)(mm)))

/* ֱ���ڴ��ַȡchar���� */
#define TYPE_CAST_UCHAR(mm) (*(yuint8 *)((yuint8 *)(mm)))

/* ָ��ƫ��n�ֽ� */
#define pointer_offset_nbyte(p, offset) (((yuint8 *)(p)) + (offset))

/* ������� */
#define memappend(addr, data, nbyte) memcpy(addr, data, nbyte)

/* ------------------------------------------------------------------------------------------------------ */
typedef struct YByteArray {
    yuint8 *data;
    yuint32 dlen;
    yuint32 size;
} YByteArray;

/* -----------------------------------------��Ϸ��������еĹؼ���---------------------------------------- */

/* ------------------------------------------------------------------------------------------------------- */

/* ----------------------------------diag_master ��diag_master_api�Ľ�������--------------------------------- */
#define DM_EVENT_ACCEPT_MASK (0x40) /* ��Ӧ�������� */

/* diag_master ����������� */
#define DM_EVENT_SERVICE_INDICATION_EMIT       (1) /* diag_master -> diag_master_api  */
#define DM_EVENT_SERVICE_INDICATION_ACCEPT     (DM_EVENT_SERVICE_INDICATION_EMIT | DM_EVENT_ACCEPT_MASK) /* diag_master_api -> diag_master  */

/* diag_master_api �������Ӧ������ */
#define DM_EVENT_SERVICE_RESPONSE_EMIT         (2) /* diag_master_api -> diag_master  */
#define DM_EVENT_SERVICE_RESPONSE_ACCEPT       (DM_EVENT_SERVICE_RESPONSE_EMIT | DM_EVENT_ACCEPT_MASK)  /* diag_master -> diag_master_api  */

/* diag_master_api ���� diag_master ������Ͻű� */
#define DM_EVENT_START_UDS_CLIENT_EMIT         (3) /* diag_master_api -> diag_master  */
#define DM_EVENT_START_UDS_CLIENT_ACCEPT       (DM_EVENT_START_UDS_CLIENT_EMIT | DM_EVENT_ACCEPT_MASK) /* diag_master -> diag_master_api  */

/* diag_master_api ���� diag_master ֹͣ��Ͻű� */
#define DM_EVENT_STOP_UDS_CLIENT_EMIT          (4) /* diag_master_api -> diag_master  */
#define DM_EVENT_STOP_UDS_CLIENT_ACCEPT        (DM_EVENT_STOP_UDS_CLIENT_EMIT | DM_EVENT_ACCEPT_MASK) /* diag_master -> diag_master_api  */

/* diag_master ֪ͨ diag_master_api �������ִ�н�� */
#define DM_EVENT_UDS_CLIENT_RESULT_EMIT        (5) /* diag_master -> diag_master_api  */
#define DM_EVENT_UDS_CLIENT_RESULT_ACCEPT      (DM_EVENT_UDS_CLIENT_RESULT_EMIT | DM_EVENT_ACCEPT_MASK) /* diag_master_api -> diag_master  */

/* diag_master_api ������Ͻű� */
#define DM_EVENT_UDS_SERVICE_ITEM_ADD_EMIT     (6) /* diag_master_api -> diag_master  */
#define DM_EVENT_UDS_SERVICE_ITEM_ADD_ACCEPT   (DM_EVENT_UDS_SERVICE_ITEM_ADD_EMIT | DM_EVENT_ACCEPT_MASK) /* diag_master -> diag_master_api  */

/* diag_master_api �� diag_masterֱ�Ӷ��ű��ļ� */
#define DM_EVENT_READ_UDS_SERVICE_CFG_FILE_EMIT   (7) /* diag_master_api -> diag_master  */
#define DM_EVENT_READ_UDS_SERVICE_CFG_FILE_ACCEPT (DM_EVENT_READ_UDS_SERVICE_CFG_FILE_EMIT | DM_EVENT_ACCEPT_MASK) /* diag_master -> diag_master_api  */

/* diag_master_api �� diag_master ���� uds�ͻ��� */
#define DM_EVENT_UDS_CLIENT_CREATE_EMIT        (8) /* diag_master_api -> diag_master  */
#define DM_EVENT_UDS_CLIENT_CREATE_ACCEPT      (DM_EVENT_UDS_CLIENT_CREATE_EMIT | DM_EVENT_ACCEPT_MASK) /* diag_master -> diag_master_api  */

/* diag_master_api �� diag_master ���� uds�ͻ��� */
#define DM_EVENT_UDS_CLIENT_DESTORY_EMIT       (9) /* diag_master_api -> diag_master  */
#define DM_EVENT_UDS_CLIENT_DESTORY_ACCEPT     (DM_EVENT_UDS_CLIENT_DESTORY_EMIT | DM_EVENT_ACCEPT_MASK) /* diag_master -> diag_master_api  */

/* diag_master_api ���� diag_master ��λ */
#define DM_EVENT_DIAG_MASTER_RESET_EMIT         (10) /* diag_master_api -> diag_master  */
#define DM_EVENT_DIAG_MASTER_RESET_ACCEPT       (DM_EVENT_DIAG_MASTER_RESET_EMIT | DM_EVENT_ACCEPT_MASK) /* diag_master -> diag_master_api  */

/* diag_master_api ���� diag_master ����ͨ���� */
#define DM_EVENT_UDS_CLIENT_GENERAL_CFG_EMIT      (11) /* diag_master_api -> diag_master  */
#define DM_EVENT_UDS_CLIENT_GENERAL_CFG_ACCEPT    (DM_EVENT_UDS_CLIENT_GENERAL_CFG_EMIT | DM_EVENT_ACCEPT_MASK) /* diag_master -> diag_master_api  */

/* diag_master ֪ͨ diag_master_api ��������� */
#define DM_EVENT_UDS_SERVICE_RESULT_EMIT          (12) /* diag_master -> diag_master_api  */
#define DM_EVENT_UDS_SERVICE_RESULT_ACCEPT        (DM_EVENT_UDS_SERVICE_RESULT_EMIT | DM_EVENT_ACCEPT_MASK) /* diag_master_api -> diag_master  */

/* diag_master�������Ӹ�diag_master_api����key */
#define DM_EVENT_UDS_SERVICE_27_SA_SEED_EMIT     (13) /* diag_master -> diag_master_api  */
#define DM_EVENT_UDS_SERVICE_27_SA_SEED_ACCEPT   (DM_EVENT_UDS_SERVICE_27_SA_SEED_EMIT | DM_EVENT_ACCEPT_MASK) /* diag_master_api -> diag_master  */

/* diag_master_api�����ɵ�key���͸�diag_master */
#define DM_EVENT_UDS_SERVICE_27_SA_KEY_EMIT     (14) /* diag_master_api -> diag_master  */
#define DM_EVENT_UDS_SERVICE_27_SA_KEY_ACCEPT   (DM_EVENT_UDS_SERVICE_27_SA_KEY_EMIT | DM_EVENT_ACCEPT_MASK) /* diag_master -> diag_master_api  */

/* diag_master_api��������ota master������ */
#define DM_EVENT_CONNECT_DIAG_MASTER_EMIT     (15) /* diag_master_api -> diag_master  */
#define DM_EVENT_CONNECT_DIAG_MASTER_ACCEPT   (DM_EVENT_CONNECT_DIAG_MASTER_EMIT | DM_EVENT_ACCEPT_MASK) /* diag_master -> diag_master_api  */

/* ota master��������������ȷ��diag_master_api�Ƿ����� */
#define DM_EVENT_OMAPI_KEEPALIVE_EMIT     (16) /* diag_master -> diag_master_api  */
#define DM_EVENT_OMAPI_KEEPALIVE_ACCEPT   (DM_EVENT_OMAPI_KEEPALIVE_EMIT | DM_EVENT_ACCEPT_MASK) /* diag_master_api -> diag_master  */

/* diag_master_api���󴴽�DOIP�ͻ��� */
#define DM_EVENT_DOIP_CLIENT_CREATE_EMIT     (17) /* diag_master_api -> diag_master  */
#define DM_EVENT_DOIP_CLIENT_CREATE_ACCEPT   (DM_EVENT_DOIP_CLIENT_CREATE_EMIT | DM_EVENT_ACCEPT_MASK) /* diag_master -> diag_master_api  */

/* diag_master_api����doipͨ����uds�ͻ��� */
#define DM_EVENT_DOIP_CLIENT_BIND_UDSC_EMIT     (18) /* diag_master_api -> diag_master  */
#define DM_EVENT_DOIP_CLIENT_BIND_UDSC_ACCEPT   (DM_EVENT_DOIP_CLIENT_BIND_UDSC_EMIT | DM_EVENT_ACCEPT_MASK) /* diag_master -> diag_master_api  */

/* diag_master_api�����ȡUDS�ͻ�����������ͳ�� */
#define DM_EVENT_UDS_CLIENT_RUNTIME_DATA_STATIS_EMIT     (19) /* diag_master_api -> diag_master  */
#define DM_EVENT_UDS_CLIENT_RUNTIME_DATA_STATIS_ACCEPT   (DM_EVENT_UDS_CLIENT_RUNTIME_DATA_STATIS_EMIT | DM_EVENT_ACCEPT_MASK) /* diag_master -> diag_master_api  */

/* diag_master_api�����ȡUDS�ͻ����Ƿ�����ִ���� */
#define DM_EVENT_UDS_CLIENT_ACTIVE_CHECK_EMIT     (20) /* diag_master_api -> diag_master  */
#define DM_EVENT_UDS_CLIENT_ACTIVE_CHECK_ACCEPT   (DM_EVENT_UDS_CLIENT_ACTIVE_CHECK_EMIT | DM_EVENT_ACCEPT_MASK) /* diag_master -> diag_master_api  */

/* diag_master_api�����ȡapi�Ƿ���Ч */
#define DM_EVENT_OMAPI_CHECK_VALID_EMIT     (21) /* diag_master_api -> diag_master  */
#define DM_EVENT_OMAPI_CHECK_VALID_ACCEPT   (DM_EVENT_OMAPI_CHECK_VALID_EMIT | DM_EVENT_ACCEPT_MASK) /* diag_master -> diag_master_api  */

/* �����ն˿��Ʒ��� */
#define DM_EVENT_TERMINAL_CONTROL_SERVICE_CREATE_EMIT    (22) /* diag_master_api -> diag_master */
#define DM_EVENT_TERMINAL_CONTROL_SERVICE_CREATE_ACCEPT  (DM_EVENT_TERMINAL_CONTROL_SERVICE_CREATE_EMIT | DM_EVENT_ACCEPT_MASK) /* diag_master -> diag_master_api  */

/* �����ն˿��Ʒ��� */
#define DM_EVENT_TERMINAL_CONTROL_SERVICE_DESTORY_EMIT    (23) /* diag_master_api -> diag_master */
#define DM_EVENT_TERMINAL_CONTROL_SERVICE_DESTORY_ACCEPT  (DM_EVENT_TERMINAL_CONTROL_SERVICE_DESTORY_EMIT | DM_EVENT_ACCEPT_MASK) /* diag_master -> diag_master_api  */

/* ����36����Ĵ������ */
#define DM_EVENT_36_TRANSFER_PROGRESS_EMIT (24) /* diag_master -> diag_master_api  */
#define DM_EVENT_36_TRANSFER_PROGRESS_ACCEPT  (DM_EVENT_36_TRANSFER_PROGRESS_EMIT | DM_EVENT_ACCEPT_MASK) /* diag_master_api -> diag_master  */

/* diag_master ���� diag_master_api�����¼�֪ͨ */
#define DM_EVENT_INSTANCE_DESTORY_EMIT (25) /* ˫������Ӧ */

#define DM_IPC_EVENT_TYPE_MAX (26) /* �������ֵ */
/* ------------------------------------------------------------------------------------------------------- */

#define DIAGNOSTIC_TYPE_CAN (1)
#define DIAGNOSTIC_TYPE_DOIP (2)
#define DIAGNOSTIC_TYPE_REMOTE (3)
#define DIAGNOSTIC_TYPE_RUNTIME_DBG (4)

#define PHYSICAL_ADDRESS (1)
#define FUNCTION_ADDRESS (2)

/* ���̼佻�����ݽṹ���壬ֱ�Ӷ�����������ڽṹ���ڵ�ƫ����������ֱ�ӷ��ʹ����ڴ��е����ݣ��������ݿ������� */
/* ------------------------------------------------------------------------------------------------------- */
/* struct ipc_common_msg */
#define DM_IPC_EVENT_TYPE_OFFSET (0)
#define DM_IPC_EVENT_TYPE_SIZE   (4)

#define DM_IPC_EVENT_RECODE_OFFSET (DM_IPC_EVENT_TYPE_OFFSET + DM_IPC_EVENT_TYPE_SIZE)
#define DM_IPC_EVENT_RECODE_SIZE   (4) 

#define DM_IPC_EVENT_UDSC_ID_OFFSET (DM_IPC_EVENT_RECODE_OFFSET + DM_IPC_EVENT_RECODE_SIZE)
#define DM_IPC_EVENT_UDSC_ID_SIZE   (2)

#define DM_IPC_EVENT_TV_SEC_OFFSET (DM_IPC_EVENT_UDSC_ID_OFFSET + DM_IPC_EVENT_UDSC_ID_SIZE)
#define DM_IPC_CEVENT_TV_SEC_SIZE   (4)

#define DM_IPC_EVENT_TV_USEC_OFFSET (DM_IPC_EVENT_TV_SEC_OFFSET + DM_IPC_CEVENT_TV_SEC_SIZE)
#define DM_IPC_EVENT_TV_USEC_SIZE   (4)

#define DM_IPC_EVENT_MSG_SIZE (DM_IPC_EVENT_TYPE_SIZE + \
                                DM_IPC_EVENT_RECODE_SIZE + \
                                DM_IPC_EVENT_UDSC_ID_SIZE + \
                                DM_IPC_CEVENT_TV_SEC_SIZE + \
                                DM_IPC_EVENT_TV_USEC_SIZE)
typedef struct ipc_common_msg {
    yuint32 evtype; /* �¼����� */
    yuint32 recode; /* �¼���Ӧ�� */
    yuint16 udsc_id; /* UDS �ͻ���ID�����ڱ�ʶ���ĸ�UDS�ͻ��� */
    yuint32 tv_sec; /* ʱ��� s */
    yuint32 tv_usec; /* ʱ��� us */
} ipc_common_msg;

/* ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define DM_EVENT_UDS_CLIENT_CREATE_EMIT
#define DM_EVENT_UDS_CLIENT_CREATE_ACCEPT
     ---------------------------------------
    |       n nyte     |        nbyte       |  
     ---------------------------------------
    | ipc common header| json format config | 
     ---------------------------------------
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ */
#define C_KEY_SERVICE_ITEM_CAPACITY   "ServiceItemCapacity"
#define C_KEY_UDSC_NAME               "UDSClientName"
#define C_KEY_UDS_MSG_PARSE           "UDSMsgParseEnable"
#define C_KEY_UDS_ASC_RECORD          "UDSASCRecordEnable"

int ydm_udsc_create_config_encode(yuint8 *buff, yuint32 size, udsc_create_config *config);

int ydm_udsc_create_config_decode(yuint8 *buff, yuint32 size, udsc_create_config *config);
/* ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define DM_EVENT_UDS_SERVICE_ITEM_ADD_EMIT
#define DM_EVENT_UDS_SERVICE_ITEM_ADD_ACCEPT
     ---------------------------------------
    |       n nyte     |        nbyte       |  
     ---------------------------------------
    | ipc common header| json format config | 
     ---------------------------------------
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ */
#define C_KEY_SI_ID                         "ServiceID"
#define C_KEY_SI_ID_38_FRT                  "RequestFileTransfer"
#define C_KEY_SI_ID_38_MOO                  "ModeOfOperation"
#define C_KEY_SI_ID_38_FPANL                "FilePathAndNameLength"
#define C_KEY_SI_ID_38_FPAN                 "FilePathAndName"
#define C_KEY_SI_ID_38_DFI                  "DataFormatIdentifier"
#define C_KEY_SI_ID_38_FSPL                 "fileSizeParameterLength"
#define C_KEY_SI_ID_38_FSUC                 "FileSizeUnCompressed"
#define C_KEY_SI_ID_38_FSC                  "FileSizeCompressed"
#define C_KEY_SI_ID_34_RD                   "RequestDownload"
#define C_KEY_SI_ID_34_DFI                  "DataFormatIdentifier"
#define C_KEY_SI_ID_34_AALFI                "AddressAndLengthFormatIdentifier"
#define C_KEY_SI_ID_34_MA                   "MemoryAddress"
#define C_KEY_SI_ID_34_MS                   "MemorySize"
#define C_KEY_SI_SUB                        "SubFunction"
#define C_KEY_SI_DID                        "DataIdentifier"
#define C_KEY_SI_ITEM_DELAY_TIME            "DelayTime"
#define C_KEY_SI_TIMEOUT                    "Timeout"
#define C_KEY_SI_IS_SUPPRESS                "IsSuppress"
#define C_KEY_SI_TA_TYPE                    "TaType"
#define C_KEY_SI_TA                         "Ta" 
#define C_KEY_SI_SA                         "Sa"
#define C_KEY_SI_REQUEST_BYTE               "RequestByte"
#define C_KEY_SI_VARIABLE_BYTE              "VariableByte" 
#define C_KEY_SI_EXPECT_RESPONSE_BYTE       "ExpectResponseByte"
#define C_KEY_SI_EXPECT_RESPONSE_RULE       "ExpectResponseRule"
#define C_KEY_SI_FINISH_RESPONSE_BYTE       "FinishByte"
#define C_KEY_SI_FINISH_RESPONSE_RULE       "FinishRule"
#define C_KEY_SI_FINISH_RESPONSE_TIMEOUT    "FinishTimeout"
#define C_KEY_SI_FINISH_RESPONSE_TRYMAX     "FinishTryMax"
#define C_KEY_SI_DESC                       "ServiceDesc"
#define C_KEY_SI_LOCAL_FILE_PATH            "LocalFilePath"
#define C_KEY_SI_REQUEST_RESULT_CALL_ID     "RequestResultCallId"
#define C_KEY_SI_ID_36_TD                   "TransferData"
#define C_KEY_SI_MAX_NUMBER_OF_BLOCK_LEN    "MaxNumberOfBlockLength"

int ydm_service_config_encode(yuint8 *buff, yuint32 size, service_config *config);

int ydm_service_config_decode(yuint8 *buff, yuint32 size, service_config *config);
/* ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define DM_EVENT_UDS_CLIENT_GENERAL_CFG_EMIT
#define DM_EVENT_UDS_CLIENT_GENERAL_CFG_ACCEPT
     ---------------------------------------
    |       n nyte     |        nbyte       |  
     ---------------------------------------
    | ipc common header| json format config | 
     ---------------------------------------
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ */
#define C_KEY_GC_IS_FAILABORT             "IsFailAbort"
#define C_KEY_GC_TP_ENABLE                "TesterPresentEnable"
#define C_KEY_GC_TP_IS_REFRESH            "TesterPresentIsRefresh"
#define C_KEY_GC_TP_INTERVAL              "TesterPresentInterval"
#define C_KEY_GC_TP_TA                    "TesterPresentTa"
#define C_KEY_GC_TP_SA                    "TesterPresentSa"
#define C_KEY_GC_TD_36_NOTIFY_INTERVAL    "36TDNotifyInterval"
#define C_KEY_RUN_STATIS_ENABLE           "RunStatisEnable"

int ydm_general_config_encode(yuint8 *buff, yuint32 size, udsc_runtime_config *config);

int ydm_general_config_decode(yuint8 *buff, yuint32 size, udsc_runtime_config *config);
/* ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define DM_EVENT_DOIP_CLIENT_CREATE_EMIT
#define DM_EVENT_DOIP_CLIENT_CREATE_ACCEPT
     ---------------------------------------
    |       n nyte     |        nbyte       |  
     ---------------------------------------
    | ipc common header| json format config | 
     ---------------------------------------
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ */
#define C_KEY_DOIPC_VERSION      "Version"
#define C_KEY_DOIPC_SA           "SourceAddr"
#define C_KEY_DOIPC_SRC_IP       "SourceIP"
#define C_KEY_DOIPC_SRC_PORT     "SourcePort"
#define C_KEY_DOIPC_DST_IP       "DestIP"
#define C_KEY_DOIPC_DST_PORT     "DestPort"
#define C_KEY_DOIPC_RX_LEN       "RxLen"
#define C_KEY_DOIPC_TX_LEN       "TxLen"

int ydm_doipc_create_config_encode(yuint8 *buff, yuint32 size, doipc_config_t *config);

int ydm_doipc_create_config_decode(yuint8 *buff, yuint32 size, doipc_config_t *config);
/* ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define DM_EVENT_UDS_CLIENT_RUNTIME_DATA_STATIS_EMIT
#define DM_EVENT_UDS_CLIENT_RUNTIME_DATA_STATIS_ACCEPT
     ---------------------------------------
    |       n nyte     |        nbyte       |  
     ---------------------------------------
    | ipc common header| json format config | 
     ---------------------------------------
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ */
#define C_KEY_SENT_TOTAL_BYTES     "SentTotalBytes"
#define C_KEY_RECV_TOTAL_BYTES     "RecvTotalBytes"
#define C_KEY_SENT_PEAK_SPEED      "SentPeakSpeed"
#define C_KEY_RECV_PEAK_SPEED      "RecvPeakSpeed"
#define C_KEY_SENT_VALLEY_SPEED    "SentValleySpeed"
#define C_KEY_RECV_VALLEY_SPEED    "RecvValleySpeed"
#define C_KEY_RUN_TIME_TOTAL       "RunTimeTotal"
#define C_KEY_SENT_PACKET_NUM      "SentPacketNumber"
#define C_KEY_RECV_PACKET_NUM      "RecvPacketNumber"

int ydm_udsc_rundata_statis_encode(yuint8 *buff, yuint32 size, runtime_data_statis_t *statis);

int ydm_udsc_rundata_statis_decode(yuint8 *buff, yuint32 size, runtime_data_statis_t *statis);
/* ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define DM_EVENT_CONNECT_DIAG_MASTER_EMIT
#define DM_EVENT_CONNECT_DIAG_MASTER_ACCEPT
     ---------------------------------------
    |       n nyte     |        nbyte       |  
     ---------------------------------------
    | ipc common header| json format config | 
     ---------------------------------------
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ */
#define C_KEY_CM_KEEPALIVE_INTER     "KeepaliveInter"
#define C_KEY_CM_CALL_PATH           "CallbackPath"
#define C_KEY_CM_INFO_PATH           "InfoPath"
typedef struct omapi_connect_master_s {
    yuint32 keepalive_inter;
    char event_listen_path[IPC_PATH_LEN];
    char event_emit_path[IPC_PATH_LEN];
} omapi_connect_master_t;
int ydm_omapi_connect_master_encode(yuint8 *buff, yuint32 size, omapi_connect_master_t *cf);

int ydm_omapi_connect_master_decode(yuint8 *buff, yuint32 size, omapi_connect_master_t *cf);

/* ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define DM_EVENT_TERMINAL_CONTROL_SERVICE_CREATE_EMIT
#define DM_EVENT_TERMINAL_CONTROL_SERVICE_CREATE_ACCEPT
     ---------------------------------------
    |       n nyte     |        nbyte       |  
     ---------------------------------------
    | ipc common header| json format config | 
     ---------------------------------------
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ */
#define KEY_TR_DEV_ID          "DeviceIdentification"
#define KEY_TR_PROVINCE        "Province"
#define KEY_TR_CITY            "City"
#define KEY_TR_MANUFACTURER    "Manufacturer"
#define KEY_TR_MODEL           "Model"
#define KEY_TR_TER_ID          "TerminalID"
#define KEY_TR_COLOR           "Color"
#define KEY_TR_VIN             "VIN"
#define KEY_TR_SRV_ADDR        "ServerAddress"
#define KEY_TR_SRV_PORT        "ServerPort"
#define KEY_TR_IMEI            "IMEI"
#define KEY_TR_APP_VERSION     "AppVersion"

int ydm_terminal_control_service_info_encode(yuint8 *buff, yuint32 size, terminal_control_service_info_t *trinfo);

int ydm_terminal_control_service_info_decode(yuint8 *buff, yuint32 size, terminal_control_service_info_t *trinfo);

/* ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    --------------------------------------------------------------------------------------------------------------------------
   |       n nyte     |       4byte         |     4byte      |      4byte     |  4byte  |        4byte      |      nbyte      |
    --------------------------------------------------------------------------------------------------------------------------
   | ipc common header| diag type ip or can | target address | source address | ta type | request data len  |  request data   |
    --------------------------------------------------------------------------------------------------------------------------
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ */
/* struct service_indication_s */
#define SERVICE_INDICATION_MTYPE_OFFSET (0)
#define SERVICE_INDICATION_MTYPE_SIZE (4)

#define SERVICE_INDICATION_A_SA_OFFSET (SERVICE_INDICATION_MTYPE_OFFSET + SERVICE_INDICATION_MTYPE_SIZE)
#define SERVICE_INDICATION_A_SA_SIZE (4)

#define SERVICE_INDICATION_A_TA_OFFSET (SERVICE_INDICATION_A_SA_OFFSET + SERVICE_INDICATION_A_SA_SIZE)
#define SERVICE_INDICATION_A_TA_SIZE (4)

#define SERVICE_INDICATION_TA_TYPE_OFFSET (SERVICE_INDICATION_A_TA_OFFSET + SERVICE_INDICATION_A_TA_SIZE)
#define SERVICE_INDICATION_TA_TYPE_SIZE (4)

#define SERVICE_INDICATION_PAYLOAD_LEN_OFFSET (SERVICE_INDICATION_TA_TYPE_OFFSET + SERVICE_INDICATION_TA_TYPE_SIZE)
#define SERVICE_INDICATION_PAYLOAD_LEN_SIZE (4)

#define SERVICE_INDICATION_PAYLOAD_OFFSET (SERVICE_INDICATION_PAYLOAD_LEN_OFFSET + SERVICE_INDICATION_PAYLOAD_LEN_SIZE)

typedef struct ipc_service_indication_s {
    yuint32 Mtype;
    yuint32 A_SA;
    yuint32 A_TA;
    yuint32 TA_TYPE;    
    int payload_length;
    yuint8 payload[0];
} ipc_service_indication_t;

/* ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    --------------------------------------------------------------------------------------------------------------------------
   |       n nyte     |       4byte         |     4byte      |      4byte     |  4byte  |        4byte      |      nbyte      |
    --------------------------------------------------------------------------------------------------------------------------
   | ipc common header| diag type ip or can | target address | source address | ta type | response data len |  response data  |
    --------------------------------------------------------------------------------------------------------------------------
  \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ */
/* struct service_response_s */
#define SERVICE_RESPONSE_MTYPE_OFFSET (0)
#define SERVICE_RESPONSE_MTYPE_SIZE (4)

#define SERVICE_RESPONSE_A_SA_OFFSET (SERVICE_RESPONSE_MTYPE_OFFSET + SERVICE_RESPONSE_MTYPE_SIZE)
#define SERVICE_RESPONSE_A_SA_SIZE (4)

#define SERVICE_RESPONSE_A_TA_OFFSET (SERVICE_RESPONSE_A_SA_OFFSET + SERVICE_RESPONSE_A_SA_SIZE)
#define SERVICE_RESPONSE_A_TA_SIZE (4)

#define SERVICE_RESPONSE_TA_TYPE_OFFSET (SERVICE_RESPONSE_A_TA_OFFSET + SERVICE_RESPONSE_A_TA_SIZE)
#define SERVICE_RESPONSE_TA_TYPE_SIZE (4)

#define SERVICE_RESPONSE_PAYLOAD_LEN_OFFSET (SERVICE_RESPONSE_TA_TYPE_OFFSET + SERVICE_RESPONSE_TA_TYPE_SIZE)
#define SERVICE_RESPONSE_PAYLOAD_LEN_SIZE (4)

#define SERVICE_RESPONSE_PAYLOAD_OFFSET (SERVICE_RESPONSE_PAYLOAD_LEN_OFFSET + SERVICE_RESPONSE_PAYLOAD_LEN_SIZE)

typedef struct ipc_service_response_s {
    yuint32 Mtype;
    yuint32 A_SA;
    yuint32 A_TA;
    yuint32 TA_TYPE;    
    int payload_length;
    yuint8 payload[0];
} ipc_service_response_t;
/* ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    --------------------------------------------------------------------
   |       n nyte     |       4byte      |     4byte       |    nbyte   |
    --------------------------------------------------------------------
   | ipc common header| callback func id | result data len | result data| 
    --------------------------------------------------------------------
  \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ */
#define SERVICE_REQUEST_RESULT_RR_CALLID_OFFSET (DM_IPC_EVENT_MSG_SIZE)
#define SERVICE_REQUEST_RESULT_RR_CALLID_SIZE (4)

#define SERVICE_REQUEST_RESULT_DATA_LEN_OFFSET (SERVICE_REQUEST_RESULT_RR_CALLID_OFFSET + SERVICE_REQUEST_RESULT_RR_CALLID_SIZE)
#define SERVICE_REQUEST_RESULT_DATA_LEN_SIZE (4)

#define SERVICE_REQUEST_RESULT_DATA_OFFSET (SERVICE_REQUEST_RESULT_DATA_LEN_OFFSET + SERVICE_REQUEST_RESULT_DATA_LEN_SIZE)
#define SERVICE_REQUEST_RESULT_DATA_MAX (512)

#define SERVICE_REQUEST_RESULT_SIZE (SERVICE_REQUEST_RESULT_RR_CALLID_SIZE + \
                                     SERVICE_REQUEST_RESULT_DATA_LEN_SIZE)
typedef struct ipc_service_request_result_s {
    yuint32 rr_callid;
    yuint32 dlen;
    yuint8 data[0];
} ipc_service_request_result_t;


/* ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ---------------------------------------------------------------------------------------
   |       n nyte     |       4byte      |     nbyte    |        4byte     |     nbyte     |
    ------------------ ------------------ -------------- ------------------ ---------------
   | ipc common header| request data len | request data | response data len| response data |
    ---------------------------------------------------------------------------------------
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ */
#define SERVICE_FINISH_RESULT_IND_LEN_OFFSET (DM_IPC_EVENT_MSG_SIZE)
#define SERVICE_FINISH_RESULT_IND_LEN_SIZE (4)

#define SERVICE_FINISH_RESULT_IND_OFFSET (SERVICE_FINISH_RESULT_IND_LEN_OFFSET + SERVICE_FINISH_RESULT_IND_LEN_SIZE)

#define SERVICE_FINISH_RESULT_RESP_LEN_SIZE (4)
typedef struct ipc_service_finish_result_s {
    yuint32 indl;
    yuint8 ind[1];    
    yuint32 respl;
    yuint8 resp[1];
} ipc_service_finish_result_t;

/* ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    --------------------------------------------------------
   |       n nyte     |   1byte  | 2byte  | nbyte           |
    ------------------ ---------- -------- -----------------
   | ipc common header| SA level |seed len| seed data array |
    --------------------------------------------------------
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ */
#define SERVICE_SA_SEED_LEVEL_OFFSET (DM_IPC_EVENT_MSG_SIZE)
#define SERVICE_SA_SEED_LEVEL_SIZE (1)

#define SERVICE_SA_SEED_SIZE_OFFSET (SERVICE_SA_SEED_LEVEL_OFFSET + SERVICE_SA_SEED_LEVEL_SIZE)
#define SERVICE_SA_SEED_SIZE_SIZE (2)

#define SERVICE_SA_SEED_OFFSET (SERVICE_SA_SEED_SIZE_OFFSET + SERVICE_SA_SEED_SIZE_SIZE)
typedef struct ipc_service_sa_seed_s {
    yuint16 seed_size;
    yuint8 seed[0];
} ipc_service_sa_seed_t;

/* ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    --------------------------------------------------------
   |       n nyte     |   1byte  | 2byte  | nbyte           |
    ------------------ ---------- -------- -----------------
   | ipc common header| SA level |key len | key data array  |
    --------------------------------------------------------
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ */
#define SERVICE_SA_KEY_LEVEL_OFFSET (DM_IPC_EVENT_MSG_SIZE)
#define SERVICE_SA_KEY_LEVEL_SIZE (1)

#define SERVICE_SA_KEY_SIZE_OFFSET (SERVICE_SA_KEY_LEVEL_OFFSET + SERVICE_SA_KEY_LEVEL_SIZE)
#define SERVICE_SA_KEY_SIZE_SIZE (2)

#define SERVICE_SA_KEY_OFFSET (SERVICE_SA_KEY_SIZE_OFFSET + SERVICE_SA_KEY_SIZE_SIZE)
typedef struct ipc_service_sa_key_s {
    yuint16 key_size;
    yuint8 key[0];
} ipc_service_sa_key_t;

/* ota master api��������ota master */
/* ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    -------------------------------------------------------------------------------------------------
   |       n nyte     |         4byte      |            4byte             |     nbyte                |
    ------------------ -------------------- ------------------------------ --------------------------
   | ipc common header| keepalive interval | ota master api unix path len | ota master api unix path | 
    -------------------------------------------------------------------------------------------------
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ */
#define DMREQ_CONNECT_KEEPALIVE_INTERVAL_OFFSET (DM_IPC_EVENT_MSG_SIZE)
#define DMREQ_CONNECT_KEEPALIVE_INTERVAL_SIZE (4)

#define DMREQ_CONNECT_OMAPI_PATH_LEN_OFFSET (DMREQ_CONNECT_KEEPALIVE_INTERVAL_OFFSET + DMREQ_CONNECT_KEEPALIVE_INTERVAL_SIZE)
#define DMREQ_CONNECT_OMAPI_PATH_LEN_SIZE (4)

#define DMREQ_CONNECT_OMAPI_PATH_OFFSET (DMREQ_CONNECT_OMAPI_PATH_LEN_OFFSET + DMREQ_CONNECT_OMAPI_PATH_LEN_SIZE)

#define DMREQ_CONNECT_OMAPI_MSG_MIN_SIZE (DM_IPC_EVENT_MSG_SIZE + DMREQ_CONNECT_KEEPALIVE_INTERVAL_SIZE +DMREQ_CONNECT_OMAPI_PATH_LEN_SIZE )
typedef struct ipc_diag_master_connect_request_s {
    yuint32 keepalive_inter;
    yuint32 path_len;
    char path[0];
} ipc_diag_master_connect_request_t;

/* ota master ��Ӧota master api���������� */
/* ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    --------------------------------------------------------------------
   |       n nyte     |            4byte         |     nbyte            |
    ------------------ -------------------------- ----------------------
   | ipc common header| ota master unix path len | ota master unix path | 
    --------------------------------------------------------------------
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ */
#define DMREP_CONNECT_DM_PATH_LEN_OFFSET (DM_IPC_EVENT_MSG_SIZE)
#define DMREP_CONNECT_DM_PATH_LEN_SIZE (4)
    
#define DMREP_CONNECT_DM_PATH_OFFSET (DMREP_CONNECT_DM_PATH_LEN_OFFSET + DMREP_CONNECT_DM_PATH_LEN_SIZE)

#define DMREQ_CONNECT_DM_MSG_MIN_SIZE (DM_IPC_EVENT_MSG_SIZE + DMREP_CONNECT_DM_PATH_LEN_SIZE)
typedef struct ipc_diag_master_connect_reply_s {
    yuint32 path_len;
    char path[0];
} ipc_diag_master_connect_reply_t;

/* ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    -----------------------------------
   |       n nyte     |     1byte      |  
    ------------------ ---------------- 
   | ipc common header| doip client id | 
    -----------------------------------
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ */
#define DOIPC_CREATE_DOIP_CLIENT_ID_OFFSET (DM_IPC_EVENT_MSG_SIZE)
#define DOIPC_CREATE_DOIP_CLIENT_ID_SIZE (4)

#define DOIPC_CREATE_REPLY_SIZE (DM_IPC_EVENT_MSG_SIZE + DOIPC_CREATE_DOIP_CLIENT_ID_SIZE)
typedef struct ipc_doipc_create_reply_s {
    yuint32 id;
} ipc_doipc_create_reply_t;

/* ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    -----------------------------------
   |       n nyte     |     4byte      |  
    ------------------ ---------------- 
   | ipc common header| udsc is active | 
    -----------------------------------
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ */
#define UDSC_IS_ACTIVE_OFFSET (DM_IPC_EVENT_MSG_SIZE)
#define UDSC_IS_ACTIVE_SIZE (4)

#define UDSC_IS_ACTIVE_REPLY_SIZE (DM_IPC_EVENT_MSG_SIZE + UDSC_IS_ACTIVE_SIZE)
typedef struct ipc_udsc_active_reply_s {
    yuint32 isactive;
} ipc_udsc_active_reply_t;

/* ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ---------------------------------------
   |       n nyte     |        1byte       |  
    ------------------ -------------------- 
   | ipc common header| terminal remote id | 
    ---------------------------------------
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ */
#define TERMINAL_CONTROL_SERVICE_ID_OFFSET (DM_IPC_EVENT_MSG_SIZE)
#define TERMINAL_CONTROL_SERVICE_ID_SIZE (4)

#define TERMINAL_CONTROL_SERVICE_SIZE (DM_IPC_EVENT_MSG_SIZE + TERMINAL_CONTROL_SERVICE_ID_SIZE)
typedef struct ipc_terminal_control_service_s {
    yuint32 id;
} ipc_terminal_control_service_t;

/* ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ---------------------------------------
   |       n nyte     |        1byte       |  
    ------------------ -------------------- 
   | ipc common header| terminal remote id | 
    ---------------------------------------
\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\ */
#define IPC_36_TRANSFER_FILE_SIZE_OFFSET (DM_IPC_EVENT_MSG_SIZE)
#define IPC_36_TRANSFER_FILE_SIZE_SIZE   (4)

#define IPC_36_TRANSFER_TOTAL_BYTE_OFFSET (IPC_36_TRANSFER_FILE_SIZE_OFFSET + IPC_36_TRANSFER_FILE_SIZE_SIZE)
#define IPC_36_TRANSFER_TOTAL_BYTE_SIZE   (4)

#define IPC_36_TRANSFER_ELAPSED_TIME_OFFSET (IPC_36_TRANSFER_TOTAL_BYTE_OFFSET + IPC_36_TRANSFER_TOTAL_BYTE_SIZE)
#define IPC_36_TRANSFER_ELAPSED_TIME_SIZE   (4)

#define IPC_36_TRANSFER_PROGRESS_MSG_SIZE (DM_IPC_EVENT_MSG_SIZE + \
                                            IPC_36_TRANSFER_FILE_SIZE_SIZE + \
                                            IPC_36_TRANSFER_TOTAL_BYTE_SIZE + \
                                            IPC_36_TRANSFER_ELAPSED_TIME_SIZE)
typedef struct ipc_36_transfer_progress_s {
    yuint32 file_size; /* �ļ���С */
    yuint32 total_byte; /* �Ѵ����ֽ����� */
    yuint32 elapsed_time; /* unit ms */
} ipc_36_transfer_progress_t;

YByteArray *YByteArrayNew();

void YByteArrayDelete(YByteArray *arr);

void YByteArrayClear(YByteArray *arr);

const yuint8 *YByteArrayConstData(YByteArray *arr);

int YByteArrayCount(YByteArray *arr);

void YByteArrayAppendChar(YByteArray *dest, yuint8 c);

void YByteArrayAppendArray(YByteArray *dest, YByteArray *src);

void YByteArrayAppendNChar(YByteArray *dest, yuint8 *c, yuint32 count);

int YByteArrayEqual(YByteArray *arr1, YByteArray *arr2);

int YByteArrayCharEqual(YByteArray *arr1, yuint8 *c, yuint32 count);

int ydm_recvfrom(int sockfd, yuint8 *buff, yuint32 size, yuint32 toms);

int ydm_tv_get(yuint32 *tv_sec, yuint32 *tv_usec);
int ydm_tv_currtoms(yuint32 tv_sec, yuint32 tv_usec);
int ydm_common_ipc_tv_reset(yuint8 *buff, yuint32 size);
int ydm_common_is_system_time_stamp(yuint32 tv_sec, yuint32 tv_usec);
int ydm_common_encode(yuint8 *buff, yuint32 size, yuint32 evtype, yuint32 recode, yuint16 id, yuint32 tv_sec, yuint32 tv_usec);
int ydm_common_decode(yuint8 *buff, yuint32 size, yuint32 *evtype, yuint32 *recode, yuint16 *id, yuint32 *tv_sec, yuint32 *tv_usec);
char *ydm_ipc_event_str(yuint32 evtype);
char *ydm_event_rcode_str(int rcode);

int ydm_ipc_channel_create(void *addr, int rbuf_size, int sbuf_size);

yuint32 ydm_common_hex2bin(yuint8 *bin, char *hex, yuint32 binlength);

yuint32 ydm_common_nhex2bin(yuint8 *bin, char *hex, yuint32 hexlen, yuint32 binlength);

int ydm_common_is_hex_digit(char c);

#endif /* __YDM_IPC_COMMON_H__ */
