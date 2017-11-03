#include "Precompile.h"
#include "cJSON.h"


MAKE_INSTANCE_CPP( FileManager );



FileManager::FileManager()
    : _running( true )
    , _disposed( true )
    , _state( FileManagerState::None )
    , _load_imageFilesCounter( 0 )
    , _load_resFilesCounter( 0 )
    , _lua_eh_loadIndex( 0 )
    , _lua_eh_download( 0 )
    , _lua_eh_load( 0 )
    , _asyncLoading( false )
{
    for( auto &d : _downloaders )
    {
        d.setFileIndexRowMutex( &_fileIndexRowMutex );
    }
    thread( &FileManager::process, this ).detach();
}

FileManager::~FileManager()
{
    _running = false;
    while( !_disposed ) sleepMS( 50 );
    vector<FileIndexRow*> tmp;
    for( auto& o : _loadLog )
    {
        tmp.push_back( o.first );
    }
    for( auto& o : tmp )
    {
        unloadCore( o );
    }
}

void FileManager::registerLoadIndexEventHandler( EventHandlerType eh )
{
    _eh_loadIndex = eh;
}

void FileManager::registerLoadIndexEventHandler( LUA_FUNCTION eh )
{
    _lua_eh_loadIndex = eh;
}

void FileManager::registerLoadEventHandler( EventHandlerType eh )
{
    _eh_load = eh;
}

void FileManager::registerLoadEventHandler( LUA_FUNCTION eh )
{
    _lua_eh_load = eh;
}

void FileManager::registerDownloadEventHandler( EventHandlerType eh )
{
    _eh_download = eh;
}

void FileManager::registerDownloadEventHandler( LUA_FUNCTION eh )
{
    _lua_eh_download = eh;
}


bool FileManager::loadIndex( string const & fileName, string const & baseURL )
{
    if( !_eh_loadIndex && !_lua_eh_loadIndex )
    {
        CCLOG( "%s", "FileManager::loadIndex        need registerLoadIndexEventHandler !" );
        return false;
    }
    {
        lock_guard<mutex> g( _stateMutex );
        if( _state != FileManagerState::None )
        {
            CCLOG( "%s", "FileManager::loadIndex        _state != None !" );
            return false;
        }
        _state = FileManagerState::IndexLoading;
    }
    _loadIndex_fileName = fileName;
    _baseURL = baseURL;

    return true;
}

void FileManager::download( vector<string> const & files )
{
    for( auto &fn : files )
    {
        if( auto row = getFileIndexRow( fn ) )                       // ��Ӧ���ļ�����
        {
            lock_guard<mutex> g( _fileIndexRowMutex );
            if( row->state == FileStates::NeedDownload
                || row->state == FileStates::CRCNeedDownload
                || row->state == FileStates::NeedChecksum
                || row->state == FileStates::NeedExtract )
            {
                _download_files.push( row );
            }
        }
        else
        {
            CCLOG( "%s%s", "FileManager::download       getFileIndexRow( fn )     can't find fileName: ", fn.c_str() );
        }
    }

    // ��̨������ػص����� loadIndex �ɹ���ע��
}

bool FileManager::load( vector<string> const & files )
{
    {
        lock_guard<mutex> g( _stateMutex );
        if( !_eh_load && !_lua_eh_load )
        {
            CCLOG( "%s", "FileManager::load     need registerLoadEventHandler !" );
            return false;
        }
        if( _state == FileManagerState::None )
        {
            CCLOG( "%s", "FileManager::load     _state == None !" );
            return false;
        }
        if( _state == FileManagerState::IndexLoading )
        {
            CCLOG( "%s", "FileManager::load     _state == IndexLoading !" );
            return false;
        }
        if( _state == FileManagerState::Loading )       // �Ѿ���ִ����
        {
            CCLOG( "%s", "FileManager::load     _state == Loading !" );
            return false;
        }
    }

    _load_files.clear();

    for( auto &fn : files )
    {
        if( auto row = getFileIndexRow( fn ) )
        {
            _load_files.push_back( row );

            lock_guard<mutex> g2( _fileIndexRowMutex );
            if( row->state == FileStates::NeedDownload
                || row->state == FileStates::CRCNeedDownload
                || row->state == FileStates::NeedChecksum
                || row->state == FileStates::NeedExtract )
            {
                _load_needDownloadFiles.push( row );
            }
        }
        else
        {
            log( "%s%s", "FileManager::load       getFileIndexRow( fn )     can't find fileName: ", fn.c_str() );
            printf( "%s%s", "FileManager::load       getFileIndexRow( fn )     can't find fileName: ", fn.c_str() );
            
        }
    }

    setState( FileManagerState::Loading );
    return true;
}

void FileManager::unloadCore( FileIndexRow* fir )
{
    // loadLog �������

    auto it = _loadLog.find( fir );
    if( it == _loadLog.end() )
    {
        CCLOG( "unload error, loadLog can't find fir = %s", fir->name.c_str() );
        return;
    }
    it->second--;
    if( it->second )
    {
        return;
    }
    _loadLog.erase( it );

    auto tc = Director::getInstance()->getTextureCache();
    switch( fir->type )
    {
    case FileTypes::Jpg:            // ���ֵ�ͼ���ͣ�ֻҪ��ͼ�����þͲ��Ƴ�
    case FileTypes::Png:
    case FileTypes::Pvr:
    case FileTypes::Ccz:
        tc->tryRemoveTextureForKey( fir->name );
        break;
    case FileTypes::FramePlist:     // ��ɨ Texture ��Ӧ�� SpriteFrames �Ƿ񻹴������ã��оͲ�ɾ��û�о�һ��ɾ
        if( SpriteFrameCache::getInstance()->tryRemoveSpriteFramesFromFile( fir->name ) )
        {
            tc->tryRemoveTextureForKey( fir->name );
        }
        break;
    case FileTypes::Wav:
    case FileTypes::Mp3:
    case FileTypes::Caf:
    case FileTypes::Ogg:
        SimpleAudioEngine::getInstance()->unloadEffect( fir->name.c_str() );
        break;
    case FileTypes::BgWav:
    case FileTypes::BgMp3:
    case FileTypes::BgCaf:
    case FileTypes::BgOgg:
        SimpleAudioEngine::getInstance()->stopBackgroundMusic( true );
        break;
    default:
        break;
    }

}


void FileManager::unloadCore( vector<FileIndexRow*> & firs )
{
    for( auto &fir : firs )
    {
        unloadCore( fir );
    }
}

bool FileManager::unload( vector<string> const & files )
{
    if( _state == FileManagerState::None )
    {
        CCLOG( "%s", "FileManager::download     _state == None !" );
        return false;
    }
    if( _state == FileManagerState::IndexLoading )
    {
        CCLOG( "%s", "FileManager::download     _state == IndexLoading !" );
        return false;
    }
    if( _state == FileManagerState::Loading )
    {
        CCLOG( "%s", "FileManager::download     _state == Loading !" );
        return false;
    }

    for( auto &fn : files )
    {
        if( auto fir = getFileIndexRow( fn ) )
        {
            unloadCore( fir );
        }
        else
        {
            CCLOG( "%s%s", "FileManager::unload       getFileIndexRow( fn )     can't find fileName: ", fn.c_str() );
        }
    }
    return true;
}





void FileManager::process()
{
    function<void()> f;

    while( _running )       // ��ѭ��ֱ��֪ͨ�˳�
    {
        {
            lock_guard<mutex> g( _stateMutex );

            // ·��Ҫִ�еĺ�����
            if( _state == FileManagerState::None )      // δ��ʼ��ʱ�ȿ�ת
            {
                goto Lab_Sleep;
            }
            else if( _state == FileManagerState::IndexLoading )
            {
                f = [ this ] { loadIndex(); };
            }
            else if( _state == FileManagerState::Downloading )
            {
                f = nullptr;                            // ��̨���ؾ��� load �����º�ָ�
                //f = [ this ] { download(); };
            }
            else
            {
                f = [ this ] { load(); };
            }
        }

        if( f )
        {
            f();
            continue;
        }

Lab_Sleep:
        sleepMS( 50 );
        continue;
    }
    _disposed = true;       // �ɹ��˳�ѭ��ʱ���Ϊ���˳�
}

// db ver �ṹ
struct Ver
{
    int ver;
    int len;
    string md5;

    // ���ļ���������, ��ʧ�ܽ����� false, �ɹ����� true
    inline bool readFromJson( string const & fn )
    {
        // ��ȡ ver ������
        auto s = Utils::fileGetText( fn );

        // תΪ json ��
        auto cj = cJSON_Parse( s.c_str() );

        // ʧ���˳�
        if( !cj ) return false;

        // ȷ������ cj
        Utils::ScopeGuard autoDeleteCJ( [ cj ] { cJSON_Delete( cj ); } );

        // ��ʼһϵ�еĶ�ȡ
        if( auto cj_item = cJSON_GetObjectItem( cj, "ver" ) )
            ver = cj_item->valueint;
        else
            return false;

        if( auto cj_item = cJSON_GetObjectItem( cj, "md5" ) )
            md5 = cj_item->valuestring;
        else
            return false;

        if( auto cj_item = cJSON_GetObjectItem( cj, "len" ) )
            len = cj_item->valueint;
        else
            return false;

        // �ɹ�����
        return true;
    }
};

void FileManager::loadIndex()
{
    // ������:
    // ��� ����ʱ db  δ�ҵ�, �� �ڿ�дĿ¼ �����ձ�
    // ���� ����ʱ db
    // �ж� ���� db .ver �ļ�������, ���� ���� db �İ汾��
    // ��� ���� db �汾�� ������ʱ db �Ĵ�, �� ������ db ���Ƶ� ��дĿ¼, load �� �� ����ʱ db.upgrade( ���� db )
    // ���� web db ver �ļ�����дĿ¼, ���� web db �İ汾��
    // ��� web db �汾�� ������ʱ db �Ĵ�, �� load �� �� ����ʱdb.upgrade( web db )
    // ����ʱ db ��������ļ���

    // ����: .ver �ļ�, ��һ�� json, ���� ver ( db �İ汾�� ), md5( ���غ��У���� ), len( ���س��Ȳο� ) ��������

    // ׼������
    //

    // ���õ��ļ�ϵͳ
    auto fu = FileUtils::getInstance();

    // ����ʱdb �����ļ���
    auto fn_rt = Global::WritablePath + _loadIndex_fileName;
    // web db �����ļ���
    auto fn_web = fn_rt + ".web";
    // web db ver �����ļ���
    auto fn_web_ver = fn_rt + ".ver.web";
    // ���Ƴ����� pkg db �����ļ���
    auto fn_pkg = fn_rt + ".pkg";
    // pkg db �����ļ���
    auto fn_ori = Global::ResourcePath + _loadIndex_fileName;
    // pkg db ver �����ļ���
    auto fn_ori_ver = fn_ori + ".ver";
    // web db ����·��
    auto url_web = _baseURL + _loadIndex_fileName;
    // web db ver ����·��
    auto url_web_ver = url_web + ".ver";

    // ��������״̬�ĺ���
    auto err = [ = ]( string const & s )
    {
        setState( FileManagerState::None );
        _events_loadIndex.push( FileEvent( FileEventTypes::LoadIndex_LoadError, s ) );
    };


    // ���� runtime db �ļ�·��
    _fileIndex.setFileName( fn_rt );

    // ע: �����ձ�ʽ��Ϊ�� android ���޷��������� ���²��뻺��������
    // �����и�˵���� String sqliteDir = "/data/data/" + getApplicationContext().getPackageName() + "/databases";
    // ��ǰ���� .......files/data/ ���洴�����ļ�, ��֪���ĳ� .......databases/  �᲻���޸�

    // ����ļ��Ƿ����
    // ��������Ӱ��ڸ���һ��
    if( !fu->isFileExist( fn_rt ) && !Utils::fileCopy( fn_ori, fn_rt ) )
    {
        err( "Utils::fileCopy( fn_ori, fn_rt ) error" );
        return;
    }

    // ��������ʱ db
    if( !_fileIndex.load() )
    {
        err( "_fileIndex.load() error" );
        return;
    }

    // �� ���� db .ver �ļ�������
    Ver ver_pkg = { 0, 0 };
    if( !ver_pkg.readFromJson( fn_ori_ver ) )
    {
        err( "ver_pkg.readFromJson( fn_ori_ver ) error" );
        return;
    }

    // ���� db ������ʱ db ��( ���������� )
    if( ver_pkg.ver > _fileIndex.getVersion() )
    {
        // ������db ���Ƶ���д��( ��Ϊ android ��db�� zip ���޷��ı�, ��ǰ��δ���� VFS �� sqlite �򿪷�ʽ )
        if( !Utils::fileCopy( fn_ori, fn_pkg ) )
        {
            err( "Utils::fileCopy( fn_ori, fn_pkg ) error" );
            return;
        }

        // ���� �Ѹ��Ƶ� pkg db
        FileIndex fi_pkg( fn_pkg );
        if( !fi_pkg.load() )
        {
            err( "fi_pkg.load() error" );
            return;
        }

        // ��������ʱ db
        if( !_fileIndex.upgrade( fi_pkg, true, ver_pkg.ver ) )
        {
            err( "_fileIndex.upgrade( fi_pkg, true, ver_pkg.ver ) error" );
            return;
        }
    }


    // ��ʼ���� web db ver
    // ����������������������
    FileDownloadTask dt;
    FileIndexRow fir;
    dt.fileIndexRow = &fir;
    Ver ver_web = { 0, 0 };

    // ����һ��������
    auto &d = _downloaders[ 0 ];

    // ���ڴ�ŵ�ǰ�����¼�����( �����ַ�ʽ��֪ͨ ����� while �ȴ��� ������Ӧ����Ϊ )
    auto et = (FileEventTypes)0;

    // �������������
    if( _baseURL.size() )
    {
        // ��ʼ
        _events_loadIndex.push( FileEvent( FileEventTypes::LoadIndex_Started
            , _baseURL, _loadIndex_fileName ) );

        // ׼������ web db

        // ��ʼ��������
        d.registerDownloadEventHandler( [ &]( FileEvent && e )      // ע�������¼��ص�
        {
            et = e.eventType;                                       // �赱ǰ�¼�����
            _events_loadIndex.push( move( e ) );
        } );

        // ɾ�� ver �ļ�
        Utils::fileDelete( fn_web_ver );

        // α�� FileindexRow for ���� ver
        fir.name = fn_web_ver;
        fir.fullName = fn_web_ver;              // ָ��д���ļ���
        fir.size = 0;                           // ����δ֪
        fir.crc32 = 0;                          // ��У��
        fir.md5 = "";                           // ��У��
        fir.state = FileStates::NeedDownload;   // ��Ҫ����
        dt.fullURL = url_web_ver;               // ָ�������ļ���
        dt.currentDownloadSize = 0;             // ������ size ��ʼ��Ϊ 0
        dt.originalFileSize = 0;                // �ļ���ɾ, ��ǰ�ߴ�Ϊ 0

        // ��ʼ����
        _events_loadIndex.push( FileEvent( FileEventTypes::LoadIndex_DownloadBegin
            , fir.name, dt.fullURL ) );
        d.download( dt );

        // �����ؽ���
        // �Ǵ����¼�����Ҫ�����ȴ� ���ڿ�����չ cancel. �ɼ������˳�
        while( !( et >= FileEventTypes::Download_Finished
            && et < FileEventTypes::Download_Error_End ) )
        {
            if( !_running ) return;
            sleepMS( 50 );
        }

        // ������س���, �˳�
        if( et != FileEventTypes::Download_Finished )
        {
            setState( FileManagerState::None );
            return;
        }

        // ���سɹ�
        _events_loadIndex.push( FileEvent( FileEventTypes::LoadIndex_DownloadFinished
            , fir.name, dt.fullURL ) );

        // ��ʼȡ ver ������
        if( !ver_web.readFromJson( fn_web_ver ) )
        {
            err( "ver_web.readFromJson( fn_web_ver ) err" );
            return;
        }

        if( ver_web.ver > _fileIndex.getVersion() )  // web db �汾�ȱ�����, ��Ҫ����
        {
            // ɾ���ļ�
            Utils::fileDelete( fn_web );

            // ������������
            fir.name = fn_web;
            fir.fullName = fn_web;
            fir.size = ver_web.len;
            fir.crc32 = 0;
            fir.md5 = ver_web.md5;
            fir.state = FileStates::NeedDownload;
            dt.fullURL = url_web;
            dt.currentDownloadSize = 0;
            dt.originalFileSize = 0;

            // ��֮ǰ���¼�
            et = (FileEventTypes)0;

            // ��ʼ����
            _events_loadIndex.push( FileEvent( FileEventTypes::LoadIndex_DownloadBegin
                , fir.name, dt.fullURL ) );
            d.download( dt );

            // �����ؽ���
            // �Ǵ����¼�����Ҫ�����ȴ� ���ڿ�����չ cancel. �ɼ������˳�
            while( !( et >= FileEventTypes::Download_Finished
                && et < FileEventTypes::Download_Error_End ) )
            {
                if( !_running ) return;
                sleepMS( 50 );
            }

            // ������س���, �˳�
            if( et != FileEventTypes::Download_Finished )
            {
                setState( FileManagerState::None );
                return;
            }

            // ���سɹ�
            _events_loadIndex.push( FileEvent( FileEventTypes::LoadIndex_DownloadFinished
                , fir.name, dt.fullURL ) );

            // ���� web db
            FileIndex fi_web( fn_web );
            if( !fi_web.load() )
            {
                err( "fi_web.load() err" );
                return;
            }

            // ����
            if( !_fileIndex.upgrade( fi_web, false, ver_web.ver ) )
            {
                err( "_fileIndex.upgrade( fi_web, false, ver_web.ver )" );
                return;
            }
        }
    }

    // ��������ļ���
    _fileIndex.fillFullName();

    // ���: ��״̬Ϊ Downloading
    setState( FileManagerState::Downloading );
    _events_loadIndex.push( FileEvent( FileEventTypes::LoadIndex_Finished ) );


    // ע���̨���������ṩ���¼��ص�
    download();
}



void FileManager::download()
{
    // ����������������������
    auto d = &_downloaders[ 0 ];         // ָ��ָ�� [0] ������ ���㸴�Ƶ� lambda

    auto ts = [ this, d ]
    {
        // ���ϵĴ� _download_files ��ȡ����
        FileIndexRow* fir;
        if( _download_files.pop( fir ) )
        {
            lock_guard<mutex> lg( _fileIndexRowMutex );
            if( fir->state == FileStates::Finished ) return;
            d->download( FileDownloadTask( _baseURL + fir->name, fir ) );
        }
    };

    auto cb = [ this, d ]( FileEvent && e )
    {
        // ��� e Ϊ����, ���������·Ż� _download_files
        if( e.eventType > FileEventTypes::Download_Error_Begin
            && e.eventType < FileEventTypes::Download_Error_End )
        {
            _download_files.push( (FileIndexRow*)e.userData );
        }
        if( e.eventType == FileEventTypes::Download_Finished )
        {
            auto fir = (FileIndexRow*)e.userData;               // ����֮���db
            lock_guard<mutex> lg( _fileIndexRowMutex );
            fir->state = FileStates::Finished;
            _fileIndex.dbUpdateState( fir );
        }
        // �� e �ƽ��¼�����
        _events_download.push( move( e ) );
    };

    d->registerSupplierAndHandler( ts, cb );
}



void FileManager::load()
{
    // ��������:
    // �� ��ǰ�ĺ�̨������ �´� stop ָ��, ���� ����ֹͣ��ǰ������Ϊ
    // �������������� �����ṩ���л�Ϊ�� load �б�����, �¼�ѹ�� load �¼�����
    // ��ĳһ�������� ��������ʱ, ���������� �Ż� load �б�, ��������������� �ṩ��, ���ټ���������������
    // �ȴ�����������, ͨͨ��Ϊ δ���� ̬,
    // ֮���� load �б� �Ƿ��Ѿ�Ϊ��, Ҫ load �������ļ��� FileState �Ƿ��� Finished
    // �������� ����� �첽����( ��׶������������������� )
    // ���������� �򱨴� ���粻����֮��
    // �첽����, �����ļ�������·����Ӧ��ʽ.
    // С�ļ�����. ����˳�


    // �����ⲽ������ǰ, �Ա� Load_Started ʱ�� lua ��� ��������ļ���

    // ����ɼ�����
    _load_imageFilesCounter = 0;
    vector<FileIndexRow*> firs_async;       // ��ʱ���� for �첽����

    // ɨ����Ҫ�� TextureCache  addImageAsync ���ļ�
    for( auto fir : _load_files )
    {
        if( fir->type > FileTypes::TextureTypes_Begin && fir->type < FileTypes::TextureTypes_End )
        {
            firs_async.push_back( fir );
        }
    }


    // ����ɼ�����
    _load_resFilesCounter = 0;
    vector<FileIndexRow*> firs_sync;        // ��ʱ���� for ͬ������


    // ɨ����Ҫͬ�����ص��ļ�
    for( auto fir : _load_files )
    {
        if( fir->type != FileTypes::FramePlist
            && !( fir->type > FileTypes::SoundTypes_Begin && fir->type < FileTypes::SoundTypes_End )
            ) continue;
        firs_sync.push_back( fir );
    }

    // ��ʼ�¼�, ��� ���ļ���, Ҫ���ص��ļ�����, Ҫ�첽���ص���ͼ����, Ҫͬ�����ص���Դ����
    _events_load.push( FileEvent( FileEventTypes::Load_Started
        , (int)_load_files.size(), (int)_load_needDownloadFiles.size(), (int)firs_async.size(), (int)firs_sync.size() ) );

    // ���ж��Ƿ���Ҫ����
    if( !_load_needDownloadFiles.empty() )
    {
        // �����ֽ���
        int totalSize = 0;
        _load_needDownloadFiles.foreach( [ &totalSize ]( FileIndexRow*& o ) { totalSize += o->size; } );
        _events_load.push( FileEvent( FileEventTypes::Load_DownloadInit, totalSize ) );

        int numFiles = _load_needDownloadFiles.size();
        atomic<int> finishCounter( 0 );
        atomic<int> errorCounter( 0 );
        for( auto &downloader : _downloaders )
        {
            // ȡָ������, ���㸴�Ƶ� lambda
            auto d = &downloader;
            auto ts = [ this, d ]
            {
                // ���ϵĴ� _load_files ��ȡ����
                FileIndexRow* fir;
                if( _load_needDownloadFiles.pop( fir ) )
                {
                    d->download( FileDownloadTask( _baseURL + fir->name, fir ) );
                }
            };
            auto cb = [ this, d, &errorCounter, &finishCounter ]( FileEvent && e )
            {
                // ��� e Ϊ����, ��ǰ��������ֹ ��ȡ����, �黹��ǰ�������� ���˳�( ������� +1 )
                if( e.eventType > FileEventTypes::Download_Error_Begin
                    && e.eventType < FileEventTypes::Download_Error_End )
                {
                    d->registerSupplierAndHandler( nullptr, nullptr );
                    _load_needDownloadFiles.push( (FileIndexRow*)e.userData );
                    ++errorCounter;
                }
                else if( e.eventType == FileEventTypes::Download_Finished )
                {
                    ++finishCounter;        // �ۼӼ�����, �� db
                    auto fir = (FileIndexRow*)e.userData;
                    lock_guard<mutex> lg( _fileIndexRowMutex );
                    fir->state = FileStates::Finished;
                    _fileIndex.dbUpdateState( fir );
                }
                // �� e �ƽ��¼�����
                _events_load.push( move( e ) );
            };
            d->registerSupplierAndHandler( ts, cb );
        }

        _events_load.push( FileEvent( FileEventTypes::Load_DownloadBegin ) );

        // �ȴ�������ɻ��˳�
        while( true )
        {
            if( errorCounter >= _countof( _downloaders ) )
            {
                for( auto &downloader : _downloaders )
                {
                    // ��� downloader �Ĺ���
                    downloader.registerSupplierAndHandler( nullptr, nullptr );
                }

                setState( FileManagerState::Downloading );
                _events_load.push( FileEvent( FileEventTypes::Load_DownloadFailed ) );

                // ��ʧ�ܶ��˳�
                _load_needDownloadFiles.clear();
                return;
            }

            if( finishCounter == numFiles )
            {
                for( auto &downloader : _downloaders )
                {
                    // ��� downloader �Ĺ���
                    downloader.registerSupplierAndHandler( nullptr, nullptr );
                }
                // �ָ���̨����
                download();

                // ���� while, ����������첽����
                break;
            }
            sleepMS( 50 );
        }

        setState( FileManagerState::Downloading );
        _events_load.push( FileEvent( FileEventTypes::Load_DownloadFinished ) );
    }

    // ��ʼ�첽����

    // ����: ���� load �������ļ�( ���������ṩ .plist , .atlas �����������õ��������ļ��������б� ), 
    //      ����ͼ��, ��Ҫ �첽�������ŵ� cache; ��ͼ��, ͨ���첽����, load

    // ǰ�÷���:
    // 1. ����һ�� LRUCache �� FileUtils ֮ getData/getText ��Ƕ��;
    // 2. ���� c2dx ����, ������Ҫ�������� ����֧�� ���ļ�������� Data ���ط�ʽ

    // ���뷽��:
    // 1. push ���� ��ͼ�ļ��� ���̼߳��ض���, ��ǰ�߳� wait
    // 2. �����߳� ֡�ص� check ���̼߳��ض���, foreach ���� TextureCache ֮ addImageAsync
    // 3. �ص������ۼӼ�����, ����ǰ�̵߳� wait ɨ
    // 4. ����������֮ǰ push ���ļ�����ͬʱ, ֹͣ wait
    // 5. foreach ���� ����ͼ�ļ�
    // 6. ͨ�� FileUtils ȡ���� Data, push �����߳�, ��ǰ�߳� wait
    // 7. �����߳� ֡�ص� check Data, ��ͬ����ʽ����, ����ɱ�־
    // 8. ��ǰ�̵߳� wait ɨ�� ��ɱ�־, goto 5 ( Ҫ��Ҫ��Ƴ�ʱ���� ��˵ )
    // 9. foreach ���, ѹ����¼�, �˳�


    // ��¼ load ��־
    for( auto& fir : _load_files )
    {
        auto it = _loadLog.insert( make_pair( fir, 1 ) );
        if( !it.second )
        {
            it.first->second++;

            // �Ѽ���, �� firs_async firs_sync ���Ƴ�, ������¼�
            {
                auto i = find( firs_async.begin(), firs_async.end(), fir );
                if( i != firs_async.end() )
                {
                    firs_async.erase( i );
                    _events_load.push( FileEvent( FileEventTypes::Load_AsyncLoaded, fir->name ) );
                }
            }

            {
                auto i = find( firs_sync.begin(), firs_sync.end(), fir );
                if( i != firs_sync.end() )
                {
                    firs_sync.erase( i );
                    _events_load.push( FileEvent( FileEventTypes::Load_SyncLoaded, fir->name ) );
                }

            }

        }
    }


    // ��ɨ���� ��ͼ �ļ�������������
    for( auto fir : firs_async )
    {
        _load_imageFiles.push( fir );
        _events_load.push( FileEvent( FileEventTypes::Load_AsyncLoading, fir->name ) );
    }

    // �� image file �첽�������: check ����ɵĸ���
    while( (int)firs_async.size() > _load_imageFilesCounter )
    {
        if( !_running )
        {
            _load_imageFiles.clear();
            return;
        }
        sleepMS( 50 );
    }

    // ��ɨ������Ҫͬ�����ص��ļ����� cache
    for( auto fir : firs_sync )
    {
        // ���� sound ������( ���� c2dx �ļ�ϵͳ����, ֱ�Ӿ���ϵͳ�� api ��ͨ���ļ�·��ֱ�� )
        if( fir->type > FileTypes::SoundTypes_Begin && fir->type < FileTypes::SoundTypes_End ) continue;

        //_events_load.push( FileEvent( FileEventTypes::Load_AsyncCaching, fir->name ) );
        (void)Utils::fileGetData( fir->name );
//        std::this_thread::sleep_for( std::chrono::milliseconds( 1000 ) );
        //_events_load.push( FileEvent( FileEventTypes::Load_AsyncCached, fir->name ) );
    }

    // ��ɨ�����ļ�������������
    for( auto fir : firs_sync )
    {
        _events_load.push( FileEvent( FileEventTypes::Load_SyncLoading, fir->name ) );
        _load_resFiles.push( fir );
    }

    // �����߳�ͬ���������
    while( (int)firs_sync.size() > _load_resFilesCounter )
    {
        //if( !_running )
        //{
        //    _load_resFiles.clear();
        //    return;
        //}
        sleepMS( 50 );
    }

    // �ɹ��˳�
    setState( FileManagerState::Downloading );
    _events_load.push( FileEvent( FileEventTypes::Load_Finished ) );
}

void FileManager::update( float dt )
{
    static int frame_idx = 0;
    frame_idx++;

    auto tc = Director::getInstance()->getTextureCache();

    FileIndexRow* fir = nullptr;

    // �첽������ͼ
    if( !_asyncLoading && _load_imageFiles.pop( fir ) )
    {
        _asyncLoading = true;
        tc->addImageAsync( fir->name, [ this, fir ]( Texture2D* tex )
        {
            // �ӳַ�ɾ
            tex->retain();

            // ˳�㿴��Ҫ��Ҫ�ؿ����
            if( fir->noaa )
            {
                tex->setAliasTexParameters();
            }

            _events_load.push( FileEvent( FileEventTypes::Load_AsyncLoaded, fir->name ) );
            ++_load_imageFilesCounter;
            _asyncLoading = false;
        } );

        goto LabPushEvents;
    }

    //struct timeval now;
    //struct timeval now2;
    //auto t1 = [ &]
    //{
    //    //gettimeofday( &now, nullptr );
    //};
    //auto t2 = [ &, this ]
    //{
    //    //gettimeofday( &now2, nullptr );
    //    //CCLOG( "************************ preload %d, %dus, dt: %0.3f, fi: %u, fn: %s"
    //    //       , (int)_load_resFilesCounter
    //    //       , ( (int)now2.tv_sec - (int)now.tv_sec ) * 1000000 + (int)now2.tv_usec - (int)now.tv_usec
    //    //       , dt
    //    //       , frame_idx
    //    //       , fir->name.c_str() );
    //};
    //auto t3 = [ &, this ]( const char* m )
    //{
    //    //gettimeofday( &now2, nullptr );
    //    //CCLOG( "------------------------ %dus, %s"
    //    //       , ( (int)now2.tv_sec - (int)now.tv_sec ) * 1000000 + (int)now2.tv_usec - (int)now.tv_usec
    //    //       , m );
    //};

    // ͬ������
    if( _load_resFiles.pop( fir ) ) // �� while ��Ϊ�� if
    {
        switch( fir->type )
        {
        case FileTypes::FramePlist:
            //t1();
            SpriteFrameCache::getInstance()->addSpriteFramesWithFile( fir->name, true );
            //t2();
            break;
        case FileTypes::BgWav:
        case FileTypes::BgMp3:
        case FileTypes::BgCaf:
        case FileTypes::BgOgg:
            //t1();
            // ��Ԥ��������Ч����Ϊ android ���ƺ�ֻ��Ԥ�� 3 ~ 5 �� )
            // SimpleAudioEngine::getInstance()->preloadBackgroundMusic( fir->name.c_str() );
            //t2();
            break;
        case FileTypes::Wav:
        case FileTypes::Mp3:
        case FileTypes::Caf:
        case FileTypes::Ogg:
        {
            //t1();
            // SimpleAudioEngine::getInstance()->preloadEffect( fir->name.c_str() );
            //t2();
            break;
        }
            //case FileTypes::ParticlePlist:
            //    tc->addImage( Utils::getTextureFileNameFromPlistFile( fir->name ) )->retain();
            //    break;
        default:break;
        }
        // maybe have another file type
        _events_load.push( FileEvent( FileEventTypes::Load_SyncLoaded, fir->name ) );
        ++_load_resFilesCounter;
    }

LabPushEvents:

    // �� event �� handlers
    FileEvent e, e2;

    auto push = [ &]
    {
        LuaHelper::push( (int)e.eventType );
        for( auto p : e.parameters )
        {
            if( p.isNumeric )
            {
                LuaHelper::push( p.value_numeric );
            }
            else
            {
                LuaHelper::push( p.value_string );
            }
        }
        return e.parameters.size() + 1;
    };

    for( int i = 0; i < 2; ++i )
    {
        if( _events_loadIndex.pop( e ) )
        {
            if( _eh_loadIndex ) _eh_loadIndex( e );
            if( _lua_eh_loadIndex )
            {
                LuaHelper::executeFunction( _lua_eh_loadIndex, push() );
            }
        }
        else break;
    }

    for( int i = 0; i < 2; ++i )
    {
        if( _events_download.pop( e ) )
        {
            if( _eh_download ) _eh_download( e );
            if( _lua_eh_download )
            {
                LuaHelper::executeFunction( _lua_eh_download, push() );
            }
        }
        else break;
    }
   
    for( int i = 0; i < 2; ++i )
    {
        if( _events_load.pop( e ) )
        {
            // �ϲ���������׷�ӵ��¼�
            if( e.eventType == FileEventTypes::Download_Appending )
            {
                // ��ȡ��һ���¼�, �����, ��Ϊ ����׷��, ��Ϊ ��ͬ�ļ�, ��ϲ�, ����ȡ, �ظ��������ֱ�����ܺϲ�Ϊֹ
                while( _events_load.pop( e2, [ &]( FileEvent const & item )
                {
                    return item.eventType == FileEventTypes::Download_Appending
                        && e.parameters[ 0 ].value_string == item.parameters[ 0 ].value_string;
                } ) )
                {
                    e.parameters[ 1 ].value_numeric += e2.parameters[ 1 ].value_numeric;
                }
            }

            if( _eh_load ) _eh_load( e );
            if( _lua_eh_load )
            {
                LuaHelper::executeFunction( _lua_eh_load, push() );
            }
        }
        else break;
    }
}

FileIndexRow* FileManager::getFileIndexRow( string const & fn )
{
    // �� check fn �Ƿ�Ϊ ����·��
    // ����·��: ������������·�� ���ң� �ҵ��򷵻�
    // ���·��: �� search path ��ƴ�ӳ���� key ������

    if( FileUtils::getInstance()->isAbsolutePath( fn ) )
    {
        if( fn.find_first_of( Global::WritablePath, 0 ) == 0 )
        {
            auto key = fn.substr( Global::WritablePath.size() );
            return _fileIndex.getRow( key );
        }
        else if( fn.find_first_of( Global::ResourcePath, 0 ) == 0 )
        {
            auto key = fn.substr( Global::ResourcePath.size() );
            return _fileIndex.getRow( key );
        }
        else
        {
            CCLOG( "FileManager::getFileIndexRow can't find file: %s", fn.c_str() );
            return nullptr;
        }
    }

    FileIndexRow* rtv = nullptr;
    for( auto sp : Global::SearchPaths )
    {
        auto key = sp + fn;
        rtv = _fileIndex.getRow( key );
        if( rtv ) return rtv;
    }
    return rtv;
}

FileIndex* FileManager::getIndex()
{
    return &_fileIndex;
}

void FileManager::setState( FileManagerState v )
{
    lock_guard<mutex> g( _stateMutex );
    _state = v;
}


