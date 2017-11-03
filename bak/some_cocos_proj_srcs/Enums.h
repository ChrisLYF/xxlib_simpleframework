#ifndef __ENUMS_H__
#define __ENUMS_H__

// ��ǰ��Ŀ���� enum �����ڴ˴� ���㻥��, ͳһ����, ���� lua ��

// ��������������ò���
enum class NetConfigs : int
{
    // send ʱ�õ��� buffer ��
    BufferSize = 1024,

    // Ҫ send �����ݵ��ۼƳ�( ������ )
    MaxWriteBufferSize = 327680,

    // ÿ�ε��� send ���޳�
    MaxSendSizePerFrame = 8192,

    // �������� select �ĵȴ���ʱʱ�� sec : usec
    WorkerSelectTimeOutSecond = 1,
    WorkerSelectTimeOutUSecond = 0,

    // �����۴�������ݵ� list buffer ����ÿ�������С�ߴ�
    ListBufferMinBlockSize = 1024,

    // ���ݰ������Ķ���
    PacketBufferSize = 32768,
};


// socket ����״̬
enum class SocketStates
{
    // δ֪( ��ʼ, Ĭ��̬)
    Unknown,

    // ���
    Alive,

    // �ѹر�
    Closed,

    // ����
    Dead
};


// �����¼�
enum class NetEvents
{
    // δ֪( Ĭ��ֵ )
    Unknown,

    // ������
    Connected,

    // ��ʧ��
    ConnectFailed,

    // �ѹر�
    Closed,

    // �յ�����( ��ӳ�䵽 LUA )
    Received,

    // �յ����ݰ�
    ReceivedPacket,
};


// ������������
enum class NetTypes
{
    // ����
    None,

    // ������
    Wifi,

    // �绰��
    Wwan
};


// http ������
enum class HttpRequestResults
{
    // �ɹ�
    Success,

    // ʧ��
    Fail
};


// �ļ�����¼�����( ���������ؼ��ز��� )
enum class FileEventTypes                                       // �·��� ����˵��
{
    /************************************************************************/
    /* LoadIndex ���                                                       */
    /************************************************************************/

    // ��ʼִ��
    LoadIndex_Started,                                          // db���ļ���, baseURL

    // ��ʼ���� Web db
    LoadIndex_DownloadBegin,                                    // web db �ļ�ȫ��, ��������URL

    /////////////////////////////////
    // ���ڼ����� Download_ ϵ���¼�
    /////////////////////////////////

    // Web db �������
    LoadIndex_DownloadFinished,

    // ���� Web db
    LoadIndex_WebLoaded,

    // ������ԭʼ���е� db
    LoadIndex_PackageLoaded,

    // �Ѽ�� Runtime db �Ƿ����
    LoadIndex_RuntimeChecked,

    // �Ѵ� Package ���Ƶ� ��д��
    LoadIndex_RuntimeCopyed,                                    // Դ�ļ�ȫ��, runtime db �ļ�ȫ��

    // ������ Runtime db
    LoadIndex_RuntimeLoaded,                                    // runtime db �ļ�ȫ��

    // ��ʹ�� Original db ������ Runtime db ( ����������ԭʼ�� )
    LoadIndex_RuntimeUpdatedByPackage,                          // Ҫ������ runtime db �ļ�ȫ��, ������ packet db �ļ�ȫ��

    // ʹ�� Web db ������ Runtime db
    LoadIndex_RuntimeUpdatedByWeb,                              // Ҫ������ runtime db �ļ�ȫ��, ������ web db �ļ�ȫ��

    // LoadIndex ���
    LoadIndex_Finished,

    //////////////////////////////////////////////////////////////////////////
    // ��־λ�¼�
    LoadIndex_Error_Begin,

    // sqlite ��ʧ��
    LoadIndex_OpenDBError,                                      // �ļ�ȫ��

    // ��ԭʼ������ db ����д�� ��
    LoadIndex_CopyFileError,                                    // Դ�ļ�ȫ��, Ŀ���ļ�ȫ��

    // ����ʧ��
    LoadIndex_LoadError,                                        // �ļ�ȫ��

    // update db ��
    LoadIndex_UpdateError,                                      // ������db�ļ�ȫ��

    // ��־λ�¼�
    LoadIndex_Error_End,
    //////////////////////////////////////////////////////////////////////////


    /************************************************************************/
    /* Download ���                                                        */
    /************************************************************************/

    // ��ʼ����
    Download_Started,                                           // ���ļ���( ��ͨ������ȡ�� FileIndexRow ��ȡ��ϸ��Ϣ )

    // �ѻ�ȡ������Ϣ
    Download_GotLength,                                         // ���ļ���, ��������, �Ѵ��ڵ��ļ�����

    // �ļ�׷��д��( ����¼�ͨ���ᷢ���ܶ��, ���ļ����ȳ����� )
    Download_Appending,                                         // ���ļ���, ����׷�ӳ���

    // ������( δУ�� )
    Download_Downloaded,                                        // ���ļ���

    // crc �ļ�׷��д��( crc �ļ���֪������ ��ͨ���ܶ� )
    Download_CRCAppending,                                      // ���ļ���, ����׷�ӳ���

    // ������ crc �ļ�( δУ�� )
    Download_CRCDownloaded,                                     // ���ļ���, crc�ļ�����

    // ����У�� crc32, md5
    Download_Checksuming,                                       // ���ļ���, crc32, md5

    // ��У��
    Download_Checksumed,                                        // ���ļ���, crc32, md5

    // ���ڽ�ѹ
    Download_Extracting,                                        // ���ļ���

    // �ѽ�ѹ
    Download_Extracted,                                         // ���ļ���

    // ���ؽ���( ��У��, ��ѹ )
    Download_Finished,                                          // ���ļ���

    //////////////////////////////////////////////////////////////////////////
    // ��־λ�¼�
    Download_Error_Begin,

    // ������������ش����¼�

    // ���ر���ֹ( crc, zip ����ֹҲ����� )
    Download_Stoped,

    // ��ʱ
    Download_Timeout,

    // fopen �ļ�����
    Download_OpenFileError,

    // ȡ����ʱ����
    Download_GetLengthError,

    // ��ʼ�� curl ʧ��
    Download_InitCurlError,

    // д�ļ���( ���д�ļ������ص� return 0 ��ֹ����Ҳ��õ�������� )
    Download_WriteFileError,

    // ���ػ�Ӧ����
    Download_ResponseError,


    // ������У����ش����¼�

    // CRC �ļ����ݸ�ʽ��
    Download_CRCFormatError,

    // md5 У���
    Download_MD5Error,

    // crc32 У���
    Download_CRC32Error,

    // �����ǽ�ѹ��ش����¼�

    // zip �ļ� fopen ʧ��
    Download_OpenZipError,

    // �� zip ��Ϣ����
    Download_ReadZipInfoError,

    // �� ��ѹ��Ϣ ��
    Download_ReadFileInfoError,

    // ����Ŀ¼��
    Download_CreateDirError,

    // �� zip �е��ļ�ʧ��
    Download_OpenZipFileError,

    // fopen Ŀ���ļ�ʧ��
    Download_OpenDestFileError,

    // �� zip �е��ļ�ʧ��
    Download_ReadZipFileError,

    // �� zip �е���һ���ļ� ʧ��
    Download_ReadNextFileError,

    // ��־λ�¼�
    Download_Error_End,
    //////////////////////////////////////////////////////////////////////////

    /************************************************************************/
    /* Load ���                                                            */
    /************************************************************************/

    // ��ʼִ��
    Load_Started,                                               // ���ļ���, Ҫ���ص��ļ�����, Ҫ�첽���ص���ͼ����, Ҫͬ�����ص���Դ����

    // ׼�������б�
    Load_DownloadInit,                                          // Ҫ���ص��ļ����ֽ���

    // ��ʼ���� �����ļ� 
    Load_DownloadBegin,                                         // �ļ�ȫ��, URL

    /////////////////////////////////
    // ���ڼ����� Download_ ϵ���¼�, �����ǳ���, Ҳ���������˳�( �������� downloader ������ )
    /////////////////////////////////

    // ������� 
    Load_DownloadFinished,                                      // �ļ�ȫ��, URL

    // �����첽������ͼ
    Load_AsyncLoading,                                          // �ļ���

    // �첽�������
    Load_AsyncLoaded,                                           // �ļ���

    // ���� cache
    Load_AsyncCaching,                                          // �ļ���

    // �� cache
    Load_AsyncCached,                                           // �ļ���

    // ����ͬ������plist
    Load_SyncLoading,                                           // �ļ���

    // ͬ���������
    Load_SyncLoaded,                                            // �ļ���

    // ���� Load �������
    Load_Finished,                                              //


    //////////////////////////////////////////////////////////////////////////
    // ��־λ�¼�
    Load_Error_Begin,

    // �ۺ����������  todo: ��һ��ϸ��
    Load_Error,                                                 // ������Ϣ

    // ����ʧ��( �����������������Ծ���ʧ�� )
    Load_DownloadFailed,

    // ��־λ�¼�
    Load_Error_End,
    //////////////////////////////////////////////////////////////////////////

};


// �ļ�����״̬
enum class FileStates : int
{
    // �����. ��������е���ʱ, ��Ҫ�� fileIndexRow �� updateState ����
    Finished,

    // ��Ҫ��
    NeedDownload,

    // ������
    Downloading,

    // ��Ҫ�� crc
    CRCNeedDownload,

    // ������ crc
    CRCDownloading,

    // ��ҪУ��
    NeedChecksum,

    // ����У��
    Checksuming,

    // ��Ҫ��ѹ
    NeedExtract,

    // ���ڽ�ѹ
    Extracting
};


// �ļ�����
enum class FileTypes : int
{
    Unknown,
    // �����Ǹ���ͼƬ/��ͼ
    TextureTypes_Begin = 100,                     // ռλ��_��ʼ( �����ж��Ƿ�Ϊ��ͼ )   
    Jpg,
    Png,
    Pvr,
    Ccz,                                    // pvr.ccz
    Dds,
    Webp,
    TextureTypes_End,                       // ռλ��_����( �����ж��Ƿ�Ϊ��ͼ )
    // �����Ӱ�
    SoundTypes_Begin = 200,
    BgWav,                                  // ��������
    BgMp3,                                  // ��������
    BgCaf,                                  // ��������
    BgOgg,                                  // ��������
    Wav,                                    // ǰ����Ч
    Mp3,                                    // ǰ����Ч
    Caf,                                    // ǰ����Ч
    Ogg,                                    // ǰ����Ч
    SoundTypes_End,
    
    MiscTypes_Begin = 300,
    FramePlist,                             // sprite frame ������
    SpineAtlas,                             // spine ��ͼ������
    SpineJson,                              // spine �������嵵
    Lua,                                    // ���� lua script
    ParticlePlist,                          // particle quad ������
    Db,                                     // sqlite db
    TTF,                                    // true type font
    MiscTypes_End,
    // todo: others, ����
};

// ExtSprite �� shader ����
enum class ShaderTypes
{
    None,
    Gray,
    Blur,
};


// ExtButton �� states
enum class ButtonStates
{
    Normal,
    HighLighted,
    Disabled,
    Selected
};

// ExtButton �� touch �¼�
enum class ButtonTouchEvents
{
    Down,
    DragInside,
    DragOutside,
    DragEnter,
    DragExit,
    UpInside,
    UpOutside,
    Cancel,
};

// ExtScrollView �� �ƶ�����
enum class ScrollViewDirection
{
    // ����
    Horizontal,

    // ����
    Vertical,

    // ˫��
    Both
};

// ExtScrollView �� action �¼�( todo: ��ǰֻʵ���� Stoped �¼� )
// �����Ͻ�ÿ���¼������ظ�����( ������ֱ��ȡֵ )
enum class ScrollViewEvents
{
    Begin,

    // ��ָ���ڻ���( ÿ frame )
    Draging,

    // ��ָ̧�������ƶ�( ÿ frame )
    Moving,

    // ������Ե, ��ָ�ſ��� ���� ������( ���� )
    Bounce,

    // ��ȫͣ����( ���� )
    Stoped,
};

// ���ı�ÿ�л��߶��뷽ʽ
enum class RichTextLineVAligns
{
    Top,
    Middle,
    Bottom
};






// ����ʧ�� ������
enum class BuyFailedTypes
{
    BuyErrorUnknown,            // �⼸������ʧ�ܺ�� PaymentTrans.error.code.  ��ͬ�� ios SK ��ͷ�ļ��� enum
    BuyErrorClientInvalid,
    BuyErrorPaymentCancelled,
    BuyErrorPaymentInvalid,
    BuyErrorPaymentNotAllowed,
    BuyErrorStoreProductNotAvailable,
};

// У����� ������
enum class VerifyErrorTypes
{
    OpenUrlFailed,              // �򲻿�У����ַ
    ResponseError,              // HTTP ��Ӧ����
    ResponseFormatError,        // HTTP ��Ӧ���ݸ�ʽ����
    VerifyTimeout,              // ȥappleУ�鳬ʱ���ͻ��˿�����Ҫ���� ���԰�ť

    Xxxxxxxxx,                  // todo: more errors
    ApplicationIdError,
    WrongProductId,
};

enum class PurchaseEventTypes
{
    PullSuccess,                // ����Ʒ��ɹ�
    Pulling,                    // ��������Ʒ��( ��������δ���� )

    BuyInvalidId,               // �Ҳ������ֹ�� product ID( ������ _products ���û���ҵ� )

    Buying,                     // �����򡣶�Ӧ ios �¼� enum Ϊ SKPaymentTransactionStatePurchasing
    BuySuccess,                 // ��ɹ�����Ӧ ios �¼� enum Ϊ SKPaymentTransactionStatePurchased
    BuyRestored,                // ���ڶ��ĳɹ�����Ӧ ios �¼� enum Ϊ SKPaymentTransactionStateRestored
    BuyFailed,                  // ��ʧ�ܡ���Ӧ ios �¼� enum Ϊ SKPaymentTransactionStateFailed. �¼�����Ϊ BuyFailedTypes

    // ����ɹ���
    VerifySuccess,              // У��ɹ���δ��¼���Ѽ�¼������Ϸ��δ���ţ�֪ͨ��Ϸ������, �ͻ��˵���Ϸ����Ϣ����transId����֮��رն���
    Verifyed,                   // У����ͨ��, ��Ϸ���ѷ��ţ�ֱ�ӹرն������ͻ�������Ӧ��ʾ
    VerifyedAnother,            // У����ͨ��, ����ֵ��ɫ���ǵ�ǰ��ɫ, ��Ϸ���ѷ��ţ�ֱ�ӹرն������ͻ��˲���ʾ
    VerifyTimeout,              // apple У�鳬ʱ�� �ͻ���Ӧ����ʾ����
    VerifyInvalidReceipt,       // У�� status != 0, �ͻ���Ӧ��ʾ�Ƿ�����֮��
    VerifyError,                // ����У������������溬����־

    // for exchange
    ExchangeSuccess,            // �һ��ɹ�( ����һ���ͳ�ֵ�ɹ��������ȴ� )
    ExchangeUsed,               // �����ѱ��ù�
    ExchangeInvalid,            // ��Ч����
    ExchangeElapsed,            // ���ڿ���
    ExchangeError,              // ���ֳ����������溬����־
};



#endif
