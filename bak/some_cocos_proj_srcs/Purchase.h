#ifndef _PURCHASE_H_
#define _PURCHASE_H_

class Purchase;

class BuyInfo : public Ref
{
    std::string _productId;
    std::string _verifyURL;
    int _charId;
    int _accountId;
    int _regionId;
    std::string _accountName;
    std::string _transactionId;
    std::string _transactionBase64;
    std::string _exchangeURL;

    friend Purchase;
public:
    BuyInfo();
    ~BuyInfo();
    BuyInfo( std::string const& productId
             , std::string const& verifyURL
             , std::string const& accountName
             , int charId
             , int accountId
             , int regionId
             , std::string const& transactionId
             , std::string const& transactionBase64
             , std::string const& exchangeURL
             );
    BuyInfo( BuyInfo const& other );
    BuyInfo( BuyInfo&& other );
    BuyInfo& operator=( BuyInfo const& other );
    BuyInfo& operator=( BuyInfo&& other );
    bool operator==( BuyInfo const& other );

    /************************************************************************/
    // ���к��� lua �¿���
    /************************************************************************/

    static BuyInfo* create();

    std::string const& getProductId() const;
    void setProductId( std::string val );
    std::string const& getVerifyURL() const;
    void setVerifyURL( std::string val );
    int getCharId() const;
    void setCharId( int val );
    int getAccountId() const;
    void setAccountId( int val );
    int getRegionId() const;
    void setRegionId( int val );
    std::string const& getAccountName() const;
    void setAccountName( std::string val );
    std::string const& getTransactionId() const;
    void setTransactionId( std::string val );
    std::string const& getTransactionBase64() const;
    void setTransactionBase64( std::string val );
    std::string const& getExchangeURL() const;
    void setExchangeURL( std::string val );
};





class Product : public Ref
{
    std::string _pid;
    std::string _title;
    std::string _desc;
    std::string _price;

    friend Purchase;
public:
    Product();
    ~Product();
    Product( std::string const& pid
             , std::string const& title
             , std::string const& desc
             , std::string const& price );
    Product( Product const& other );
    Product( Product&& other );
    Product& operator=( Product const& other );
    Product& operator=( Product&& other );
    bool operator==( Product const& other );

    /************************************************************************/
    // ���к��� lua �¿���
    /************************************************************************/

    static Product* create();

    std::string const& getPid() const;
    void setPid( std::string val );
    std::string const& getTitle() const;
    void setTitle( std::string val );
    std::string const& getDesc() const;
    void setDesc( std::string val );
    std::string const& getPrice() const;
    void setPrice( std::string val );
};





class Products
{
    friend Purchase;
    Products();
    ~Products();

public:
    std::vector<Product*> _data;

    /************************************************************************/
    // ���к��� lua �¿���
    /************************************************************************/

    void insert( Product* p );
    void erase( std::string const& pid );
    int size() const;
    bool empty() const;
    void clear();
    Product* at( int idx ) const;
};







class Purchase
{
    MAKE_INSTANCE_H( Purchase );
public:
    typedef std::function<void( GenericEvent<PurchaseEventTypes> const& )> EventHandlerType;

    // ע���¼��ص�
    void registerEventHandler( EventHandlerType f );

    // ���º�����ɨ���ִ����������������¼��ص�
    void update();

    // ����һ���¼�
    template<typename ET>
    void pushEvent( ET&& e )
    {
        _events.push( std::forward<ET>( e ) );
    }


    // for IOS
    // ������ʵ��Ϊƽ̨����
    // ��������ֱ��ʹ��
    // �� ����id ������ ���� web server У��
    // web server ���Ȳ����ݿ⣬���У�����״̬·��
    // ���δУ�飬�򷢵� apple server У�飬ͨ����֪ͨ GS ����
    // �� game server ������ɺ󣬿ͻ���Ӧ�û���Ҫ�յ�һ������, ������ tid, ���ڹرս���
    void verify();

    /************************************************************************/
    // ���к��� lua �¿���
    /************************************************************************/

    // ������ʵ��Ϊƽ̨����
    // ���ù���������( ios �����ý��׶��м��� )
    // ����Ϸ�������Ǻ����( �������в������� )
    void start( std::string const& verifyURL,
                int charId,
                int accountId,
                int regionId,
                std::string const& accountName,
                std::string const& exchangeURL
                );


    // ������ʵ��Ϊƽ̨����
    // ios ��ֹͣ���׶��м���
    void stop();

    // ע���¼��ص�( ���Ҫ��д LUA )
    void registerEventHandler( LUA_FUNCTION f );

    // ��Ӳ�Ʒ�� _products���� id ����ƽ̨�������������ͬ��
    void add( std::string const& id
              , std::string const& title
              , std::string const& desc
              , std::string const& price );

    // �� _products �Ƴ���Ʒ
    void remove( std::string const& id );

    // �õ���Ʒ�б�
    Products* getProducts();
    
    // �õ�������Ϣ
    BuyInfo* getBuyInfo();

    // for IOS
    // ������ʵ��Ϊƽ̨����
    // ��ʼ����Ʒ��( android, windows ����Ҫ�� )
    bool pull( std::vector<std::string> const& pids );

    // ������ʵ��Ϊƽ̨����
    // IOS: ʹ��ϵͳSDK, ���������ã���ʼ�򣨴�ʱ��Ϸ��ȴ� BuyXxxx, VerifyXxxx �¼��� ������֪ͨ��
    // ����ɹ��� bi �� trans ��ؽ������
    bool buy( std::string const& pid );

    // for IOS
    // ������ʵ��Ϊƽ̨����
    // �رս���( ͨ�����յ���Ϸ��֪ͨ����� ), ���� BuyInfo( ����������д��ڵĻ� )
    void close( std::string const& transId );


    // ȫƽ̨
    // ����������кţ�������������Ӧ�Ķ���
    void exchange( std::string const& serial );

private:
    // �¼�����ص�
    EventHandlerType _eventHandler;

    // �¼����У����� update ʱ����
    ThreadSafeQueue<GenericEvent<PurchaseEventTypes>> _events;

    // ��Ҫ������ʾ�Ĳ�Ʒ��
    Products _products;

    // ����������
    BuyInfo _buyInfo;
};

#endif

