#ifndef __NODEDUMPER_H__
#define __NODEDUMPER_H__

// node info dumper
class NodeDumper
{
public:
    NodeDumper( string const & rootName = "n0", int bufSize = 9999999 );
    ~NodeDumper();

    void dump( Node* node, string const & parentName = "", string const & name = "", string const & addToParentCode1 = "", string const & addToParentCode2 = "" );
    void dumpChilds( Node* node, string const & name, string const & addToParentCode1 = "", string const & addToParentCode2 = "" );
    // todo: more bool dumpXxxxx
    bool dumpExtButton( Node* node, string const & parentName, string const & name, string const & addToParentCode1 = "", string const & addToParentCode2 = "", bool inherit = false );
    bool dumpSprite( Node* node, string const & parentName, string const & name, string const & addToParentCode1 = "", string const & addToParentCode2 = "", bool inherit = false );
    bool dumpLabel( Node* node, string const & parentName, string const & name, string const & addToParentCode1 = "", string const & addToParentCode2 = "", bool inherit = false );
    bool dumpLayer( Node* node, string const & parentName, string const & name, string const & addToParentCode1 = "", string const & addToParentCode2 = "", bool inherit = false );
    void dumpNode( Node* node, string const & parentName, string const & name, string const & addToParentCode1 = "", string const & addToParentCode2 = "", bool inherit = false, bool skipContentSize = false );

    char* getBuffer();
    void clear();

    // fn �����·��, ǰ���� /, ���� Global::WritablePath Ϊ��. ����ｨĿ¼
    void saveToFile( string const & fn );

private:
    char * _buf;
    int _bufIdx;
    int _bufSize;
    string _rootName;     // ���ڵ�ı�����

    // delete
    NodeDumper( NodeDumper const & other );
    NodeDumper & operator=( NodeDumper const & );
};

#endif
