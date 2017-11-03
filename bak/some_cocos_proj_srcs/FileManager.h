#ifndef __FILEMANAGER_H__
#define __FILEMANAGER_H__


// �ļ�����������̬
enum class FileManagerState
{
    None,                   // ��ʼ̬, ֻ�� loadIndex
    IndexLoading,           // ���� loadIndex, ���ʧ�� ����Ϊ None
    Downloading,            // ��̨������( ����һ�ֳ�̬. ֻҪ��̨���ض��в���, �ͻ�һֱ���� )
    Loading                 // ������, ����ͣ��̨���ز���ʼ��ǰ���ε����غ��첽����, ֮��ָ���̨����
};


// �ļ�������( ����, �첽����, ж��. �����Ե�����ʽ���� )
/*

�������� �µ� ��ʽ����, ���漸������ ֻ���������� ��ǰ��Ҫ��������, ������� process() ȥ��. ������� �ļ������� �е�����
    ͨ�� loadIndex ���� index db �����Ϣ �� ����/����/����/���� �ļ�����db
    ͨ�� download �����ļ����б� ��׷�Ӻ�̨������, �������᲻�ϳ��Ժ�̨���� + У��. У�鲻����ɾ������. 
    ͨ�� load ���� �����ļ����б� �Է��� ��ǰ�������� + �첽��������. 
    ͨ�� unload ���� �����ļ����б� �Է�����Դж������. ( �ú�������ִ�в���Ч, ���� 3 �����첽, Ҫ�ȴ��¼�֪ͨ��֪���Ƿ�ɹ�֮�� )
��: 
    ���� �ļ�����������̬, ���ȱ��� loadIndex, �ɹ�֮����� load. download ������ʱ�ɵ���, ������ loadIndex ����֮��ſ�ʼ����
    �����Ͻ����� load �� unload �Ĳ�����ͻ, ����Ҫʹ���߸����¼� ������ʱ��

���ط�ʽ:
    ͨ�� curl, �賬ʱ 5 ��( �������� ), �� 1k 5�����( �������� ), �Զ��߷�ʽ����, У��, ��ѹ, ����ִ��.
    ʼ�մ� _download_files pop ����ǰҪ���ص��ļ�, ����ʼ����. �ۺϿ�����������Ϊ, �������ǵ�״̬��������������.
    load ����ִ���ڼ�, ��ֹͣ��̨���ؽ�ѹУ������, �� _download_files ѹ��һ�����ļ�, �ٻָ� ��̨���ؽ�ѹУ�� ������.
    ͨ���¼����, �ɵ�֪�����ļ��Ƿ��Ѿ��������. ���δ��Ҫ��� cancel load ����, ֹͣ�¼����, �ָ���̨���ؾͺ�.
*/
class FileManager
{
public:
    typedef function<void( FileEvent const & )> EventHandlerType;

    FileManager();
    ~FileManager();

    void registerLoadIndexEventHandler( LUA_FUNCTION eh );
    void registerDownloadEventHandler( LUA_FUNCTION eh );
    void registerLoadEventHandler( LUA_FUNCTION eh );

    // todo: maybe need unreg ? maybe not ?

    void registerLoadIndexEventHandler( EventHandlerType eh );          // ע�� ������� ���¼��ص�. ���� nullptr Ϊ��ע��
    void registerDownloadEventHandler( EventHandlerType eh );           // ע�� ��̨���� ���¼��ص�. ���� nullptr Ϊ��ע��
    void registerLoadEventHandler( EventHandlerType eh );               // ע�� �������� + �첽���� ���¼��ص�. ���� nullptr Ϊ��ע��

    // �������ļ�����: ( ��Ϸ launcher ����Ҫ�������� )
    // fileName: index db �ļ���( ���·��. ����ʱ�汾λ�� ��д��, ����汾λ����Դ )
    // baseURL: ���� url ������βΪ /. ���ص� db �ļ�Ϊ baseURL + fileName + ".web" , ���ص� db �� crc �ļ�Ϊ baseURL + fileName + ".web.crc"
    // ��� baseURL Ϊ ��"", �����������ⲽ
    // ע: ·����Ϊ���·��, ǰ�治�� /
    bool loadIndex( string const & fileName, string const & baseURL );

    // ����������Ҫ ��̨���� ���ļ��б�. ��ȥ�ز�����( ��ʱ�ɲ��� )
    // Ϊ�Ż���������, ʵʱ��Ӧ, ��������[0] Ϊ��̨ר��. load ʱ�л� ���������ṩ������¼�����
    void download( vector<string> const & files );

    // ע�� �������� + �첽���� �ļ��б�. �����ǰ�� load ��Ϊ��û�� ����, �� return false
    bool load( vector<string> const & files );

    // ��ж����Դ. �첽����, ж�� ����� ���첽���ص���Դ���ļ�. ��ǰ c2dx �汾Ϊ TextureCache ֧�� AsyncLoad ����ͼ
    bool unload( vector<string> const & files );


    // ע�ᵽ frame update �ص�, �Բ����¼�( ������ LUA )
    void update(float dt);

    // �õ� index �Է��� lua ��ͨ�����ļ���ȡ��Ϣ
    FileIndex* getIndex();

    MAKE_INSTANCE_H( FileManager );
    FileIndexRow* getFileIndexRow( string const & fn );                 // �� _fileIndex ���� fn( ��Ϊ���·���;���·��, ����ַ�Ϊ ʵ��·���� search ·�� )
private:
    void unloadCore( FileIndexRow* fir );
    void unloadCore( vector<FileIndexRow*> & firs );

    void process();                                                     // ���ȹ����߳�( �л�ִ������ 3 ������֮һ )
    void loadIndex();
    void download();
    void load();
    mutex                                   _stateMutex;                // �л� state �õ��� mutex
    FileManagerState                        _state;                     // FileManager ִ��̬
    void setState( FileManagerState v );

    // ��������ز���Ĳ��� store ���Լ��Ӻ���
    // for loadIndex
    string                                  _loadIndex_fileName;        // index db ���ļ���( ����·��Ϊ _baseURL + �� )
    FileIndex                               _fileIndex;                 // �� �ļ�����
    mutex                                   _fileIndexRowMutex;         // ���ǹ�ϵ�� FileIndoexRow ��Ĳ��� ��Ҫ lock( �������/������ֵ )

    // for load
    vector<FileIndexRow*>                   _load_files;                // load ʱ�����ԭʼ�ļ��б�
    ThreadSafeQueue<FileIndexRow*>          _load_needDownloadFiles;    // ��Ҫ �������� ���ļ�����ָ���б�
    ThreadSafeQueue<FileIndexRow*>          _load_imageFiles;           // ���߳� update ��ɨ, ������ļ�, ���� TextureCache addImageAsync ����
    atomic<int>                             _load_imageFilesCounter;    // TextureCache addImageAsync �ص����ۼӸ� counter
    ThreadSafeQueue<FileIndexRow*>          _load_resFiles;             // ���߳� update ��ɨ, ������ļ�, ������ļ���������Ӧ�ļ��ط�ʽ����
    atomic<int>                             _load_resFilesCounter;      // ���ع����н��ۼӸ� counter

    // for download
    ThreadSafeQueue<FileIndexRow*>          _download_files;            // ��̨Ҫ���� ���ļ��б�
    FileDownloader                          _downloaders[ 2 ];          // ���߲��ֿ���������( ���� _downloaders[ 0 ] ������ ���� index db �Լ���̨ )
	
    string                                  _baseURL;                   // ���ظ����ļ��ĸ� url

    EventHandlerType                         _eh_loadIndex;
    EventHandlerType                         _eh_download;
    EventHandlerType                         _eh_load;

    LUA_FUNCTION                            _lua_eh_loadIndex;
    LUA_FUNCTION                            _lua_eh_download;
    LUA_FUNCTION                            _lua_eh_load;

    ThreadSafeQueue<FileEvent>              _events_loadIndex;          // ����¼�����
    ThreadSafeQueue<FileEvent>              _events_download;           // ����¼�����
    ThreadSafeQueue<FileEvent>              _events_load;               // ����¼�����

    bool                                    _running;                   // �߳��˳����λ
    bool                                    _disposed;                  // true ��ʾ�̳߳ɹ��˳�

    unordered_map<FileIndexRow*, int>       _loadLog;                   // ���Լ�¼�ļ� load ����, unload ʱ�������, FileManager ����ʱ unload all

    bool                                    _asyncLoading;              // �����첽���صı�ǣ��첽����һ��һ�����ص����ٸ��ģ�
};


#endif







/*

���ʹ�÷�ʽ:


FileManager
host:
���� init, free

lua:


fm:load(fs, q, cb)

����:

fm = ww.FileManager:getInstance()   -- ������һ��ȫ�ֵı���

-- launcher:

fm:regLoadIndexCB( function( eventName )
         if eventName == "Started" then
    else if eventName == "Downloading" then
    else if eventName == "Verifyed" then
    else if eventName == "Copyed" then
    else if eventName == "Opened" then
    else if eventName == "Loaded" then
    else
    end
end )
fm:loadDB( "index.db", "index.db", "index.db.www", "http://asdfasdfasdf/v234/index.db" )







-- ��Ϸ����ʱ:



local files = {
"asdf.png",
"qwer.plist",
"zxcv.json"
-- ...
}
local priority = 1         -- Priority ���ȼ� ���ȼ�ֵΪ 1~n , Խ��Խ����

local callback = function( batch, eventName, ... )

if eventName == "��Ҫ����" then                 -- Ϊ�˷������, �տ�ʼ ���ܻ� ����һ��
    batch._dialog = xx.ShowDialog( ���ֲ��� )  -- ����¼��д����Ի���
elseif eventName == "ȫ�����" then             -- ֻ����һ��
    batch._dialog:hide();                      -- _ui Ϊ���ӵ� batch �ϵ� ui ����
-- navgateToxxxxx
Ŀ��UI._batch = batch                      -- ����һ������, �Ա��� Ŀ��UI �˳���, �� Ŀ��UI._batch:unload(); Ŀ��UI._batch = nil

elseif eventName == "������" then
    batch._dialog:setXxxxx( ... )              -- �����ֵ,̬
elseif eventName == "У����" then
    batch._dialog:setXxxxx( ... )              -- �����ֵ,̬
elseif eventName == "��ѹ��" then
    batch._dialog:setXxxxx( ... )              -- �����ֵ,̬
elseif eventName == "�����������" then
    batch._dialog:setXxxxx( ... )              -- �����ֵ,̬
elseif eventName == "����У�����" then
    batch._dialog:setXxxxx( ... )              -- �����ֵ,̬
elseif eventName == "������ѹ���" then
    batch._dialog:setXxxxx( ... )              -- �����ֵ,̬
elseif eventName == "����ɶɶ" then
    batch:cancel()                             -- ȡ�����β���
elseif eventName == "���ֳ���ʧ��" then
    batch._dialog:setXxxxx( ... )              -- �����ֵ,̬
elseif eventName == "��ȡ��" then
    xxxxxxx                                    -- ִ���� batch:cancel() ������ڲ�ȡ���˱�������( �����ص��ļ�������ɾ )
else
    -- ...�����쳣? ��ж���¼� ���ڵ���?
end
    -- ...
end

local batch = fm:load( files, priority, callback )

batch._dialog = xx.ShowDialog()        -- �� loading ui �� batch ����
batch._dialog._batch = batch


*/

