#ifndef _LIST_BUFFER_H_
#define _LIST_BUFFER_H_


/*

�������������ݰ����жѵ��ݴ�������

�ڴ��ṹ��
---------------------
|size | next | data  |
|____________________|
4[+4]  4[/8]    n

�������̣�
����������������״̬( 1:����ͷ; 2:������� )
1. ����.push( socket.receive() )
2. if ״̬ == 1 and ����.size < 6
   or ״̬ == 2 and ����.size < ��.�� then goto 1.
3. if ״̬ == 1 {
    ��.�� = ����.copy( 4 bytes )
    if ����.size >= ���� {
        goto 4
    }
    else {
        ״̬ = 2
        goto 1
    }
4. ��.���� = ����.copy( ��.�� )
    ����.pop( ��.�� )
    ״̬ = 1
    ����( ������ )   �ⲽ�п����ǽ����� ѹ��һ�����У�Ȼ����engineÿ֡��ĳʱ��ȡ������
    goto 1
 
��ע��ԭ�������õ���MCL_ROUND_SIZE���ĵط����Ѹ�Ϊ��round2n���������ڴ���䳤��ȡ���պô��� x �� 2^n ֵ���������ڴ��������
���磬 round2n( 1 ) == 2,  round2n( 9 ) == 16, round2n( 36 ) = 64,  round2n( 4097 ) = 8192

 */
class ListBuffer
{
public:
    ListBuffer( int block_min_size = (int)NetConfigs::ListBufferMinBlockSize );
    ~ListBuffer();

    void push( const char *buf, int len );
    int copy( char *buf, int len );
    int pop( int len );
    void clear();

    int size() const;
    int empty() const;

private:
    struct Block
    {
        int     size;
        Block * next;
        char * data() const;
    };

    void append_block( int len );
    void free_block( Block * block );

    Block *     _header;
    int         _header_ptr;

    Block *     _tail;
    int         _tail_ptr;

    int         _block_min_size;
    int         _size;
};

#endif
