
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
    unsigned int Misalign;//��ʼλ��
    unsigned int Lenght;//�����С
    unsigned int Offset;//���ݳ���
    char Buffer[1];
}BUFFERCHAIN;


typedef struct __STREAMBUFFER
{
    BUFFERCHAIN * pFirst;
    BUFFERCHAIN * pLast;
    unsigned int uChainBuf;//ÿ������BUFFER����Ĵ�С
    unsigned int uTotal;//buf�ܳ���
    unsigned int uPeekOffet;//����ȡ��,����û�Ƴ���������ݳ���
    volatile unsigned int uMaxBuffer;//��󻺳�,һ��ֻ��д�¼�������Ч
}STREAMBUFFER;

#ifdef __cplusplus
extern "C" {
#endif

    STREAMBUFFER * stream_buffer_new(unsigned int uChainBufferSize);
    void stream_buffer_free(STREAMBUFFER * pStream);

    //������󻺳�
    void stream_buffer_set_maxbuffer(STREAMBUFFER * pStream, unsigned int uSize);
    unsigned int stream_buffer_get_maxbuffer(STREAMBUFFER *pStream);


    //���������
    void stream_buffer_clear(STREAMBUFFER * pStream);

    //д������
    unsigned int stream_buffer_write(STREAMBUFFER * pStream, const char * pBuffer, unsigned int uSize);

    //��������,�����������ݲ������
    unsigned int stream_buffer_peek_read(STREAMBUFFER * pStream, char * pBuffer, unsigned int uSize);

    //����Ϊ��
    void stream_buffer_reset_peek(STREAMBUFFER * pStream);

    //����Ѿ�����������
    void stream_buffer_remove_peek(STREAMBUFFER * pStream);

    //��������, ������Ѷ�ȡ������
    unsigned int stream_buffer_read(STREAMBUFFER * pStream, char * pBuffer, unsigned int uSize);

    //ȡ�������������ݵĳ���
    unsigned int stream_buffer_lenght(STREAMBUFFER * pStream);

    //��д����
    CACHE * cache_new(unsigned int uSize);
    void cache_free(CACHE * pCache);



#ifdef __cplusplus
};
#endif

#endif