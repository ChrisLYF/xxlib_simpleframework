#ifndef __FILECACHE_H__
#define __FILECACHE_H__

// LRUCache �ĵ�����װ�� c2dx FileUtils ע�뺯�� bind
// ע: ��ǰδ���� FileUtils::getFileData, getFileDataFromZip ����ע��( �����������ٷ������� )
class FileCache
{
public:
    MAKE_INSTANCE_H( FileCache );

    // ���� c2dx �Ļص� �� FileUtils �ҽ�
    FileCache();

    // ɶ�¶�����
    ~FileCache();

    // ��� cache
    void clear();

private:
    LRUCache<string, Data> _cache;
    mutex _mutex;
};

#endif

