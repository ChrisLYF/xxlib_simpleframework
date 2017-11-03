#ifndef _YVSDK_YVLISTERN_H_
#define _YVSDK_YVLISTERN_H_
#include <iostream>
#include <algorithm>
#include <list>
#include "YVType/YVType.h"
#include "YVProtocol/YVProtocol.h"

namespace YVSDK
{
	//========================================================================================
#define InitListern(Name, ArgcType)  \
public:  \
	typedef YVListern::YV##Name##Listern Name##Lister;  \
	void add##Name##Listern(Name##Lister* lister)  \
	{ \
	del##Name##Listern(lister);   \
	m_##Name##listerList.push_back(lister); \
} \
	void del##Name##Listern(Name##Lister* lister)  \
	{  \
	std::list<Name##Lister* >::iterator iter = m_##Name##listerList.begin(); \
	for (;iter != m_##Name##listerList.end(); ++iter) {if ((*iter) == lister)break;} \
	if (iter != m_##Name##listerList.end()) \
	{ \
	m_##Name##listerList.erase(iter); \
} \
} \
	void call##Name##Listern(ArgcType t) \
	{ \
	std::list<Name##Lister* >::iterator _listenerItr = m_##Name##listerList.begin(); \
	while (_listenerItr != m_##Name##listerList.end())  \
	{  \
	Name##Lister* lister = *_listenerItr; \
	++_listenerItr; \
	lister->on##Name##Listern(t); \
}  \
} \
private: \
	std::list<Name##Lister* > m_##Name##listerList;

	//========================================================================================

	class YVListern
	{
	public:

		//CP��¼�¼� 
		class YVCPLoginListern
		{
		public:
			virtual void onCPLoginListern(CPLoginResponce*) = 0;
		};

		//ѽѽ�ʺŵ�¼�¼�
		class YVYYLoginListern
		{
		public:
			virtual void onYYLoginListern(LoginResponse*) = 0;
		};

		//�����б��¼� 
		class YVFriendListListern
		{
		public:
			virtual void onFriendListListern(FriendListNotify*) = 0;
		};
		
		//�������б��¼�
		class YVBlackListListern
		{
		public:
			virtual void onBlackListListern(BlackListNotify*) = 0;
		};
	    
		//��Ӻ���
		class YVAddFriendListern
		{
		public:
			virtual void onAddFriendListern(YVUInfoPtr) = 0;
		};

		//�Ƴ�����
		class YVDelFriendListern
		{
		public:
			virtual void onDelFriendListern(YVUInfoPtr) = 0;
		};

		//����ӶԷ�Ϊ����
		class YVBegAddFriendListern
		{
		public:
			virtual void onBegAddFriendListern(YVBegFriendNotifyPtr) = 0;
		};

		//��Ӻ��ѵĽ��(һ������������һ���Ǳ�����)
		class YVAddFriendRetListern
		{
		public:
			virtual void onAddFriendRetListern(YVAddFriendRetPtr) = 0;
		};

		//��Ӻ�ɫ��
		class YVAddBlackListern
		{
		public:
			virtual void onAddBlackListern(YVUInfoPtr) = 0;
		};

		//�Ƴ�������
		class YVDelBlackListern
		{
		public:
			virtual void onDelBlackListern(YVUInfoPtr) = 0;
		};

		//ˢ���û���Ϣ(�����Ǻ�����Ϣ��Ҳ�п����Ǻ�������Ϣ)
		class YVUpdateUserInfoListern
		{
		public:
			virtual void onUpdateUserInfoListern(YVUInfoPtr) = 0;
		};

		//���ѵ��������
		class YVSearchFriendRetListern
		{
		public:
			virtual void onSearchFriendRetListern(SearchFriendRespond*) = 0;
		};

		//���ѵ��Ƽ����
		class YVRecommendFriendRetListern
		{
		public:
			virtual void onRecommendFriendRetListern(RecommendFriendRespond*) = 0;
		};

		//�����¼� 
		class YVReConnectListern
		{
		public:
			virtual void onReConnectListern(ReconnectionNotify*) = 0;
		};

		//����¼���¼�
		class YVStopRecordListern
		{
		public:
			virtual void onStopRecordListern(RecordStopNotify*) = 0;
		};

		//���ʶ���¼�
		class YVFinishSpeechListern
		{
		public:
			virtual void onFinishSpeechListern(SpeechStopRespond*) = 0;
		};

		//���������¼� 
		class YVFinishPlayListern
		{
		public:
			virtual void onFinishPlayListern(StartPlayVoiceRespond*) = 0;
		};

		//�ϴ��¼� 
		class YVUpLoadFileListern
		{
		public:
			virtual void onUpLoadFileListern(YVFilePathPtr) = 0;
		};

		//�����¼� 
		class YVDownLoadFileListern
		{
		public:
			virtual void onDownLoadFileListern(YVFilePathPtr) = 0;
		};

		//����״̬�¼� 
		class YVNetWorkSateListern
		{
		public:
			virtual void onNetWorkSateListern(NetWorkStateNotify*) = 0;
		};

		////¼�������¼�
		class YVRecordVoiceListern
		{
		public:
			virtual void onRecordVoiceListern(RecordVoiceNotify*) = 0;
		};

		///Ƶ����ʷ��Ϣ�¼�
		class YVChannelHistoryChatListern
		{
		public:
			virtual void onChannelHistoryChatListern(YVMessageListPtr) = 0;
		};
		
		//�յ�Ƶ����Ϣ�¼� 
		class YVChannelChatListern
		{
		public:
			virtual void onChannelChatListern(YVMessagePtr) = 0;
		};

		//������ϢƵ��״̬�¼� 
		class YVChannelChatStateListern
		{
		public:
			virtual void onChannelChatStateListern(YVMessagePtr) = 0;
		};

		//������ʷ��Ϣ
		class YVFriendHistoryChatListern
		{
		public:
			virtual void onFriendHistoryChatListern(YVMessageListPtr) = 0;
		};

		//����ʵʱ��Ϣ
		class YVFriendChatListern
		{
		public:
			virtual void onFriendChatListern(YVMessagePtr) = 0;
		};

		//���ͺ�����Ϣ״̬�¼� 
		class YVFriendChatStateListern
		{
		public:
			virtual void onFriendChatStateListern(YVMessagePtr) = 0;
		};


	};
};
#endif