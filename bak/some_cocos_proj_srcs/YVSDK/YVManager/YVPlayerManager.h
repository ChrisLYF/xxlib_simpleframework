#ifndef _YVSDK_YVPLAYERMANAGER_H_
#define _YVSDK_YVPLAYERMANAGER_H_
#include <string>
#include "YVListern/YVListern.h"
namespace YVSDK
{
	struct YaYaRespondBase;

	class YVPlayerManager
	{
	public:
		bool init();
		bool destory();

		bool cpLogin(std::string ttStr);
		bool yyLogin(int userId, std::string passWord);

		void loginResponceCallback(YaYaRespondBase*);
		void cpLoginResponce(YaYaRespondBase*);

		const YVUInfoPtr getLoginUserInfo();

		InitListern(CPLogin, CPLoginResponce*);
		InitListern(YYLogin, LoginResponse*);
	private:
		//��¼���Ǹ��û��Ļ�����Ϣ//
		YVUInfoPtr m_loginUserInfo;
	};
}
#endif