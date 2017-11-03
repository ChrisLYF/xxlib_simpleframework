#ifndef _YVSDK_YVPATH_H_
#define _YVSDK_YVPATH_H_
#include <string>
#include "YVDef.h"
#include "YVUtils/YVUtil.h"

namespace YVSDK
{

	enum YVFileState
	{
		UnkownState ,

		DownLoadingState,                //���ص���
		UpdateLoadingState ,              //�ϴ�����

		DownLoadErroSate ,                //����ʧ��
		UpdateLoadErrorState,

		OnlyLocalState  ,                  //�����ڱ���״̬
		OnlyNetWorkState,                //����������״̬

		BothExistState ,                  //���缰����״̬����
	};

	class _YVFilePath
	{
	public:
		_YVFilePath(const char* path, const char* url);

		std::string& getLocalPath();
		std::string& getUrlPath();

		void setLocalPath(std::string& path);
		void setUrlPath(std::string& url);
		YVFileState getState();
		uint64 getPathId();

		void setState(YVFileState);
	private:
		YVFileState m_state;
		uint64 m_id;
		std::string m_path;
		std::string m_url;
	};

	//����Ϊ֪��ָ��
	WISDOM_PTR(_YVFilePath, YVFilePathPtr);
}
#endif
