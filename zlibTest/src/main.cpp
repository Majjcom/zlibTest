
#include "zlib1/zlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#define BUFF_SIZE 8192

int compress(Bytef* src, uLong src_len, Bytef* dst, uLong* dst_len)
{
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;

    if (deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, MAX_WBITS + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK)
    {
        printf("init fail...\n");
        return -1;
    }

    stream.next_in = src;
    stream.avail_in = src_len;
    stream.next_out = dst;
    stream.avail_out = *dst_len;

    while (stream.avail_in != NULL && stream.total_out < *dst_len)
    {
        if (deflate(&stream, Z_NO_FLUSH) != Z_OK)
        {
            printf("deflate fail...\n");
            return -2;
        }
    }

    if (stream.avail_in != NULL)
    {
        printf("in stream not cleared...\n");
        return -3;
    }

    for (;;)
    {
        int err;
        if ((err = deflate(&stream, Z_FINISH)) == Z_STREAM_END)
            break;
        if (err != Z_OK)
        {
            printf("deflate finish fail: %d\n", err);
            return -4;
        }
    }

    if (deflateEnd(&stream) != Z_OK)
    {
        printf("deflate end fail...\n");
        return -5;
    }

    *dst_len = stream.total_out;
    return 0;
}

int compress2(Bytef* src, uLong src_len, Bytef* dst, uLong* dst_len)
{
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;

    if (deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, MAX_WBITS + 16, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY) != Z_OK)
    {
        printf("init fail...\n");
        return -1;
    }

    stream.next_in = src;
    stream.avail_in = src_len;

    Bytef* buff = (Bytef*)malloc(BUFF_SIZE);
    int lastout = 0;
    while (stream.avail_in != 0 && stream.total_out < *dst_len)
    {
        stream.next_out = buff;
        stream.avail_out = BUFF_SIZE;
        int err = deflate(&stream, Z_NO_FLUSH);
        if (err >= 0)
        {
            int new_data_size = stream.total_out - lastout;
            memcpy_s(dst + lastout, *dst_len - lastout, buff, stream.total_out - lastout);
            lastout = stream.total_out;
            if (err == Z_FINISH)
                break;
        }
        if (err != Z_OK)
        {
            printf("deflate fail...\n");
            free(buff);
            return -1;
        }
    }

    free(buff);
    
    while (stream.total_out < *dst_len)
    {
        stream.next_out = dst + lastout;
        stream.avail_out = *dst_len - lastout;
        int err = deflate(&stream, Z_FINISH);
        if (err == Z_STREAM_END)
            break;
        if (err != Z_OK)
        {
            printf("deflate finish fail...\n");
            return -1;
        }
    }

    if (deflateEnd(&stream) != Z_OK)
    {
        printf("deflateEnd fail...\n");
        return -1;
    }

    *dst_len = stream.total_out;

    return 0;
}

int decompress(Bytef* src, uLong src_len, Bytef* dst, uLong* dst_len)
{
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.next_in = src;

    if (inflateInit2(&stream, MAX_WBITS + 16) != Z_OK)
    {
        printf("inflateInit2 fail...\n");
        return -1;
    }

    Bytef* buff = (Bytef*)malloc(BUFF_SIZE);

    int lastout = 0;
    while (stream.total_in < src_len && stream.total_out < *dst_len)
    {
        stream.next_out = buff;
        stream.avail_out = BUFF_SIZE;
        stream.avail_in = BUFF_SIZE;
        int err = inflate(&stream, Z_NO_FLUSH);
        if (err >= 0)
        {
            memcpy_s(dst + lastout, *dst_len - lastout, buff, stream.total_out - lastout);
            lastout = stream.total_out;
            if (err == Z_STREAM_END)
                break;
        }
        if (err != Z_OK)
        {
            printf("inflate fail...\n");
            free(buff);
            return -2;
        }
    }

    free(buff);
    stream.next_in = NULL;
    stream.avail_in = 0;
    stream.next_out = NULL;
    stream.avail_out = 0;

    if (inflateEnd(&stream) != Z_OK)
    {
        printf("inflateEnd fail...\n");
        return -3;
    }

    return 0;
}

int main()
{
    FILE* fp;
    
    if (fopen_s(&fp, "MayPackCreator.exe", "rb"))
    {
        printf("file open fail...\n");
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    uLong blen = (uLong)ftell(fp);
    fseek(fp, 0, SEEK_SET);
    Bytef* src = (Bytef*)malloc(blen);

    if (!src)
    {
        printf("out of memory...\n");
        return -1;
    }

    fread_s(src, blen, sizeof(Bytef), blen, fp);
    fclose(fp);

    uLong clen = compressBound(blen);

    Bytef* out = (Bytef*)malloc(sizeof(Bytef) * clen);
    if (!out)
    {
        printf("out of memory...\n");
        return -1;
    }
    memset(out, 0, sizeof(Bytef) * clen);
    if (compress2(src, blen, out, &clen))
    {
        printf("compress fail...\n");
    }

    if (!fopen_s(&fp, "test.exe.gz", "wb"))
    {
        fwrite(out, sizeof(Bytef), clen, fp);
        fclose(fp);
    }

    Bytef* de = (Bytef*)malloc(sizeof(Bytef) * blen);
    if (!de)
    {
        printf("out of memory...\n");
        return -1;
    }
    memset(de, 0, sizeof(Bytef) * blen);
    if (decompress(out, clen, de, &blen))
    {
        printf("decompress fail...\n");
        return -1;
    }

    free(out);

    errno_t err;
    if (err = fopen_s(&fp, "test.exe", "wb") != 0)
    {
        printf("file create fail...\n");
        return -1;
    }

    fwrite(de, sizeof(Bytef), blen, fp);
    fclose(fp);
    free(de);

    return 0;
}

