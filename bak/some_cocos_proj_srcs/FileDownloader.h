#ifndef __FILEDOWNLOADER_H__
#define __FILEDOWNLOADER_H__

// ����¶�� LUA

// �ļ�������������( ��������ֱ���ṩ ���� FileManager ���ۺϹ��� )
struct FileDownloadTask
{
    template<typename STRING_T>
    FileDownloadTask( STRING_T && fullURL, FileIndexRow* fileIndexRow, int originalFileSize = 0 )
        : fullURL( forward<STRING_T>( fullURL ) )
        , fileIndexRow( fileIndexRow )
        , originalFileSize( originalFileSize )
        , currentDownloadSize( 0 )
    { }
    FileDownloadTask();
    FileDownloadTask( FileDownloadTask const & other );
    FileDownloadTask( FileDownloadTask && other );
    FileDownloadTask & operator=( FileDownloadTask const & other );
    FileDownloadTask & operator=( FileDownloadTask && other );

    
    string          fullURL;                        // ���� url ��( ������ / )
    FileIndexRow*   fileIndexRow;                   // ָ����Ӧ FileIndexRow( ʵ������ url Ϊ �� url + row.name, size ���������ж�/�洢�����ܳ� )       
    int             originalFileSize;               // ��ʼ����ʱ���ļ��ߴ�( ������û��������� )
    int             currentDownloadSize;            // �����������ֽ���( ���� originalFileSize ���ǵ�ǰ�ļ����� )
};


// �ļ�������( ����һ�� thread. Ϊ������, ��������ÿ��ֻ�����ص����ļ�, ���ǿ���Ϊ��������һ�����������ṩ���� )
class FileDownloader
{
public:
    typedef function<void( FileEvent && )> EventHandler;
    typedef function<void()> TaskSupplier;

    FileDownloader();
    ~FileDownloader();

    // ע�� �¼�������
    void registerDownloadEventHandler( EventHandler f );

    // ע�� ��������Ӧ����. process ���ȵ�. ͨ�� download �����´�����
    void registerDownloadTaskSupplier( TaskSupplier f );

    // ͬʱע������ 2 �ֺ���
    void registerSupplierAndHandler( TaskSupplier ts, EventHandler eh );

    // ���� download �ڼ����ڲ��� fileIndexRow state �� mutex
    void setFileIndexRowMutex( mutex* m );

    // ������������֪ͨ�߳̿�ʼ����. ��� �Ѿ�������, �� return false
    bool download( FileDownloadTask const & task );

    // ֪ͨ�߳�ֹͣ����. �ɹ������ Stoped �¼�
    void stop();

//private:
    mutex *                 _fileIndexRowMutex;             // ���޸� fileIndexRow.state ʱʹ�õ� mutex
    mutex                   _defaultFileIndexRowMutex;      // _fileIndexRowMutex ��Ĭ��ֵ( Ϊ�˷���д���� )
    atomic<bool>            _downloading;                   // �Ƿ���������( download ����ִ���ڼ� ��ֵ��Ϊ true )
    bool                    _needStop;                      // ���ڴ���Ƿ���Ҫֹͣ���ź�
    FileDownloadTask        _task;                          // ��ǰ�ļ���������

    EventHandler            _eventHandler;                  // �¼�������
    TaskSupplier            _taskSupplier;                  // ��������Ӧ���� 
    mutex                   _funcMutex;                     // ������� 2 ������. set �� call �ص�ʱ lock һ�ѷ���

    bool                    _running;                       // ����֪ͨ�߳��˳�
    bool                    _disposed;                      // true ��ʾ�̳߳ɹ��˳�

    void *                  _curl;                          // CURL������. �� void ��Ϊ������ h ������ curl
    FILE *                  _file;                          // �ļ�������( download ��ʱ��Ż���� )
    string                  _crc;                           // ���� crc ��ʱ��������д, �������ļ�����

private:
    // for constructor
    void init();                                            

    // �����̵߳��ú���
    void process();                                         

    // �� process ���õ����غ���
    void download();                                        

    FileDownloader( FileDownloader const & other ) {};                      // delete
    FileDownloader & operator=( FileDownloader const & ) { return *this; }; // delete
};


#endif

