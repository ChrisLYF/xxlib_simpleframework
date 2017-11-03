#ifndef __FILEINDEX_H__
#define __FILEINDEX_H__

// �ļ��������¼���
typedef GenericEvent<FileEventTypes> FileEvent;

class FileIndex;
class FileManager;

// �ļ�������( db ���ڴ��е�ӳ�� )
class FileIndexRow
{
public:
    string              name;                   // ·�� + �ļ���( ǰ�治��/ )
    FileTypes           type;                   // �ļ�����
    int                 size;                   // �ļ��ֽڳ���
    int                 version;                // �汾��( �������� )
    int                 crc32;                  // �ļ����ݵ� crc32
    string              md5;                    // �ļ����ݵ� md5
    string              desc;                   // �ļ�����˵��
    FileStates          state;                  // ��ǰ�ļ�״̬( ֻ�� db ���� NeedDownload, Finished. �����м�̬���ڴ�, ����ع� )
    bool                in_pkg;                 // �ļ��Ƿ�����ڰ���( true: ����.     false: λ�ڿ�д�� )
    bool                noaa;                   // �Ƿ񲻿����
    bool                in_base_pkg;            // �ļ��Ƿ�����ڻ�����

    string              fullName;               // ���ڴ������·��( ���ֶβ������������ݿ���, ���Ǽ��غ������ )

    inline void copyTo( FileIndexRow * other )
    {
        other->name = this->name;
        other->type = this->type;
        other->size = this->size;
        other->version = this->version;
        other->crc32 = this->crc32;
        other->md5 = this->md5;
        other->desc = this->desc;
        other->state = this->state;
        other->in_pkg = this->in_pkg;
        other->noaa = this->noaa;
        other->in_base_pkg = this->in_base_pkg;
        other->fullName = this->fullName;
    }

    ~FileIndexRow()
    {
    }

    // for lua
    inline int getVersion() { return version; };
    inline FileStates getState() { return state; };
    inline FileTypes getType() { return type; };
    inline int getSize() { return size; };
    inline string getFullName() { return fullName; };
    inline string getName() { return name; };
    inline bool getInBasePkg() { return in_base_pkg; };
};

// �ļ�����( �����ļ� ��ǰ���� sqlite3 db �ļ���ʽ, ����ʱ�ļ���������Ϸ ��д�����, ��Ϸ����ʱ�� ����, ���� ��db �� update ���� )
// ��ʼ����: Ҫ�� setRootPath, �� setFileName
class FileIndex : public Sqlite::Connection
{
public:
    // Ĭ��ɶ������, ����������� setFileName �������ļ���
    FileIndex();

    // �赱ǰ�ļ���( ȫ·�� )
    FileIndex( string const & fileName );

    // �ر����ݿ�, ���� _rows ���ڴ�
    ~FileIndex();

    // �赱ǰ�ļ���( ȫ·�� )
    void setFileName( string const & fileName );

    // ��ִ��ԭ�� clearQueries ��ͬʱ��һЩָ��
    virtual void clearQueries() override;

    // �� _rows
    void clear();


    // ���� �� �����ݿ��������, ��� db δ��, ��� db. ����Ǻ����Լ��򿪵�, �˳�ʱ���ر� db
    bool create();                          // ����һ���ձ�( ͨ�������ڳ�����ΰ�װ�������� )
    bool load();                            // clear() �� select * ��� _rows
    bool dbReplace( FileIndexRow* row );    // row д��( replace )
    bool dbUpdateState( FileIndexRow* row );// row д��( ֻ update row �� state )
    bool dbDelete( string const & name );   // �� db �� ɾ��һ�� row ���� name

    // �� _rows �� �� ͬ������һ������( ������ open, beginTransaction, add, commit, close  )
    bool insert( FileIndexRow* row );

    // ������һ������, foreach row �� ·��+�ļ��� ��ƥ�� ���ȶ԰汾��. ��һ�� row �İ汾�Ÿ�������� row ��Ϣ����һ��. 
    // ����ǰ��������, ��һ�������� ���汾�� ����� �����������汾��
    // inPkg: �� db �Ƿ�λ�ڰ�װ���� ���ǴӰ�װ���ڸ��Ƴ�����, ���ϵ�����¹���. false: �������µ�
    // ע: ��ǰ����֧��ɾ������� row
    bool upgrade( FileIndex const & other, bool inPkg, int ver );

    // foreach _rows ��� fullName �ֶ��Է������ִ�й�����ʹ��. ͨ�������� upgrade �������֮��
    void fillFullName();

    // ͨ�� �ļ��� �� _rows ���� һ������
    FileIndexRow* getRow( string const & name );

    // ͨ�� �ļ����б� �� _rows ���� ��������( �����ɵ� LUA )
    vector<FileIndexRow*> getRows( vector<string> const & names );

    // ���������ļ���( ������ lua �б��� )
    vector<string> getNames();                  

    // �����ļ�����
    int getVersion();

    friend FileManager;
private:

    string _fileName;                                   // �����ļ���( ȫ·�� ). ͨ�� setFileName ����
    unordered_map<string, FileIndexRow*> _rows;         // �ڴ�����( key Ϊ name )

    Sqlite::Query * _selectQuery;                       // SELECT name, type, size, version, crc32, md5, desc, state FROM fileindex
    Sqlite::Query * _updateQuery;                       // UPDATE fileindex set state = ? where name = ?
    Sqlite::Query * _insertQuery;                       // REPLACE INTO fileindex ( name, type, size, version, crc32, md5, desc, state ) VALUES ( ?, ?, ?, ?, ?, ?, ?, ? )
    Sqlite::Query * _deleteQuery;                       // DELETE FROM fileindex WHERE name = ?

    Sqlite::Query * _setVerQuery;                       // REPLACE INTO string_int ( key, value ) VALUES ( 'fileindex_ver', ? )
    Sqlite::Query * _getVerQuery;                       // SELECT value FROM string_int WHERE key = 'fileindex_ver'

    int _ver;                                           // ��ǰ���ݵİ汾��( �� string_int ��� key: ver �� value ֵ��Ӧ. upgrade ��ˢ�¸�ֵ )

    // = delete
    FileIndex( const FileIndex & );
    FileIndex & operator=( const FileIndex & );
};



#endif

