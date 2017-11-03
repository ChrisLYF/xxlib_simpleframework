#ifndef _FRIENDCHATMANAGER_H_
#define _FRIENDCHATMANAGER_H_
#include <string>
#include "YVType/YVType.h"
#include "YVListern/YVListern.h"

namespace YVSDK
{
#define LINITMSGNUM  10

	struct YaYaRespondBase;
	class YVFriendChatManager 
		:public YVListern::YVFinishSpeechListern
	{
	public:
		bool init();
		bool destory();

		bool sendFriendText(uint32 chatWithUid, std::string text);

		bool sendFriendVoice(uint32 chatWithUid, YVFilePathPtr voicePath,
			uint32 voiceTime);

		//����ͼƬ��Ϣ��Ŀǰ����֧��// 
		bool sendFriendImage(uint32 chatWithUid, YVFilePathPtr path);

		//�����һ����Ϣ��һ��
		bool getFriendChatHistoryData(uint32 chatWithUid, int index);
		
		//��������֪ͨ
		void friendMessageNotifyCallback(YaYaRespondBase*);
		
		//����Ϣ֪ͨ//
		void cloundMsgNotifyCallback(YaYaRespondBase*);

		//��Ϣ����״̬֪ͨ
		void friendMessageStateCallback(YaYaRespondBase*);


		YVMessageListPtr getFriendChatListById(uint32 uid);

		InitListern(FriendHistoryChat, YVMessageListPtr);
		InitListern(FriendChat, YVMessagePtr);
		InitListern(FriendChatState, YVMessagePtr);
	protected:
		//����ʶ��ӿ�
		void onFinishSpeechListern(SpeechStopRespond*);

		//������Ϣ, isCallFriendChatListern�Ǹ���ʷ ��Ϣʹ�õģ���ʷ��Ϣ���Լ��Ļص�//
		void insertMessage(uint32 chatWithId, YVMessagePtr messageBase, bool isCallFriendChatListern = true);
		//������Ϣ����//
		FriendChatMessageMap m_friendChatMap; 
		//�����е���Ϣ����//
		YVMessageListPtr m_sendMsgCache; 
		//������ʷ��Ϣ����,������ȡ��ʷ��Ϣʹ��
		FriendChatMessageMap m_historyCache;
	};
}
#endif