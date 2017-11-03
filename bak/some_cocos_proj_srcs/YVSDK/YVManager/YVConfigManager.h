#ifndef _YVSDK_YVCONFIGURE_H_
#define _YVSDK_YVCONFIGURE_H_
#include <map>
#include <vector>
#include "YVType/YVType.h"
#include "YVManager/YVManager.h"
#include "YVUtils/YVMsgDispatcher.h"

namespace YVSDK
{
	//�����࿪
	enum  ConfigType
	{
							  //����˵��      �Ƿ��ѡ     ����      ����
		ConfigAppid,          //AppId����      ����      int��      һ��
		ConfigChannelKey,     //Ƶ����ʶ        ��ѡ     �ı���     0�����
		ConfigTempPath,       //ѽѽ�����ŵ�λ��
		ConfigIsTest,		  //�Ƿ��ǲ��Ի���
		ConfigServerId,		  //��Ϸ����ID��
		ConfigReadstatus,	  //��Ϣ�Ƿ���Ҫȷ��

		ConfigFriendChatCacheNum,     //���������ڴ滺������; 
		ConfigFriendHistoryChatNum,   //ÿ��������������ʷ��Ϣ����;

		ConfigChannelChatCacheNum,     //���������ڴ滺������; 
		ConfigChannelHistoryChatNum,   //ÿ��������������ʷ��Ϣ����;

		ConfigSpeechWhenSend,   //������Ϣ�Ƿ���ʶ����ı����ٷ���
	};

	class YVConfigManager
	{
		friend YVMsgDispatcher;
		friend YVPlayerManager;
		friend YVChannalChatManager;
		friend YVFriendChatManager;
		friend YVFileManager;
		friend _YVFilePath;

		public:
			YVConfigManager();
			void  setConfig(ConfigType type, ...);

            //: Shrimps
            void clearChannelKeys() {
                channelKey.clear();
            }
            //.

		private:
			uint32 appid;                         //appid
			std::vector<std::string> channelKey;  //Ƶ��key
			std::string tempPath;				  //���޲����Ļ����ļ���ַ
			bool isTest;						  //�ǿ������Ի���
			bool readstatus;					  //��Ϣ�Ƿ���Ҫ�Ѿ���ʶ
			std::string serverid;				  //������Ƶ��id
			
			uint32 friendChatCacheNum;             //�ڴ滺��ĺ�����Ϣ���� 
			uint32 channelChatCacheNum;            //�ڴ滺���Ƶ����Ϣ����

			uint32 friendHistoryChatNum;           //ÿ����ȡ�ĺ�����ʷ��Ϣ���� 
			uint32 channelHistoryChatNum;          //ÿ����ȡ��Ƶ������ʷ��Ϣ���� 

			uint32 isClearErroMsg;                 //����ʧ�ܵ���Ϣ�����·��ͣ��Ƿ�����������Ϣ
			bool speechWhenSend;
	};
}
#endif