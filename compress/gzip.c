/*
 * ================================
 * eli960@qq.com
 * www.mailhonor.com
 * 2019-03-11
 * ================================
 */

#include <zlib.h>

int gzip_compress_data(const char *data, size_t size, char **out)
{
    int level = 6;
    int encoding = 0x1f;
    int status;
    int ret = -1;
    z_stream Z;
    memset(&Z, 0, sizeof(z_stream));
    Z.zalloc = 0;
    Z.zfree = 0;

    status = deflateInit2(&Z, level, Z_DEFLATED, encoding, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
    if (status != Z_OK) {
        return -1;
    }
#define PHP_ZLIB_BUFFER_SIZE_GUESS(in_len) (((size_t) ((double) in_len * (double) 1.015)) + 10 + 8 + 4 + 1);
    size_t out_len = PHP_ZLIB_BUFFER_SIZE_GUESS(size);
    char *out_buf = (char *)malloc(out_len + 1);
    Z.next_in = (Bytef *)(void *)data;
    Z.next_out = (Bytef *)out_buf;
    Z.avail_in = size;
    Z.avail_out = out_len;
    status = deflate(&Z, Z_FINISH);
    deflateEnd(&Z);

    if (Z_STREAM_END == status) {
        len = Z.total_out;
    } else {
        free(*out_buf);
        len = -1;
    }
    
    return len;
}

int main(int argc, char **argv)
{
}
