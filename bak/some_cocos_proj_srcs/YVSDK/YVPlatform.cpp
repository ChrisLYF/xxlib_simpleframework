/*********************************************************************************
*Copyright(C), 2015 YUNVA Company
*FileName:  RespondFactory.cpp
*Author:  Ԫ��
*Version:  1.0.0
*Date:  2015-5-7
*Description:  ����SDK���ʵ��
**********************************************************************************/
#include "YVPlatform.h"
#include "YVUtils/YVMsgDispatcher.h"
#include "YVManager/YVManager.h"

using namespace YVSDK;
YVPlatform* YVPlatform::s_YVPlatformPtr = NULL;

YVPlatform* YVPlatform::getSingletonPtr()
{
    assert( s_YVPlatformPtr && "Not create instance");
	return s_YVPlatformPtr;
}

void YVPlatform::createSingleton()
{
    assert( !s_YVPlatformPtr && "Dupilication create instance");
	s_YVPlatformPtr = new YVPlatform();
}

void YVPlatform::destorySingleton()
{
    delete s_YVPlatformPtr;
    s_YVPlatformPtr = nullptr;
}

YVPlatform::YVPlatform()
{
	_isInit = false;
	m_msgDispatcher = new YVMsgDispatcher();
}

YVPlatform::~YVPlatform()
{
    delete m_msgDispatcher;
    m_msgDispatcher = nullptr;
}

YVMsgDispatcher* YVPlatform::getMsgDispatcher()
{
	return m_msgDispatcher;
}

void YVPlatform::updateSdk(float dt)
{
	m_msgDispatcher->dispatchMsg(dt);
}

bool YVPlatform::init()
{
	if (_isInit) return true;
	YVPlayerManager::init();
	YVContactManager::init();
	YVChannalChatManager::init();
	YVToolManager::init();
	YVFriendChatManager::init();

    //: Shrimps, init sdk
    //  ��ʽ��ʼ���������ɿ�����Ա��������ʲôʱ���ʼ��
    //  �����һ�η���������Ϣʱ����
    m_msgDispatcher->initSDK();
    //. 

	_isInit = true;
	return true;
}

bool YVPlatform::destory()
{
	if (!_isInit) return true;
	YVPlayerManager::destory();
	YVContactManager::destory();
	YVChannalChatManager::destory();
	YVToolManager::destory();
	YVFriendChatManager::destory();
	//ͨ�ð�SDK�����ͷ�
	m_msgDispatcher->releseSDK();
	_isInit = false;
	return false;
}