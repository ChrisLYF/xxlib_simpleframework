#pragma once

#include <string>


using std::string;

class YVListernImpl;

class ChatSys 
{
    friend class YVListernImpl;
	// no for lua
public:
	ChatSys();
	~ChatSys();

	// for lua manual binding
public:
	// ע��yunva �ص�
	void registerToChatEventHandler(LUA_FUNCTION func) {
		_chat_cb = func;
	}

	bool hasCB() { return !!_chat_cb; }
	LUA_FUNCTION getCB() { return _chat_cb; }

private:
	LUA_FUNCTION _chat_cb;
   

private:
    enum CallbackTypes : int
    {
        CT_Login = 1,
        CT_ModifyChannelKey,
        CT_RecvChannelMsg,
        CT_FinishSpeech,
        CT_FinishPlay,
        CT_SendChannelState,
    };

    void loginCB(int err, string err_msg);
    void modifyChannelCB(int err, string err_msg);
    void recvChannelMessageCB(int err, int msg_type, string msg_body, int channel_id, string url);
    void finishSpeechCB(int err, string err_msg, string voice_path, int duration, string txt);
    void finishPlayCB();
    void sendChannelStateCB(int err);

	// for lua auto binding
public:
	MAKE_INSTANCE_H(ChatSys);

    void resetChannelKeys();
    void setGameServerId(string id, bool imm = true);
    void setLegionId(int id, bool imm = true);
    void setStageId(int id, bool imm = true);

	//��½
	void login_yaya(string tt);
	//���Ƶ��
	void loginChannel();
	//�˳�����ϵͳ
	void outIMGame();
	//�����ı���Ϣ
	void sendMsg(string str_send_msg, string wildcard);
	//����������Ϣ
	void sendVoiceAndText(string voicePath, int voiceDuration, string wildcard, string txt);
	//��ʼ¼��
	void soundRecording();
	//���¼��
	void soundRecordingFinished();
	//ȡ��¼��
	void soundRecordingStop();
	//���ű�������
	void playAudioWithFilePath(string filePath);
	//������������
	void playAudioWithUrl(string url);

    int getChannelId(string key);
    string getChannelKey(int idx);
    void setChannelKey(int idx, string key);

private:
    void updateChannelKeys();

private:
    YVListernImpl *_lsner;
    string         _pgameServiceID;
    vector<string> _channel_keys;
};

