#ifndef _RICHTEXTHELPER_H_
#define _RICHTEXTHELPER_H_

namespace RichTextHelper
{
    enum class RTCmds
    {
        Paragraph,
        ParagraphEnd,

        UnderLine,
        UnderLineEnd,

        Button,
        ButtonEnd,

        Font,
        Color,
        Offset,
        Image,
        Align,
        Space,
        Height,
        Variant,

        // ����Ĳ��������˾����������� �������ڱ�� ���ı��ֲ�����
        Text,
        NewLine
    };

    enum class HorizontalAligns
    {
        Left, Center, Right, Unknown
    };
    enum class VerticalAligns
    {
        Top, Middle, Bottom, Unknown
    };

    class Analyzer
    {
    public:
        Analyzer();
        Analyzer( string const& richText, int maxLen = 16384, int maxStackSize = 32 );
        Analyzer& assign( string const& richText, int maxLen = 16384, int maxStackSize = 32 );
        bool analyze();

        function<void()>                                                setError_MissLeftAngleBrackets;
        function<void()>                                                setError_MissRightAngleBrackets;
        function<void()>                                                setError_MissComma;
        function<void()>                                                setError_MissCommand;
        function<void()>                                                setError_MissParameter;
        function<void( int fontIdx )>                                   setError_MissFontIndex;
        
        function<void( string const& cmdName )>                         setError_WrongCommandName;
        function<void( string const& format )>                          setError_WrongParameterFormat;
        function<void( string const& cmdName )>                         setError_MissEndCommand;
        function<void()>                                                setError_LengthOverflow;
        function<void()>                                                setError_StackOverflow;

        function<void( int colorIdx )>                                  setError_GetColorByIndex;
        function<void( string const& colorName )>                       setError_GetColorByName;
        function<void( string const& key )>                             setError_GetVariant;


        function<bool( string const& colorName, Color3B& c )>           getColorByName;
        function<bool( int colorIndex, Color3B& c )>                    getColorByIndex;
        function<bool( string const& key, string& val )>                getVariant;

        function<bool( int fontIndex )>                                 setFont;
        function<void( Color3B c )>                                     setColor;
        function<void( float offset )>                                  setOffset;
        function<void( HorizontalAligns ah, VerticalAligns av )>        setAlign;
        function<void( float lh, bool isRatio )>                        setLineHeight;

        function<void( string const & txt )>                            addText;
        function<void()>                                                addNewLine;
        function<void( string fn, bool isFileName, float w, float h )>  addImage;
        function<void( float w, float h )>                              addSpace;

        function<bool( float lw )>                                      pushParagraph;
        function<void()>                                                popParagraph;
        function<void( string const& key )>                             pushButton;
        function<void()>                                                popButton;
        function<void( Color3B c, float thickness, float y )>           pushUnderLine;
        function<void()>                                                popUnderLine;

    //private:
        stack<RTCmds> cmds;
        string richText;
        char const* s;              // ָ�� richText.c_str() �ĵ�ǰ�����
        int len;                    // �浱ǰ�����ʣ�೤��
        int maxLen;                 // ʣ�೤������( �� Variant ���ܻ����ʱ�滻�ı����ݣ���ɵݹ����ж� )
        int maxStackSize;
        int so();                   // ȡ s �� richText �� offset
    };

}

#endif
