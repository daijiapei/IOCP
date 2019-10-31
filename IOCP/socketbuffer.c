
#include "socketbuffer.h"

#include "iocpdef.h"


static BUFFERCHAIN * buffer_chain_new(unsigned int uLenght)
{
    BUFFERCHAIN * Buffer;

    Buffer = (BUFFERCHAIN *)IOCP_MALLOC(uLenght);
    if (0 == Buffer) return 0;

    memset(Buffer, 0, uLenght);
    Buffer->Lenght = uLenght - sizeof(BUFFERCHAIN);
    return Buffer;
}

static void buffer_chain_free(BUFFERCHAIN * Buffer)
{
    IOCP_FREE(Buffer);
}

STREAMBUFFER * stream_buffer_new(unsigned int uChainBufferSize)
{
    STREAMBUFFER * pStream;

    pStream = (STREAMBUFFER *)IOCP_MALLOC(sizeof(STREAMBUFFER));

    if (NULL == pStream) return NULL;
    memset(pStream, 0, sizeof(STREAMBUFFER));
    pStream->uChainBuf = uChainBufferSize;
    pStream->pFirst = pStream->pLast = NULL;
    return pStream;
}

void stream_buffer_free(STREAMBUFFER * pStream)
{
    stream_buffer_clear(pStream);
    IOCP_FREE(pStream);
}

void stream_buffer_set_maxbuffer(STREAMBUFFER * pStream, unsigned int uSize)
{
    pStream->uMaxBuffer = uSize;
}

unsigned int stream_buffer_get_maxbuffer(STREAMBUFFER *pStream)
{
    return pStream->uMaxBuffer;
}

void stream_buffer_clear(STREAMBUFFER * pStream)
{
    BUFFERCHAIN * pChain = NULL;

    pChain = pStream->pFirst;

    while (pChain)
    {
        pStream->pFirst = pStream->pFirst->Next;

        buffer_chain_free(pChain);

        pChain = pStream->pFirst;
    }

    pStream->pFirst = pStream->pLast = NULL;
    pStream->uPeekOffet = 0;
    pStream->uTotal = 0;
}

unsigned int stream_buffer_write(STREAMBUFFER * pStream, const char * pBuffer, unsigned int uSize)
{
    unsigned int uLenght = 0;
    unsigned int uOffset = 0;
    BUFFERCHAIN * pChain = NULL;

    if (0 == uSize) return 0;

    if (NULL == pStream->pFirst)
    {
        pChain = buffer_chain_new(pStream->uChainBuf);

        if (pChain)
        {
            pStream->pFirst = pStream->pLast = pChain;
        }
    }

    if (pStream->uMaxBuffer)
    {
        if (pStream->uTotal >= pStream->uMaxBuffer)
        {
            uSize = 0;
        }
        else
        {
            uSize = min(uSize, pStream->uMaxBuffer - pStream->uTotal);
        }
    }

    while (pStream->pLast && uSize > uOffset)
    {
        pChain = pStream->pLast;
        uLenght = min(uSize - uOffset, pChain->Lenght - pChain->Offset - pChain->Misalign);

        if (uLenght > 0)
        {
            //有空间可以写入
            memcpy(pChain->Buffer + pChain->Misalign + pChain->Offset, pBuffer + uOffset, uLenght);

            uOffset += uLenght;
            pChain->Offset += uLenght;
        }

        if (uSize > uOffset)
        {
            //未写完,需要申请更多内存空间
            if (NULL == (pChain = buffer_chain_new(pStream->uChainBuf))) break;

            pStream->pLast->Next = pChain;
            pStream->pLast = pChain;
        }
    }
    pStream->uTotal += uOffset;

    return uOffset;
}

unsigned int stream_buffer_peek_read(STREAMBUFFER * pStream, char * pBuffer, unsigned int uSize)
{
    unsigned int uLenght = 0;
    unsigned int uOffset = 0;
    BUFFERCHAIN * pChain = NULL;

    if (0 == uSize) return 0;

    pChain = pStream->pFirst;
    while (uSize > uOffset && pChain)
    {
        uLenght = min(uSize - uOffset, pChain->Offset);

        if (0 == uLenght) break;

        memcpy(pBuffer + uOffset, pChain->Buffer + pChain->Misalign, uLenght);
        uOffset += uLenght;

        pChain = pChain->Next;
    }

    return uOffset;
}

void stream_buffer_reset_peek(STREAMBUFFER * pStream)
{
    pStream->uPeekOffet = 0;
}

void stream_buffer_remove_peek(STREAMBUFFER * pStream)
{
    unsigned int uLenght = 0;
    BUFFERCHAIN * pChain = NULL;

    while (pStream->uPeekOffet)
    {
        pChain = pStream->pFirst;
        uLenght = min(pStream->uPeekOffet, pChain->Offset);

        if (uLenght == pChain->Offset)
        {
            if (pStream->pFirst == pStream->pLast)
            {
                //至少保留一个buffer_chain
                pStream->pFirst->Misalign = pStream->pFirst->Offset = 0;
            }
            else
            {
                pStream->pFirst = pChain->Next;

                buffer_chain_free(pChain);
            }
        }
        else
        {
            //读取了其中一部分, 将已读的部分进行标记
            pChain->Misalign += uLenght;
            pChain->Offset -= uLenght;
        }
        pStream->uPeekOffet -= uLenght;
    }
}

unsigned int stream_buffer_read(STREAMBUFFER * pStream, char * pBuffer, unsigned int uSize)
{
    unsigned int uLenght = 0;
    unsigned int uOffset = 0;
    BUFFERCHAIN * pChain = NULL;
    if (0 == uSize) return 0;

    while (uSize > uOffset && pStream->pFirst)
    {
        pChain = pStream->pFirst;
        uLenght = min(uSize - uOffset, pChain->Offset);

        if (0 == uLenght) break;

        memcpy(pBuffer + uOffset, pChain->Buffer + pChain->Misalign, uLenght);
        uOffset += uLenght;

        if (uLenght == pChain->Offset)//将一个缓冲的内容读完了, free chain
        {
            if (pStream->pFirst == pStream->pLast)
            {
                //至少保留一个buffer_chain
                pStream->pFirst->Misalign = pStream->pFirst->Offset = 0;
            }
            else
            {
                pStream->pFirst = pChain->Next;

                buffer_chain_free(pChain);
            }
        }
        else
        {
            //读取了其中一部分, 将已读的部分进行标记
            pChain->Misalign += uLenght;
            pChain->Offset -= uLenght;
            //到了这里, 说明buffer 已经被填满了 退出
            break;
        }
    }

    pStream->uPeekOffet -= min(pStream->uPeekOffet, uOffset);
    pStream->uTotal -= uOffset;

    return uOffset;
}

unsigned int stream_buffer_lenght(STREAMBUFFER * stream)
{
    return stream->uTotal;
}


CACHE * cache_new(unsigned int uSize)
{
    CACHE * pCache = IOCP_MALLOC(sizeof(CACHE) + uSize);

    if (pCache)
    {
        pCache->Offset = uSize;
        pCache->Lenght = uSize;
        pCache->Buffer = ((char*)pCache) + sizeof(CACHE);
    }

    return pCache;
}


void cache_free(CACHE * pCache)
{
    IOCP_FREE(pCache);
}