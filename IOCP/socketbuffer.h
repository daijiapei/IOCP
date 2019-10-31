
#ifndef __STREAM_BUFFER_H__
#define __STREAM_BUFFER_H__


typedef struct __CACHE
{
    unsigned int Offset;
    char * Buffer;
    unsigned int Lenght;
} CACHE, *LPCACHE;

typedef struct __BUFFERCHAIN
{
#define BUFFERCHAIN_LENGHT 2048
    struct __BUFFERCHAIN * Next;
    unsigned int Misalign;//开始位置
    unsigned int Lenght;//缓冲大小
    unsigned int Offset;//数据长度
    char Buffer[1];
}BUFFERCHAIN;


typedef struct __STREAMBUFFER
{
    BUFFERCHAIN * pFirst;
    BUFFERCHAIN * pLast;
    unsigned int uChainBuf;//每次申请BUFFER链表的大小
    unsigned int uTotal;//buf总长度
    unsigned int uPeekOffet;//被读取了,但是没移除缓冲的数据长度
    volatile unsigned int uMaxBuffer;//最大缓冲,一般只对写事件缓冲有效
}STREAMBUFFER;

#ifdef __cplusplus
extern "C" {
#endif

    STREAMBUFFER * stream_buffer_new(unsigned int uChainBufferSize);
    void stream_buffer_free(STREAMBUFFER * pStream);

    //设置最大缓冲
    void stream_buffer_set_maxbuffer(STREAMBUFFER * pStream, unsigned int uSize);
    unsigned int stream_buffer_get_maxbuffer(STREAMBUFFER *pStream);


    //清除流缓冲
    void stream_buffer_clear(STREAMBUFFER * pStream);

    //写入数据
    unsigned int stream_buffer_write(STREAMBUFFER * pStream, const char * pBuffer, unsigned int uSize);

    //读出数据,但读出的数据不会清除
    unsigned int stream_buffer_peek_read(STREAMBUFFER * pStream, char * pBuffer, unsigned int uSize);

    //重置为零
    void stream_buffer_reset_peek(STREAMBUFFER * pStream);

    //清除已经读出的数据
    void stream_buffer_remove_peek(STREAMBUFFER * pStream);

    //读出数据, 并清除已读取的数据
    unsigned int stream_buffer_read(STREAMBUFFER * pStream, char * pBuffer, unsigned int uSize);

    //取得流缓冲中数据的长度
    unsigned int stream_buffer_lenght(STREAMBUFFER * pStream);

    //读写缓冲
    CACHE * cache_new(unsigned int uSize);
    void cache_free(CACHE * pCache);



#ifdef __cplusplus
};
#endif

#endif