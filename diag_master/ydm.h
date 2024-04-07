#ifndef __DM_H__
#define __DM_H__

/* ��forkһ���ӽ��̴�����ҵ�񣬸����̼���ӽ�������״̬��������������������뽫�������� */
#define DM_SELF_MONITOR_ATTR                (0x00000001) 
/* ���ú��ȴ�ϵͳ���ʱ��ͬ����������ҵ��,�����δ���ͬ����Ȼ������ҵ�� */
#define DM_START_WAIT_SYSTEM_TIME_SYNC_ATTR (0x00000010) 

/* ����һ���̴߳�����ҵ�����߳���Ҫ���ֽ��̲��˳� */
int ydiag_master_start(int attr);

/* ֹͣ����ҵ�� */
int ydiag_master_stop();

void ydiag_master_ver_info();

void ydiag_master_debug_enable(int b);

void ydiag_master_log_config_file_set(const char *file);

void ydiag_master_log_save_dir_set(const char *dir);

void ydiag_master_sync_time_wait_set(int s);

#endif /* __DM_H__ */
