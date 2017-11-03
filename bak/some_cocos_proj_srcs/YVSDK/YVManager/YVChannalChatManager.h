#ifndef _CHANNALCHATMANAGER_H_
#define _CHANNALCHATMANAGER_H_

#include <vector>
#include "YVListern/YVListern.h"
#include "YVType/YVType.h"

namespace YVSDK
{
#define CHANNALLINITMSGNUM 10

	struct YaYaRespondBase;

	class YVChannalChatManager
		:public YVListern::YVFinishSpeechListern
	{
	public:
		bool init();
		bool destory();

        //��¼Ƶ��
        bool loginChannel(std::string &pgameServiceID, std::vector<std::string> &wildCard);
		
		//�����ı���Ϣ
		bool sendChannalText(int channelKeyId, std::string text);
		
		//����������Ϣ
		bool sendChannalVoice(int channelKeyId, YVFilePathPtr voicePath,
			uint32 voiceTime);
		bool sendChannalVoice(int channelKeyId, YVFilePathPtr voicePath,
			uint32 voiceTime, std::string text);

		//��ȡ��ʷ��Ϣ
		bool getChannalHistoryData(int channelKeyId, int index);

		//��ȡ�ڴ��е����ݡ�
		YVMessageListPtr getCacheChannalChatData(int channelKeyId);

		void channelMessageNotifyCallback(YaYaRespondBase*);
		void channelMessageHistoryCallback(YaYaRespondBase*);
		void channelMessageStateCallback(YaYaRespondBase*);
		
		InitListern(ChannelHistoryChat, YVMessageListPtr);
		InitListern(ChannelChat, YVMessagePtr);
		InitListern(ChannelChatState, YVMessagePtr);

	protected:
		//����ʶ��ӿ�
		void onFinishSpeechListern(SpeechStopRespond*);
		std::string& getChannelKeyById(int id);
		int  getChannelIdByKey(std::string key);
		void insertMsg(int channelKeyId, YVMessagePtr, bool isCallChannelChatListern = true);
		
		ChannelMessageMap m_channelMssages;           //Ƶ����Ϣ����
		YVMessageListPtr m_historyCache;            //Ƶ����ʷ��Ϣ����
		YVMessageListPtr m_sendMsgCache;            //�����е���Ϣ����
	};
}
#endif