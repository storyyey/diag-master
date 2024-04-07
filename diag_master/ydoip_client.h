#ifndef __YDOIP_CLIENT_H__
#define __YDOIP_CLIENT_H__

typedef void (*doipc_diagnostic_response)(void *arg, yuint16 sa, yuint16 ta, yuint8 *msg, yuint32 len);

typedef enum doipc_tcp_status_e {
    DOIPC_TCP_DISCONNECT,
    DOIPC_TCP_CONNECT_PROCESS,
    DOIPC_TCP_CONNECT_SUCCESS,
} doipc_tcp_status_e;

typedef struct doip_client_s {
    struct ev_loop *loop; /* ���¼�ѭ���ṹ�� */
    ev_io tcprecv_watcher; /* ����TCP���¼� */
    ev_io tcpsend_watcher; /* ����TCPд�¼� */
    int index; /* ������� */
    int udp_sfd; /* UDP socket */
    int tcp_sfd; /* TCP socket */
    yuint8 *rxbuf; /* ����Buff */
    yuint32 rxlen; /* ����Buff���� */
    yuint8 *txbuf; /* ����Buff */
    yuint32 txlen; /* ����buff���� */
    doipc_tcp_status_e con_stat; /* tcp����״̬ */
    int isactive; /* �Ƿ��Ѿ�·�ɼ��� */
    doipc_diagnostic_response response_call;
    void *ddr_arg;
    doipc_config_t config;
} doip_client_t;

int ydoipc_diagnostic_request(doip_client_t *doipc, yuint16 sa, yuint16 ta, const yuint8 *msg, yuint32 len, doipc_diagnostic_response call, void *arg);

int ydoipc_disconnect_server(doip_client_t *doipc);

int ydoipc_connect_active_server(doip_client_t *doipc, doipc_config_t *config);

doip_client_t *ydoipc_create(struct ev_loop *loop);

int ydoipc_destroy(doip_client_t *doipc);


#endif /* __YDOIP_CLIENT_H__ */
