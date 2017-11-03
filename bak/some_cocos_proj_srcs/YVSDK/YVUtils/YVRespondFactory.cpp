/*********************************************************************************
*Copyright(C), 2015 YUNVA Company
*FileName:  RespondFactory.cpp
*Author:  Ԫ��
*Version:  1.0.0
*Date:  2015-5-5
*Description:  ����Э��ĵ��ýṹ��
**********************************************************************************/

#include "YVRespondFactory.h"
#include "YVProtocol/YVProtocol.h"
namespace YVSDK
{
	RespondFactory* RespondFactory::m_respondFactoryPtr = NULL;

	//�����ʺŵ�¼��Ӧ
	AutoRegisterRespond(IM_LOGIN_RESP, LoginResponse);
	//CP�ʺŵ�¼��Ӧ
	AutoRegisterRespond(IM_THIRD_LOGIN_RESP, CPLoginResponce);

	AutoRegisterRespond(IM_FRIEND_ADD_RESP, AddFriendRepond);
	AutoRegisterRespond(IM_FRIEND_ADD_NOTIFY, AddFriendNotify);
	AutoRegisterRespond(IM_FRIEND_ACCEPT_RESP, FriendAcceptRespond);
	//������ɾ��֪ͨ������Ҫ��������ĺ����б����Ƴ���
	AutoRegisterRespond(IM_FRIEND_DEL_NOTIFY, DelFriendNotify);
	AutoRegisterRespond(IM_FRIEND_RECOMMAND_RESP, RecommendFriendRespond);
	AutoRegisterRespond(IM_FRIEND_SEARCH_RESP, SearchFriendRespond);
	AutoRegisterRespond(IM_FRIEND_OPER_RESP, BlackControlRespond);
	//����ɾ����Ӧ
	AutoRegisterRespond(IM_FRIEND_DEL_RESP, DelFriendRespond);
	//�����б�����
	AutoRegisterRespond(IM_FRIEND_LIST_NOTIFY, FriendListNotify);
	//����������
	AutoRegisterRespond(IM_FRIEND_BLACKLIST_NOTIFY, BlackListNotify);
	//����״̬֪ͨ
	AutoRegisterRespond(IM_FRIEND_STATUS_NOTIFY, FriendStatusNotify);
	//AutoRegisterRespond(IM_CLOUDMSG_NOTIFY, CloundMsgNotify);
	//������Ϣ֪ͨ
	AutoRegisterRespond(IM_CHAT_FRIEND_NOTIFY, FriendChatNotify);
	//Ƶ����Ϣ֪ͨ
	AutoRegisterRespond(IM_CHANNEL_MESSAGE_NOTIFY, ChannelMessageNotify);
	//Ƶ����ʷ��Ϣ
	AutoRegisterRespond(IM_CHANNEL_HISTORY_MSG_RESP, ChannelHistoryNotify);
	//¼��������ַ֪ͨ
	AutoRegisterRespond(IM_RECORD_STOP_RESP, RecordStopNotify);
	//�����������	
	AutoRegisterRespond(IM_RECORD_FINISHPLAY_RESP, StartPlayVoiceRespond);
	//ֹͣ����ʶ���Ӧ
	AutoRegisterRespond(IM_SPEECH_STOP_RESP, SpeechStopRespond);
	//�����ļ���Ӧ
	AutoRegisterRespond(IM_DOWNLOAD_FILE_RESP, DownLoadFileRespond);
	//����Ϣ����
	AutoRegisterRespond(IM_CLOUDMSG_NOTIFY, CloundMsgNotify);
	//����Ϣ���󷵻�
	AutoRegisterRespond(IM_CLOUDMSG_LIMIT_RESP, CloundMsgRepond);
	//����Ϣ�������ͷ���
	AutoRegisterRespond(IM_CLOUDMSG_LIMIT_NOTIFY, CloundMsgLimitNotify);
	//�û�������Ϣ��ȡ 
	AutoRegisterRespond(IM_USER_GETINFO_RESP, GetUserInfoFriendRespond);
	//����ʵ��
	AutoRegisterRespond(IM_RECONNECTION_NOTIFY, ReconnectionNotify);
	//Ƶ����Ϣ״̬
	AutoRegisterRespond(IM_CHANNEL_SENDMSG_RESP, ChannelMessageStateNotify);
	//������Ϣ״̬
	AutoRegisterRespond(IM_CHATT_FRIEND_RESP, FriendMsgStateNotify);
};
