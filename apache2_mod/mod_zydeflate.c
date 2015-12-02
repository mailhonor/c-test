#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "httpd.h"
#include "http_config.h"
#include "http_request.h"
#include "http_core.h"
#include "http_log.h"
#include "http_vhost.h"
//#include "apr_compat.h"
#include <zlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
static int zy_gzip(Bytef *data, unsigned long data_len, char **compr);
static int _etagid_gz(request_rec *r, char *ctype);
extern errno;


/*
 * The default value for the error string.
 */
#ifndef DEFAULT_MODTUT2_STRING
#define DEFAULT_MODTUT2_STRING "apache2_mod_zydeflate: A request was made."
#endif

/*
 * This module
 */
module AP_MODULE_DECLARE_DATA zydeflate_module;

typedef struct {
	char *string;
} modzydeflate_config;
static int mod_zydeflate_method_handler (request_rec *r)
{
	char *filename;
	char *p, *pt;
	char *ctype="";
	//fprintf(stderr, "mod_zydeflate_method_handler here\n");fflush(stderr);
	if(!strncmp("test", r->server->server_hostname, 4)){
	//	fprintf(stderr, "server is % test\n"); fflush(stderr);
		return DECLINED;
	}
	if(!(r->filename))
		return DECLINED;
	//fprintf(stderr, "filename: %s\n", r->filename); fflush(stderr);
	filename = strdup(r->unparsed_uri);
	pt = strrchr(filename, '.');
	if(pt){
		pt++;
		if(!strcasecmp(pt, "html")){
			ctype="text/html";
		}else if(!strcasecmp(pt, "htm")){
			ctype="text/html";
		}else if(!strcasecmp(pt, "txt")){
			ctype="text/plain";
		}else if(!strcasecmp(pt, "js")){
			ctype="application/x-javascript";
		}else if(!strcasecmp(pt, "xml")){
			ctype="text/xml";
		}else if(!strcasecmp(pt, "css")){
			ctype="text/css";
		}
		//fprintf(stderr, "ctype: %s\n", ctype);
		if(*ctype){
			if(_etagid_gz(r, ctype) == 1){
				fflush(stderr);
				return DECLINED;
			}
			fflush(stderr);
			return 0;
		}
	}
	return DECLINED;
}

static void mod_zydeflate_register_hooks (apr_pool_t *p)
{
    ap_hook_handler(mod_zydeflate_method_handler,NULL,NULL,APR_HOOK_LAST);
    //ap_hook_handler(mod_zydeflate_method_handler,NULL,NULL,APR_HOOK_FIRST);
    //ap_hook_handler(mod_zydeflate_method_handler,NULL,NULL,APR_HOOK_MIDDLE);
}

module AP_MODULE_DECLARE_DATA zydeflate_module =
{
	STANDARD20_MODULE_STUFF,
	NULL, // create per-directory configuration structures - we do not.
	NULL, // merge per-directory - no need to merge if we are not creating anything.
	NULL, // create per-server configuration structures.
	NULL, // merge per-server - hrm - examples I have been reading don't bother with this for trivial cases.
	NULL, // configuration directive handlers
	mod_zydeflate_register_hooks, // request handlers
};

static int _etagid_gz(request_rec *r, char *ctype)
{
	int ret;
	char *filename;
	char gz_filename[4096];
	unsigned long fmt, gzmt, emt, gz_size;
	struct stat fst, gzst;


	filename = r->filename;
	snprintf(gz_filename, 400, "%s.zygzip", filename);
	if(stat(filename, &fst) == -1) {
		//fprintf(stderr, "filename is not exist\n");
		return 1;
	}else{
		fmt = fst.st_mtime;
	}
	errno = 0;
	if(stat(gz_filename, &gzst) == -1){
		if(errno != ENOENT){
			fprintf(stderr, "gz_filename is not exist\n");
			return 1;
		}
		gz_size = 0;
		gzmt = 0;
	}else{
		gzmt = gzst.st_mtime;
		gz_size = gzst.st_size;
	}
	if(gzmt == 0 || gzmt < fmt){
		unsigned long uncomprLen = 0;
		unsigned long flen = 0;
		char *compr = NULL;
		FILE *ifp, *ofp;
		ret = 0;
		flen = fst.st_size;
		uncomprLen = flen;
		Bytef *uncompr= (Bytef *)malloc((unsigned int)uncomprLen + 1);
		ifp = fopen(filename, "r");
		fread(uncompr, 1, flen, ifp);
		fclose(ifp);
		ret = zy_gzip(uncompr, uncomprLen, &compr);
		free(uncompr);
		if(ret < 0) {
			fprintf(stderr, "mod_zydeflate error -- gzip : %d : %s \n", ret, zError(ret));
			return(1);
		}
		ofp = fopen(gz_filename, "w");
		if(ofp == NULL){
			fprintf(stderr, "mod_zydeflate cannot open/create file: %s\n", gz_filename);
			free(compr);
			return 1;
		}
		fwrite(compr, 1, ret, ofp);
		fclose(ofp);
		free(compr);
		if(stat(gz_filename, &gzst) == -1){
			fprintf(stderr, "mod_zydeflate cannot stat 2 file: %s\n", gz_filename);
			return 1;
		}else{
			gzmt = gzst.st_mtime;
			gz_size = gzst.st_size;
		}
	}

	const char *etag = apr_table_get(r->headers_in, "If-None-Match");
	//if(etag){ fprintf(stderr, "etag : %s\n", etag); }
	if(etag && strlen(etag) == 10 && (unsigned long)atol(etag) >= gzmt){
		r->status = 304;
		apr_table_setn(r->headers_out, "Etag", etag);
	//	fprintf(stderr, "etag 304\n");
		return 0;
	}
	FILE *ifp;
	ifp = fopen(gz_filename, "r");
	if(ifp == NULL){
		fprintf(stderr, "file : %s read error\n", gz_filename);
		return(1);
	}

	ap_set_content_type(r, ctype);
	apr_table_setn(r->headers_out, "Content-Encoding", "gzip");
	
	char s_length[11];
	snprintf(s_length, 11, "%lu", gz_size);
	//fprintf(stderr, "length: %s, %lu\n", s_length, gz_size);fflush(stderr);
	apr_table_setn(r->headers_out, "Content-Length", s_length);

	char s_etag[11];
	snprintf(s_etag, 11, "%lu", gzmt);
	apr_table_setn(r->headers_out, "Etag", s_etag);

	while((ret = fread(gz_filename, 1, 4000, ifp)) > 0){
		if(ap_rwrite(gz_filename, ret, r) == -1){
			fprintf(stderr, "ap_rwrite error, break\n");fflush(stderr);
			break;
		}
		ap_rflush(r);
	}
	fclose(ifp);
	return(0);
}
/*
#define Z_NO_COMPRESSION         0
#define Z_BEST_SPEED             1
#define Z_BEST_COMPRESSION       9
#define Z_DEFAULT_COMPRESSION  (-1)
*/
#define OS_CODE         0x03
static int zy_gzip(Bytef *data, unsigned long data_len, char **compr)
{
	long level = Z_DEFAULT_COMPRESSION;
	const int gz_magic[2] = {0x1f, 0x8b};
	int status;
	char *s2;
	z_stream stream;

	level = Z_BEST_SPEED;

	stream.zalloc = NULL;
	stream.zfree = NULL;
	stream.opaque = NULL;

	stream.next_in = (Bytef *) data;
	stream.avail_in = data_len;

	stream.avail_out = stream.avail_in + (stream.avail_in / 100) + 15 + 1;
	s2 = (char *) malloc(stream.avail_out + 18);

	/* add gzip file header */
	s2[0] = gz_magic[0];
	s2[1] = gz_magic[1];
	s2[2] = Z_DEFLATED;
	s2[3] = s2[4] = s2[5] = s2[6] = s2[7] = s2[8] = 0; /* time set to 0 */
	s2[9] = OS_CODE;

	stream.next_out = (Bytef *)&(s2[10]);

	/* windowBits is passed < 0 to suppress zlib header & trailer */
	if ((status = deflateInit2(&stream, level, Z_DEFLATED, -MAX_WBITS, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY)) != Z_OK) {
		return(status);
	}
	status = deflate(&stream, Z_FINISH);
	if (status != Z_STREAM_END) {
		deflateEnd(&stream);
		if (status == Z_OK) {
			status = Z_BUF_ERROR;
		}
	} else {
		status = deflateEnd(&stream);
	}

	if (status == Z_OK) {
		/* resize to buffer to the "right" size */
		s2 = realloc(s2, stream.total_out + 18 + 1);

		char *trailer = s2 + (stream.total_out + 10);
		uLong crc = crc32(0L, NULL, 0);

		crc = crc32(crc, (const Bytef *) data, data_len);

		/* write crc & stream.total_in in LSB order */
		trailer[0] = (char) crc & 0xFF;
		trailer[1] = (char) (crc >> 8) & 0xFF;
		trailer[2] = (char) (crc >> 16) & 0xFF;
		trailer[3] = (char) (crc >> 24) & 0xFF;
		trailer[4] = (char) stream.total_in & 0xFF;
		trailer[5] = (char) (stream.total_in >> 8) & 0xFF;
		trailer[6] = (char) (stream.total_in >> 16) & 0xFF;
		trailer[7] = (char) (stream.total_in >> 24) & 0xFF;
		trailer[8] = '\0';
		*compr = s2;
		return stream.total_out + 18;
	} else {
		free(s2);
		return status;
	}
}

