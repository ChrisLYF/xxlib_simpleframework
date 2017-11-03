#ifndef __GLOBAL_H__
#define __GLOBAL_H__


// ����ȫ��, �������֮����
class Global
{
public:
    static lua_State * L;               // ָ��ȫ���������Ǹ�
    static LuaStack * LS;               // ָ��ȫ���������Ǹ�
    static string WritablePath;         // ���дĿ¼
    static string ResourcePath;         // ����ԴĿ¼
    static vector<string> SearchPaths;

    // ������ִ��һ��( ����λ��: AppDelegate::applicationDidFinishLaunching() ���, lua ��ڳ���ִ��֮ǰ )
    static void init( LuaStack * LS, lua_State * L, string && writablePath, string && resourcePath, vector<string> && SearchPaths );

    // ����ǰִ��һ��( ����λ��: AppDelegate::~AppDelegate() ��� )
    static void free();         
};


#endif

