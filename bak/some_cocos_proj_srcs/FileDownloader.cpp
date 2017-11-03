#include "Precompile.h"
/* curl */
#include <curl/curl.h>
#include <curl/easy.h>


FileDownloadTask::FileDownloadTask()
: fileIndexRow( nullptr )
, originalFileSize( 0 )
, currentDownloadSize( 0 )
{
}

FileDownloadTask::FileDownloadTask( FileDownloadTask const & other )
: fullURL( other.fullURL )
, fileIndexRow( other.fileIndexRow )
, originalFileSize( other.originalFileSize )
, currentDownloadSize( other.currentDownloadSize )
{
}

FileDownloadTask::FileDownloadTask( FileDownloadTask && other )
: fullURL( move( other.fullURL ) )
, fileIndexRow( other.fileIndexRow )
, originalFileSize( other.originalFileSize )
, currentDownloadSize( other.currentDownloadSize )
{
}

FileDownloadTask & FileDownloadTask::operator=( FileDownloadTask const & other )
{
    fullURL = other.fullURL;
    fileIndexRow = other.fileIndexRow;
    originalFileSize = other.originalFileSize;
    currentDownloadSize = other.currentDownloadSize;
    return *this;
}

FileDownloadTask & FileDownloadTask::operator=( FileDownloadTask && other )
{
    fullURL = move( other.fullURL );
    fileIndexRow = other.fileIndexRow;
    originalFileSize = other.originalFileSize;
    currentDownloadSize = other.currentDownloadSize;
    return *this;
}




void FileDownloader::init()
{
    _running = true;
    _disposed = false;
    _downloading = false;
    //_curl = nullptr;
    _file = nullptr;
    _needStop = false;
    _fileIndexRowMutex = &_defaultFileIndexRowMutex;

    _curl = curl_easy_init();
    curl_easy_setopt( (CURL*)_curl, CURLOPT_NOSIGNAL,        1 );         // �� alarm ��ʱ( ������߳�����, ���ǿ��ܿ��������� )
    curl_easy_setopt( (CURL*)_curl, CURLOPT_CONNECTTIMEOUT , 8L );
    //curl_easy_setopt( (CURL*)_curl, CURLOPT_LOW_SPEED_LIMIT, 128L );    // �ڲ������粻���õĻ�����
    //curl_easy_setopt( (CURL*)_curl, CURLOPT_LOW_SPEED_TIME,  10L );     // ���´������ӶϿ�����ʵ���ϻ����������ڴ���
    curl_easy_setopt( (CURL*)_curl, CURLOPT_ACCEPT_ENCODING, "");
    
    thread( &FileDownloader::process, this ).detach();
}

FileDownloader::FileDownloader()
{
    init();
}

void FileDownloader::registerDownloadEventHandler( EventHandler f )
{
    lock_guard<mutex> g( _funcMutex );
    _eventHandler = f;
}

void FileDownloader::registerDownloadTaskSupplier( TaskSupplier f )
{
    lock_guard<mutex> g( _funcMutex );
    _taskSupplier = f;
}

void FileDownloader::registerSupplierAndHandler( TaskSupplier ts, EventHandler eh )
{
    lock_guard<mutex> g( _funcMutex );
    _taskSupplier = ts;
    _eventHandler = eh;
}

void FileDownloader::setFileIndexRowMutex( mutex* m )
{
    if( m ) _fileIndexRowMutex = m;
    else _fileIndexRowMutex = &_defaultFileIndexRowMutex;
}


FileDownloader::~FileDownloader()
{
    _running = false;
    while( !_disposed ) sleepMS( 1 );
    curl_easy_cleanup( (CURL*)_curl );
    _curl = nullptr;
}

bool FileDownloader::download( FileDownloadTask const & task )
{
    if( _downloading || !_running ) return false;
    _needStop = false;                          // ��һ�ѱ���
    _task = task;
    _downloading = true;                        // ֪ͨ process �߳̿�ʼ����
    return true;
}

void FileDownloader::stop()
{
    if( _downloading ) _needStop = true;
}

// �̺߳���
void FileDownloader::process()
{
    while( _running )                           // ��ѭ��ֱ��֪ͨ�˳�
    {
        TaskSupplier tsf;               // ��ȫ��ȡ ���������ṩ����
        {
            lock_guard<mutex> g( _funcMutex );
            tsf = _taskSupplier;
        }
        if( tsf )                               // ����� reg ��������Ӧ����, ��֮
        {
            tsf();
        }
        if( _downloading )                      // �������������( ͨ�� download ������� ), ��ʼ��֮
        {
            download();
            if( _needStop )                     // �ֶ�ֹͣ�������ǽ�ά��
            {
                EventHandler cbf;            // ��ȫ��ȡ �¼�������
                {
                    lock_guard<mutex> g( _funcMutex );
                    cbf = _eventHandler;
                }
                if( cbf )
                {
                    cbf( FileEvent( FileEventTypes::Download_Stoped ).attachUserData( &_task ) );
                }

                _needStop = false;
            }
            _downloading = false;
        }
        else sleepMS( 200 );                     // ����������������ʡ�� cpu. ��ʼ���ز���Ҫ��ô��ʱ
    }
    _disposed = true;                           // �ɹ��˳�ѭ��ʱ���Ϊ���˳�
}

// ���� curl д�ļ� �ص�. userdata Ϊ FileDownloader
size_t write_function_header( char *buf, size_t size, size_t nmemb, void *userdata )
{
    return nmemb;
}

// ���� curl д�ļ� �ص�. userdata Ϊ FileDownloader
size_t write_function_bin( char *buf, size_t size, size_t nmemb, void *userdata )
{
    auto d = (FileDownloader*)userdata;
    auto written = fwrite( buf, size, nmemb, d->_file );
    fflush( d->_file );                         // д�ļ� ����д��
    d->_task.currentDownloadSize += size * nmemb;
    FileDownloader::EventHandler cbf;        // ��ȫ��ȡ �¼�������
    {
        lock_guard<mutex> g( d->_funcMutex );
        cbf = d->_eventHandler;
    }
    if( cbf )
    {
        cbf( FileEvent( FileEventTypes::Download_Appending, d->_task.fileIndexRow->name, size * nmemb ).attachUserData( &d->_task ) );
    }
    if( !d->_running || d->_needStop ) return 0;// �����ǰ��Ҫֹͣ����, ���� 0 ��ֹ����
    return written;
}

// ���� curl д crc �ļ� �ص�. userdata Ϊ FileDownloader
size_t write_function_crc( char *buf, size_t size, size_t nmemb, void *userdata )
{
    auto d = (FileDownloader*)userdata;
    d->_crc.append( buf, size * nmemb );
    FileDownloader::EventHandler cbf;        // ��ȫ��ȡ �¼�������
    {
        lock_guard<mutex> g( d->_funcMutex );
        cbf = d->_eventHandler;
    }
    if( cbf )
    {
        cbf( FileEvent( FileEventTypes::Download_CRCAppending, size * nmemb ).attachUserData( &d->_task ) );
    }
    if( !d->_running || d->_needStop ) return 0;// �����ǰ��Ҫֹͣ����, ���� 0 ��ֹ����
    return nmemb;
}

void FileDownloader::download()
{
    // ָ���ļ�������
    auto &fir = _task.fileIndexRow;

    // ���ļ�״̬( lock )
    auto setState = [ &]( FileStates s )
    {
        lock_guard<mutex> g( *_fileIndexRowMutex );
        fir->state = s;
    };

    // ��ȫ call �¼�������
    auto eventCallback = [ this, fir ]( FileEvent && e )
    {
        FileDownloader::EventHandler cbf;
        {
            lock_guard<mutex> g( _funcMutex );
            cbf = _eventHandler;
        }
        if( cbf )
        {
            cbf( move( e.attachUserData(fir) ) );
        }
    };

    // return ʱ�Զ��ر��ļ�
    Utils::ScopeGuard file_close_rg( [ this ]
    {
        if( this->_file )
        {
            fclose( _file );
            this->_file = nullptr;
        }
    } );

    // �õ�д���ļ� �� ����url ����·��
    auto url = _task.fullURL;
    auto url_crc = url + ".crc";

    auto curl = (CURL*)_curl;

    // �� д�ص��� userdata ���� Ϊ this
    curl_easy_setopt( curl, CURLOPT_FILE, this );

    // ����·��, ����, д�ص� ʲô��, �����ص㸽����


    // ���ݵ�ǰ �������� �� state ��·�ɽ�����Ҫ������
    {
        lock_guard<mutex> g( *_fileIndexRowMutex );
        switch( fir->state )
        {
        case FileStates::Finished:                               // �п������ѹ����������
            eventCallback( FileEvent( FileEventTypes::Download_Finished, fir->name ) );
            return;

        case FileStates::NeedDownload:
            fir->state = FileStates::Downloading;                // ������̬
            goto Lab_Download;

        case FileStates::CRCNeedDownload:
            fir->state = FileStates::CRCDownloading;             // ������ crc ̬
            goto Lab_CRCDownload;

        case FileStates::NeedChecksum:
            fir->state = FileStates::Checksuming;                // ��У��̬
            goto Lab_Checksum;

        case FileStates::NeedExtract:
            fir->state = FileStates::Extracting;                 // �ý�ѹ̬
            goto Lab_Extract;
        default:
            // todo: CCLOG
            break;
        }
    }

Lab_Download:

    // ��Ŀ¼
    if( !Utils::ensureDirectory( Utils::filePathCutFileName( fir->fullName ) ) )
    {
        eventCallback( FileEvent( FileEventTypes::Download_OpenFileError, fir->name ) );
        setState( FileStates::NeedDownload );
        return;
    }

    // ������׷�ӻ��½�( ������ EOF �� ) 
    _file = fopen( fir->fullName.c_str(), "a+b" );
    if( !_file )
    {
        eventCallback( FileEvent( FileEventTypes::Download_OpenFileError, fir->name ) );
        setState( FileStates::NeedDownload );
        return;
    }

    // ȡ�ļ���ǰ�ߴ�
    _task.originalFileSize = Utils::fileMoveSeekEnd( _file );

    // ���ø������ز���
    curl_easy_setopt( curl, CURLOPT_URL, url.c_str() );                        // ������·��

    // �� ��ʼ���� �¼�
    eventCallback( FileEvent( FileEventTypes::Download_Started, fir->name ) );
    

    // ����ļ��ܳ�δ֪, ��ȡһ��
    if( !fir->size )
    {
        curl_easy_setopt( curl, CURLOPT_HEADER, 1L );                          // �����ļ�ͷ
        curl_easy_setopt( curl, CURLOPT_NOBODY, 1L );                          // �������ļ�����

        Utils::ScopeGuard curl_readLength_rg( [ &]                             // �����������Զ���ԭ curl ����
        {
            curl_easy_setopt( curl, CURLOPT_HEADER, 0L );                      // ��ԭ���� for ��������
            curl_easy_setopt( curl, CURLOPT_NOBODY, 0L );                      // ��ԭ���� for ��������
        } );

        // �� д�ص�
        curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, write_function_header );

        auto code = curl_easy_perform( curl );                                 // �����ļ�ͷ
        if( code )
        {
            eventCallback( FileEvent( FileEventTypes::Download_ResponseError, fir->name ) );
            setState( FileStates::NeedDownload );
            return;
        }
        long retcode = 0;
        code = curl_easy_getinfo( curl, CURLINFO_RESPONSE_CODE, &retcode );
        if( ( code == CURLE_OK ) && retcode == 200 )
        {
            double length = 0;
            code = curl_easy_getinfo( curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &length );
            fir->size = length;
            if( code != CURLE_OK || fir->size == -1 )
            {
                fir->size = 0;  // δȡ������
                eventCallback( FileEvent( FileEventTypes::Download_GotLength, fir->name, 0, _task.originalFileSize ) );
            }
            else
            {
                eventCallback( FileEvent( FileEventTypes::Download_GotLength, fir->name, length, _task.originalFileSize ) );
            }
        }
        else
        {
            eventCallback( FileEvent( FileEventTypes::Download_GetLengthError, fir->name ) );
            setState( FileStates::NeedDownload );
            return;
        }
    }
    else
    {
        eventCallback( FileEvent( FileEventTypes::Download_GotLength, fir->name, fir->size, _task.originalFileSize ) );
    }

    // �������󳤶�, �뵱ǰ���ȱȽ�, �����ܲ��ܲ�����, ������һ��
    if( fir->size )
    {
        if( _task.originalFileSize == fir->size )                // ���ȷ���Ԥ��, ֱ�� checksum �� ���� crc
        {
            eventCallback( FileEvent( FileEventTypes::Download_Downloaded, fir->name ) );

            if( fir->crc32 || fir->md5.size() )                                 // Ǳ����: ��� crc32, md5 Ϊ�� �����غ��� crc ��Ϣ���ļ�, ������չ��Ϊ .crc
            {
                setState( FileStates::NeedChecksum );
                //goto Lab_Checksum;
            }
            //else
            //{
            //    setState( FileStates::CRCDownloading );
            //    goto Lab_CRCDownload;
            //}
            goto Lab_Checksum;      // �ȷ��������� .crc 
        }
        if( _task.originalFileSize > fir->size )                 // ����Ѵ��ڵ��ļ���Ҫ���صĳ�, ��Ϊ�Ǵ�����ļ�, ɾ��ʬ��, �������¼�
        {
            Utils::fileDelete( fir->fullName );
            _task.originalFileSize = 0;
        }
    }

    // ����
    if( _task.originalFileSize )
    {
        curl_easy_setopt( curl, CURLOPT_RESUME_FROM, (long)_task.originalFileSize );   // �������ֽ���
    }
    else
    {
        curl_easy_setopt( curl, CURLOPT_RESUME_FROM, (long)0 );
    }

    // �� д�ص�
    curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, write_function_bin );

    {
    // ����
    auto res = curl_easy_perform( curl );
    if( res )
    {
        if (res == CURLE_WRITE_ERROR)
        {
            eventCallback( FileEvent( FileEventTypes::Download_WriteFileError, fir->name, res ) );
            setState( FileStates::NeedDownload );
        }
        else if (res == CURLE_RANGE_ERROR)
        {
            Utils::fileCut(this->_file, 0);
            _task.originalFileSize = 0;
            _task.currentDownloadSize = 0;
            eventCallback( FileEvent( FileEventTypes::Download_ResponseError, fir->name, res ) );
            setState( FileStates::NeedDownload );
        }
        else
        {
            eventCallback( FileEvent( FileEventTypes::Download_ResponseError, fir->name, res ) );
            setState( FileStates::NeedDownload );
        }

        return;
    }

    // ȡ������
    long resp_code = 0;
    res = curl_easy_getinfo( curl, CURLINFO_RESPONSE_CODE, &resp_code );

    // �жϷ�����. �������, ���� append ��������Ч����, �е�
    if( res != 0 || ( resp_code != 200 && resp_code != 206 ) )
    {
        if( _task.originalFileSize )
        {
            Utils::fileCut( _file, _task.originalFileSize );                    // �е��������ص����ݲ��� �����ļ�ԭʼ����
        }
        else
        {
            file_close_rg.run();                                                // ���ļ�
            Utils::fileDelete( fir->fullName );                                 // ɾ��
        }
        eventCallback( FileEvent( FileEventTypes::Download_ResponseError, fir->name, res, resp_code ) );
        setState( FileStates::NeedDownload );
        return;                                                                 // ���ﲻ���ٹ��ļ���
    }
    }

    //// �ж��Ƿ���Ҫ���� crc �ļ�
    //if( fir->crc32 || fir->md5.size() )
    //{
    eventCallback( FileEvent( FileEventTypes::Download_Downloaded, fir->name ) );
    goto Lab_Checksum;      // ������ Lab_CRCDownload
    //}




Lab_CRCDownload:

    // �� _crc ׼������
    _crc = "";

    // �� crc ���� url
    curl_easy_setopt( curl, CURLOPT_URL, url_crc.c_str() );

    // ������
    curl_easy_setopt( curl, CURLOPT_RESUME_FROM, 0L );

    // �� д crc �ص�
    curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, write_function_crc );

    {
    auto res = curl_easy_perform( curl );                                      // ����
    if( res )
    {
        auto et = ( res == CURLE_WRITE_ERROR ? FileEventTypes::Download_WriteFileError : FileEventTypes::Download_ResponseError );
        eventCallback( FileEvent( et, fir->name ) );
        setState( FileStates::CRCNeedDownload );
        return;
    }

    // ȡ������
    auto resp_code = 0;
    res = curl_easy_getinfo( curl, CURLINFO_RESPONSE_CODE, &resp_code );

    // �жϷ�����. �������, ���� append ��������Ч����, �е�
    if( res != 0 || ( resp_code != 200 && resp_code != 206 ) )
    {
        eventCallback( FileEvent( FileEventTypes::Download_ResponseError, fir->name, res, resp_code ) );
        setState( FileStates::CRCNeedDownload );
        return;

        // test code
        //_crc = "-676980427,ED329A85E35AEB31AA9F971E9D394D6D";
    }
    }

    // �����ص� crc ���ݴ浽 fir ��
    // �ļ���ʽΪ crc,md5 ���м��� ���ŷָ�. ������ת��ʧ�ܽ�����
    {
        auto ss = Utils::split( _crc, "," );
        if( ss.size() != 2 || ss[ 0 ].size() == 0 || ss[ 1 ].size() == 0 )
        {
            eventCallback( FileEvent( FileEventTypes::Download_CRCFormatError, fir->name, _crc ).attachUserData( &_task ) );
            setState( FileStates::CRCNeedDownload );
            return;
        }
        fir->crc32 = atoi( ss[ 0 ].c_str() );
        if( fir->crc32 == 0 )
        {
            eventCallback( FileEvent( FileEventTypes::Download_CRCFormatError, fir->name, ss[ 0 ] ).attachUserData( &_task ) );
            setState( FileStates::CRCNeedDownload );
            return;
        }
        fir->md5 = ss[ 1 ];
    }

    setState( FileStates::NeedChecksum );



Lab_Checksum:

    if( !fir->crc32 && !fir->md5.size() ) goto Lab_Finished;                // ���δ�ṩ crc md5 �Ͳ�У��

    {
    if( _file )                             // �Ѵ�( ��Ҫ�ƶ��α� )
    {
        fseek( _file, 0, SEEK_SET );
    }
    else                                    // ���ļ�( �����Ҫ�Ļ� )
    {
        _file = fopen( fir->fullName.c_str(), "rb" );
        if( !_file )
        {
            eventCallback( FileEvent( FileEventTypes::Download_OpenFileError, fir->name ) );
            setState( FileStates::NeedChecksum );
            return;
        }
    }

    // ��ʼУ��
    int  len;
    char buf[ 1024 ];
    uLong crc = crc32( 0L, Z_NULL, 0 );     // ��ʼ��������?
    WW_MD5_CTX md5;
    WW_MD5_Init( &md5 );                    // ��ʼ��������?

    auto md5_data = (byte*)buf;
    char *md5_string = buf + 16;

    while( !feof( _file ) && ( len = fread( buf, 1, sizeof( buf ), _file ) ) )
    {
        crc = crc32( crc, (byte*)buf, len );
        WW_MD5_Update( &md5, buf, len );
    }

    memset( md5_data, 0, sizeof( buf ) );
    WW_MD5_Final( (byte*)md5_data, &md5 );

    static char digt2hex[] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };

#define D2H(idx)                                                \
    md5_string[idx * 2    ] = digt2hex[md5_data[idx] >> 4  ];   \
    md5_string[idx * 2 + 1] = digt2hex[md5_data[idx] & 0x0F]

    D2H( 0x0 );   D2H( 0x1 );   D2H( 0x2 );   D2H( 0x3 );
    D2H( 0x4 );   D2H( 0x5 );   D2H( 0x6 );   D2H( 0x7 );
    D2H( 0x8 );   D2H( 0x9 );   D2H( 0xa );   D2H( 0xb );
    D2H( 0xc );   D2H( 0xd );   D2H( 0xe );   D2H( 0xf );

    md5_string[ 32 ] = 0;
#undef D2H

    if( fir->md5.size() && stricmp( md5_string, fir->md5.c_str() ) )
    {
        file_close_rg.runAndCancel();
        Utils::fileDelete( fir->fullName );
        eventCallback( FileEvent( FileEventTypes::Download_MD5Error, fir->name, string( md5_string ), fir->md5 ) );
        setState( FileStates::NeedDownload );
        return;
    }

    if( fir->crc32 && ( int )(uint)crc != fir->crc32 )
    {
        file_close_rg.runAndCancel();
        Utils::fileDelete( fir->fullName );
        eventCallback( FileEvent( FileEventTypes::Download_CRC32Error, fir->name, (int)crc, fir->crc32 ) );
        setState( FileStates::NeedDownload );
        return;
    }

    if( false )      // todo: check if need extract
    {
        setState( FileStates::NeedExtract );
        goto Lab_Extract;
    }
    goto Lab_Finished;
    }




Lab_Extract:
    // todo: ��ѹ( ǰ��������ļ���, url ҲҪ���� ���� zip �ļ������� )




Lab_Finished:

    file_close_rg.runAndCancel();
    setState( FileStates::Finished );
    eventCallback( FileEvent( FileEventTypes::Download_Finished, fir->name ) );       // �������, �����¼�������
}





//curl_easy_setopt( _curl, CURLOPT_NOPROGRESS, false );                       // ��������
//curl_easy_setopt( _curl, CURLOPT_PROGRESSDATA, &task );                     // �� ���Ȼص� �� userdata ���� Ϊ task
//curl_easy_setopt( _curl, CURLOPT_PROGRESSFUNCTION, progress_function );     // ���Ȼص�

//int progress_function_ex( void *userdata, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow )
//{
//}

//int progress_function( void *userdata, double dltotal, double dlnow, double ultotal, double ulnow )
//{
//    //return progress_function_ex( userdata, (curl_off_t)dltotal, (curl_off_t)dlnow, (curl_off_t)ultotal, (curl_off_t)ulnow );

//    auto task = (FileDownloadTask *)userdata;
//    double progress = 0;
//    if( dltotal > 0 )
//    {
//        progress = ( task->currSize + dlnow ) / ( task->currSize + dltotal ) * 100;
//        //task->currSize += dlnow;
//    }
//    task->downloader->_events.push( FileDownloadEvent( FileDownloadEventTypes::Downloading, task ) );
//    return 0;
//}
//
//void FileDownloader::add( string const & downloadURL, string saveToFileName, bool append, int maxFileSize )
//{
//    if( _tasks.exists( [ &]( FileDownloadTask const & t ) { return t.url == downloadURL; } ) ) return;
//    if( !append ) Utils::fileDelete( saveToFileName );
//    FileDownloadTask t = { this, downloadURL, saveToFileName, 0, maxFileSize, 0 };    // originalFileSize ���� download ������ open file ��ȡ
//    _tasks.push( t );
//}
//
//void FileDownloader::remove( FileDownloadTask* task )
//{
//    _events.erase( [ &]( FileDownloadEvent& e ) { return e.task == task; } );
//}
//
//void FileDownloader::remove( string const & downloadURL )
//{
//    _events.erase( [ &]( FileDownloadEvent& e ) { return e.task->url == downloadURL; } );
//}
//

//void FileDownloader::setTask( string const & downloadURL, string saveToFileName, bool append, int maxFileSize )
//{
//if( _tasks.exists( [ &]( FileDownloadTask const & t ) { return t.url == downloadURL; } ) ) return;
//if( !append ) Utils::fileDelete( saveToFileName );
//FileDownloadTask t = { this, downloadURL, saveToFileName, 0, maxFileSize, 0 };    // originalFileSize ���� download ������ open file ��ȡ
//_tasks.push( t );
//}

//void FileDownloader::update()
//{
//    if( _events.empty() ) return;   // ��� _cb һֱΪ�� �ƺ����л��ĺܴ�?
//    if( !_cb )
//    {
//        CCLOG( "FileDownloader::update() the _cb is nullptr, please regCB first, before add download task." );
//    }
//    FileDownloadEvent e;
//    while( _events.pop( e ) )
//    {
//        _cb( e );
//    }
//}



//
//
////typedef size_t( *ft )( void *ptr, size_t, size_t, void * );
//size_t process( void *ptr, size_t size, size_t nmemb, void *userdata )
//{
//    static int x = 0;
//    if( x++ == 2 )
//    {
//        log( "return CURL_WRITEFUNC_PAUSE" );
//        return CURL_WRITEFUNC_PAUSE;    // ����ܳ�ʱӰ��
//    }
//
//    log( "size = %d, nmemb = %d", (int)size, (int)nmemb );
//    std::string s;
//    BufferHelper::dump( s, (char*)ptr, size * nmemb );
//    //printf( "data = %s", s.c_str() );
//    return nmemb;
//}
//
//bool test( )
//{
//    // ��������һЩ���ֵ�˵��, ���ܻῨ��������. �ʿ������ڵ����߳��� ���Ƴ�ʱ��̽�� url, �õ� ip ��, �ڶ��߳���ʹ��
//
//    /*
//    �ڶ��߳�Ӧ���У���Ҫ�����߳��е�����������������������libcurl����Ļ�����ͨ��������������ʽ�ĵ���������һ�ε��� curl_easy_init()ʱ��curl_easy_init ����� curl_global_init���ڵ��̻߳����£��ⲻ�����⡣���Ƕ��߳��¾Ͳ����ˣ���Ϊcurl_global_init�����̰߳�ȫ�ġ��ڶ���� ���е���curl_easy_int��Ȼ����������߳�ͬʱ����curl_global_init��û�б����ã�ͬʱ���� curl_global_init������ͷ����ˡ�������������ĸ��ʺ�С�����������Ǵ��ڵġ�
//    */
//    curl_global_init( CURL_GLOBAL_ALL );                        // global init
//
//    auto c = curl_easy_init( );                                  // session init
//    curl_easy_setopt( c, CURLOPT_URL, "http://127.0.0.1/1.data" );
//    curl_easy_setopt( c, CURLOPT_VERBOSE, 1L );                 /* Switch on full protocol/debug output while testing */
//    curl_easy_setopt( c, CURLOPT_NOPROGRESS, 1L );              /* disable progress meter, set to 0L to enable and disable debug output */
//
//    /*
//    libcurl �и��ܺõ����ԣ����������Կ������������ĳ�ʱ��������Ĭ������£�����ʹ��alarm + siglongjmp ʵ�ֵġ���alarm�ڶ��߳�������ʱ������ͼ��������ܡ����ֻ��ʹ��alarm�������ᵼ�³�����������ǣ��ټ���siglongjmp����Ҫ���� ����������ĺܿ��£�core�м���������������Ϣ������Ϊ����Ҫһ��sigjmp_buf�͵�ȫ�ֱ��������߳��޸�������ͨ������£�����ÿ���߳�һ�� sigjmp_buf �͵ı�������������£����߳���ʹ�� siglongjmp ��û������ģ�����libcurlֻ��һ��ȫ�ֱ��������е��̶߳����ã���
//    ���������� curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L) �ĳ�ʱ���ã�����alarm��ʹ�ã����Ʒ��������������׶Σ�����ǰ���������ڶ��߳����ǲ��еġ������ʽ�ǽ��õ�alarm���ֳ�ʱ�� curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L)��
//    */
//    curl_easy_setopt( c, CURLOPT_NOSIGNAL, 1L );
//    /*
//    CURLOPT_HEADER����Ϊ1�����ڷ��ص����������http header��
//    CURLOPT_FOLLOWLOCATION����Ϊ0���򲻻��Զ�301��302��ת��
//    CURLOPT_NOBODY: ����㲻��������а���body���֣��������ѡ��Ϊһ������ֵ��
//    CURLOPT_FAILONERROR: ���������PHP�ڷ�������(HTTP���뷵�ش��ڵ���300)ʱ������ʾ���������ѡ��Ϊһ�˷���ֵ��Ĭ����Ϊ�Ƿ���һ������ҳ�����Դ��롣
//    CURLOPT_TIMEOUT: ����һ��������������Ϊ������������롣
//    CURLOPT_LOW_SPEED_LIMIT: ����һ���������������ƴ��Ͷ����ֽڡ�
//    CURLOPT_LOW_SPEED_TIME: ����һ���������������ƶ����봫�� CURLOPT_LOW_SPEED_LIMIT �涨���ֽ�����
//    */
//
//    curl_easy_setopt( c, CURLOPT_LOW_SPEED_LIMIT, 1000L );
//    curl_easy_setopt( c, CURLOPT_LOW_SPEED_TIME, 5L );
//
//    // todo: ��Ҫ�賬ʱ
//    curl_easy_setopt( c, CURLOPT_FILE, (void *)123 );
//    curl_easy_setopt( c, CURLOPT_WRITEFUNCTION, process );               /* send all data to this function  */
//    curl_easy_perform( c ); /* get it! */
//    log( "sssssssssssssss" );
//    curl_easy_cleanup( c ); /* cleanup curl stuff */
//    curl_global_cleanup( );
//    return false;
//}
// ɨ�ļ���, �����Ƿ� ��ʲô�ļ� �Ѿ������� ͬ���ȼ� ������������( �����Ͻ������ܳ���������� )
// plan A:
//      ������ļ��Ѿ��������ȼ�������������( ��״̬Ϊδ���� ), ���Ŀ������н����ļ��Ƶ���ǰ���ȼ�����
//      �����ǰ���в�δ��������ļ�( ����ȡ�������� ), ���ƹ������ļ� ���ص����ȼ�����( ���� _requests �����µ� )
// plan B:
//      ����������ļ����ظ�, ������ǰ, �� lock check FileIndex( Ҳ����˵��д������ ), ������ļ��������ػ�У���ѹ֮��, ������
// plan C:
//      ������� Ҫ���ص��ļ� ��λ��ĳ���ȼ������ض���, ����Щ�ļ��� "ǰ" ��, ��������
//      ���ض�����û�е��ļ�, ��嵽��Ӧ���ȼ�������ǰ��


