#ifndef _YVSDK_YVMESSAGE_H_
#define _YVSDK_YVMESSAGE_H_

#include <string>
#include <vector>
#include <map>
#include "YVDef.h"
#include "YVFilePath.h"
#include "YVUtils/YVUtil.h"

namespace YVSDK
{
	//��Ϣ����
	enum YVMessageType
	{
		YVMessageTypeText,        ///�ı���Ϣ
		YVMessageTypeImage,		 ///ͼƬ��Ϣ
		YVMessageTypeAudio,		 ///������Ϣ
	};

	//��Ϣ״̬
	enum YVMessageStatus
	{
		YVMessageStatusCreated,      ///��Ϣ����(Ĭ��״̬)

		YVMessageStatusUnread,       ///��Ϣδ��
		YVMessageStatusRead,         ///��Ϣ�Ѷ�

		YVMessageStatusSending,      ///��Ϣ������
		YVMessageStatusReSending,    ///��Ϣ���·�����
		YVMessageStatusSendingFailed ///��Ϣ����ʧ��
	};

	//��Ϣ����
	struct _YVMessage
	{
		_YVMessage(YVMessageType inType, YVMessageStatus instate);
		virtual ~_YVMessage();
		uint64 id;                //��Ϣid(�����������ģ�����������Ϣ)
		uint32 index;             //��������Ϣid�����ڱ�ʶ��Ϣ�ڷ������洢λ�ã�������ȡ��ʷ��Ϣ
		uint32 sendId;            //������id
		uint32 recvId;            //������id,  �����Ƶ����Ϣ�����idΪkeystring������
		YVMessageType type;       //��Ϣ����
		YVMessageStatus state;    //��Ϣ״̬
		uint32 sendTime;          //��Ϣ����ʱ��
		uint32 getTimeStamp(){ return sendTime; };


		bool operator <= (const _YVMessage&);
	};
	WISDOM_PTR(_YVMessage, YVMessagePtr);

	//�ı���Ϣ
	struct _YVTextMessage
		:public _YVMessage
	{
		_YVTextMessage(std::string inText)
		:_YVMessage(YVMessageTypeText, YVMessageStatusCreated)
		{
			text.append(inText);
		}

		std::string  text;       //�ı�����
	};
	WISDOM_PTR(_YVTextMessage, YVTextMessagePtr);

	//ͼƬ��Ϣ
	struct _YVImageMessage
		:public _YVMessage
	{
		_YVImageMessage()
		: _YVMessage(YVMessageTypeImage, YVMessageStatusCreated)
		{

		}
		YVFilePathPtr picPath;				//ԭͼ��ַ
		YVFilePathPtr littlePicPath;       //Сͼ��ַ
	};
	WISDOM_PTR(_YVImageMessage, YVImageMessagePtr);

	//������Ϣ
	struct _YVVoiceMessage
		:public  _YVMessage
	{
		_YVVoiceMessage(std::string path, bool isUrl, uint32  invoiceTimes, std::string inattach)
		: _YVMessage(YVMessageTypeAudio, YVMessageStatusCreated)
		{

			if (isUrl)
			{
				voicePath = new _YVFilePath(NULL, path.c_str());
			}
			else
			{
				voicePath = new _YVFilePath(path.c_str(), NULL);
			}
		
			voiceTimes = invoiceTimes;
			attach.append(inattach);
		}

		_YVVoiceMessage(YVFilePathPtr   invoicePath, uint32  invoiceTimes, std::string inattach)
			: _YVMessage(YVMessageTypeAudio, YVMessageStatusCreated)
		{
			voicePath = invoicePath;
			voiceTimes = invoiceTimes;
			attach.append(inattach);
		}

		YVFilePathPtr   voicePath;
		uint32      voiceTimes;          //ʱ��
		std::string attach;              //�����ı�
	};
	WISDOM_PTR(_YVVoiceMessage, YVVoiceMessagePtr);

	//��Ϣ�б�
	class _YVMessageList
	{
	public:
		_YVMessageList();
		void setMaxNum(uint32 maxNum);
		void clear();
		void insertMessage(YVMessagePtr&);
		std::vector<YVMessagePtr>& getMessageList();
		//�����Ƶ������ΪƵ��id
		uint32 getChatWithID(){ return id; }
		void setChatWithID(uint32 chatId){ id = chatId; }
		//ͨ����Ϣid��ȡ��Ϣ
		YVMessagePtr getMessageById(uint64 mssageID);
		//ͨ����Ϣidɾ��ĳ����Ϣ
		void delMessageById(uint64 mssageID);
	private:
		uint32 id;
		uint32 m_maxMessageNum;     //�����Ϣ����
		std::vector<YVMessagePtr> m_msssageList;
	};
	WISDOM_PTR(_YVMessageList, YVMessageListPtr);

	//����������Ϣ�б�
	typedef std::map<uint32, YVMessageListPtr> FriendChatMessageMap;
	typedef std::map<uint32, YVMessageListPtr> ChannelMessageMap;
}
#endif