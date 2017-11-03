#ifndef _FONTMANAGER_H_
#define _FONTMANAGER_H_

// ���������( ���ڴ洢���� �������� �Լ����� Label, ���ı� )
class FontManager
{
public:
    MAKE_INSTANCE_H( FontManager );
    FontManager();
    ~FontManager();

    // ���һ����������, ���� configIndex
    int addFont( string const & fontName, int fontSize = 36, int outLineSize = 0, Color4B outlineColor = Color4B::BLACK );

    // ���һ����ɫ����( ��ȡ���� ), ���� colorIndex
    int addColor( Color3B c, string const & nickname = "" );

    // ����һ��Ĭ��ɫ
    Color3B getDefaultColor();

    // ����ָ����������ɫ
    Color3B getColor( int colorIndex );

    // ����ָ����������ɫ
    Color3B getColor( string const & colorName );

    // �������ñ��, ����һ�� Label
    Label* createLabel( int configIndex, string const & txt = "", int lineWidth = 0 );

    // ����һ������ƽ̨Ĭ��ϵͳ����� Label
    static Label* createSystemFontLabel( string const & txt, int fontSize, int lineWidth = 0 );

    // ��ȡ utf8 ���ַ� �� ĳ���ñ�� ����ʾ�ߴ�, outLen ��䵱ǰλ�� c �� utf8 ���ֵĳ���
    cocos2d::Size getCharSize( int configIndex, char const * c, int & outLen );

    // ��ȡ ASCII �ַ� �� ĳ���ñ�� ����ʾ�ߴ�
    cocos2d::Size getCharSize( int configIndex, char c );

    // ����ĳЩ������ �Ҳ����ͷ��� false �İ汾( �����ɵ� lua )
    bool tryGetCharSize( int configIndex, char const * c, int & outLen, cocos2d::Size& sz );
    bool tryGetCharSize( int configIndex, char c, cocos2d::Size& sz );
    bool tryGetColor( int colorIndex, Color3B& c );
    bool tryGetColor( string const & colorName, Color3B& c );
    int getColorCount();
    int getFontCount();


    // ������
    void clear();
private:
    struct FontConfig
    {
        TTFConfig   ttfConfig;
        int         outLineSize;
        Color4B     outlineColor;
        int         widths[ 128 ];      // ������Ⱦ���ؿ��( widths[0] ������ ȫ���ַ���, 1 ~ 127 �Ǳ��� )
        int         height;             // ������Ⱦ���ظ�
    };
    vector<FontConfig> _fonts;
    vector<Color3B> _colors;
    unordered_map<string, Color3B> _namedColors;

    DELETE_COPY_ASSIGN( FontManager );
};


#endif
