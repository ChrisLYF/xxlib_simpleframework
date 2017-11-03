#ifndef _RICHTEXTEX_H_
#define _RICHTEXTEX_H_

/*

ָ��˵��: ( ����ĸ������ָ�����Բ��ո� <>�ڲ࣬�������� �����Լӿո��ı�����\r�������� )

font config reference:
    <f xxx>
        xxx Ϊ addFont �� font manager ��������������( ������, �ֺ�, �����Ϣ )

font color: 
    <c xxx>
        xxx Ϊ 10���� addColor �� font manager ����ɫ��������
        ��� xxx �� �����ִ�ͷ, �� xxx ��������Ϊ ��ɫ���ñ���
    <c #xxxxxx>
        xxxxxx Ϊ 6 �ֽڶ��� RGB 16������
     
offset:
    <o xxx>
        ��������ݵ���ʾ x ���꣨ ����������� ��

image:
    <i xxx>
        xxx Ϊ frame name, ����ԭʼ�ߴ���ʾ
        ��� xxx �� #��ͷ����Ϊ�ļ���
    <i xxx, width, height>
        width height Ϊ������ʾ�Ŀ��, ���ĳ��ֵΪ 0, ���ֵΪ �ȱ�����Ӧ. ����ֵ��Ϊ 0 ��ͬ ԭʼ�ߴ�

under line:
    <u a,b,c>
        a Ϊ��ɫ
        b Ϊ�߿�
        c Ϊ�������ϵ���ʾƫ����
        ��ʾ�»��ߡ������ɵ���ȱʡ
    </u>
        �ر��»���

button:
    <b xxx>
        xxx Ϊ clickHandler ���ò���( string ���� )
    </b>
        button ����������

align:  
    <a xx>
        xx Ϊ L ( left ) R ( right ) C ( center )
        �ӵ�ǰλ�ÿ�ʼ ֱ�� ���з� �� ��β, ���ָ� �����

        xx Ϊ T ( top ) M ( middle ) B ( bottom )
        �� "�ӵ�ǰλ�ÿ�ʼ ֱ�� ��һ�� <a ����>" �� "����" ����ģʽ

        ���������һ�� <a ����>�� ��������ֱ�ӿ�ʼ����Ӧ��������ƣ�֮ǰ�Ļ������ݿ��ܱ�����
        ���Բ��ִ�Сд��ͬʱָ���ݺ����ֶ��뷽ʽ������ <a CM>

space:
    <s xxx, yyy>
        ���� xxx * yyy ���صĿռ�. yyy ��ʡ( ���������� margin Ч�� )

height:
    <h xxx>
        xxx ���������Ϊ0 ��ָ�Ĭ���Ե�ǰ�������Ϊ�иߡ������� xxx ��Ϊ�и�

paragraph:
    <p xxx>
        ������ʼ��־. һ���� ��һ����ʾ ���飬 ������
        xxx Ϊ���ο�ȣ����� �� ��0 ���ֵΪ ��ǰ��ʣ����
    </p>
        ���������־

variant:
    <v xxx>
        xxx Ϊ variantHandler ���ò����� string ���� ), �䷵��ֵ����Ϊ ���ı� Ƕ���ڱ�Ǵ�

*/


class RichTextEx : public Node
{
public:
    // ��ʼ�� Analyzer
    void init_analyzer();

    // button �� ���/touch ����������
    typedef function<void( string const& key )> TouchHandlerType;

    // ���� �� �滻����������
    typedef function<bool( string const& key, string& val )> VariantHandlerType;

    // create ������������
    typedef function<void( string const& err )> ErrorHandlerType;

    // �������ı�����
    static RichTextEx * create( string const & txt
                                , float lineWidth
                                , TouchHandlerType touchHandler = nullptr
                                , VariantHandlerType variantHandler = nullptr
                                , ErrorHandlerType errorHandler = nullptr );

    // �����޸� handler
    void registerTouchEventHandler( TouchHandlerType handler );
    void registerVariantEventHandler( VariantHandlerType handler );
    void registerErrorEventHandler( ErrorHandlerType handler );

    // ��������, ʧ�ܷ��� false, ���󽫴��� _errorHandler ����
    bool assign( std::string const & txt );

    // for lua
    static RichTextEx * createLua( string const & txt
                                , float lineWidth
                                , LUA_FUNCTION touchHandler = 0
                                , LUA_FUNCTION variantHandler = 0
                                , LUA_FUNCTION errorHandler = 0 );
    void registerTouchEventHandler( LUA_FUNCTION handler );
    void registerVariantEventHandler( LUA_FUNCTION handler );
    void registerErrorEventHandler( LUA_FUNCTION handler );

//private:
    TouchHandlerType _touchHandler;
    VariantHandlerType _variantHandler;
    ErrorHandlerType _errorHandler;
    int _lineWidth;
    RichTextHelper::Analyzer _a;
    string _err;

    //bool fillNode( RichTextHelper::Analyzer& a, Node*& container, float lineWidth );
};

#endif
