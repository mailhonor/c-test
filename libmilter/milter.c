/*
 * ================================
 * eli960@qq.com
 * http://linuxmail.cn/
 * 2020-02-25
 * ================================
 */

#include <limits.h>
#include <libmilter/mfapi.h>
#include <arpa/inet.h>
#include <libmilter/mfdef.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>


typedef struct my_milter_context_t my_milter_context_t;
struct my_milter_context_t {
    char *eml_filepath;
    FILE *eml_fp;
};

static char *progname = 0;
static char *listen_address = 0;
static char *tmp_path = 0;

static void show_usage(int ecode)
{
    printf("USAGE: %s listen_address tmp_path_with/\n", progname);
    printf("EXAMPLE: %s inet:1025 /tmp/\n", progname);
    exit(ecode);
}


static void milter_cleanup(SMFICTX *smfi_ctx)
{
    my_milter_context_t *mtx;

    mtx = (my_milter_context_t *) smfi_getpriv(smfi_ctx);
    if (!mtx) {
        return;
    }
    smfi_setpriv(smfi_ctx, NULL);
    if (mtx->eml_filepath) {
        unlink(mtx->eml_filepath);
        free(mtx->eml_filepath);
    }
    if (mtx->eml_fp) {
        fclose(mtx->eml_fp);
    }
    free(mtx);
}

static sfsistat milter_stage_connect(SMFICTX *smfi_ctx, char *hostname, _SOCK_ADDR *hostaddr)
{
    /* 创建,设置,my_milter_context_t */
    my_milter_context_t *mtx = (my_milter_context_t *)calloc(1, sizeof(my_milter_context_t));
    smfi_setpriv(smfi_ctx, mtx);


    /* 获取客户端连接信息, 这个客户端只的是连接smtpd服务的客户端 */
    if (hostaddr && (hostaddr->sa_family == AF_INET)) {
        char client_addr[18];
        inet_ntop(AF_INET, &((struct sockaddr_in *)hostaddr)->sin_addr, client_addr, 16);
        printf("connection from %s\n", client_addr);
    }

    return SMFIS_CONTINUE;
}

static sfsistat milter_stage_helo(SMFICTX *smfi_ctx, char *ehlo)
{
    printf("ehlo: %s\n", ehlo);
    return SMFIS_CONTINUE;
}

static sfsistat milter_stage_envfrom(SMFICTX *smfi_ctx, char **envfrom)
{
    my_milter_context_t *mtx = (my_milter_context_t *)smfi_getpriv(smfi_ctx);

    if (!mtx) {
        return SMFIS_CONTINUE;
    }

    char *from = *envfrom;
    if (!from) {
        from = (char *)(void *)"";
    }
    printf("from: %s\n", from);

    /* smfi_getsymval, 支持 类似 {auth_authen}的参数列表, 见:
     * http://www.postfix.org/MILTER_README.html
     */
    char *auth = 0;
    if ((auth = smfi_getsymval(smfi_ctx, "{auth_authen}")) != NULL) {
        if (*auth) {
            printf("authed, auth name: %s\n", from);
        }
    }

    return SMFIS_CONTINUE;
}

static sfsistat milter_stage_envrcpt(SMFICTX *smfi_ctx, char **envrcpt)
{
    my_milter_context_t *mtx = (my_milter_context_t *) smfi_getpriv(smfi_ctx);
    if (!mtx) {
        return SMFIS_CONTINUE;
    }

    printf("rcpt: %s", *envrcpt);

    return SMFIS_CONTINUE;
}

static sfsistat milter_stage_header(SMFICTX *smfi_ctx, char *headerf, char *headerv)
{
    my_milter_context_t *mtx = (my_milter_context_t *) smfi_getpriv(smfi_ctx);

    if (!mtx) {
        return SMFIS_CONTINUE;
    }
    printf("header: %s: %s\n", headerf, headerv);

    if (mtx->eml_filepath == 0) {
        char path[PATH_MAX+1];
        struct timeval tv;
        gettimeofday(&tv, 0);
        snprintf(path, PATH_MAX, "tmp_path%lx_%lu_%ld.eml", tv.tv_usec, tv.tv_sec, (long)getpid());
        mtx->eml_filepath = strdup(path);
        mtx->eml_fp = fopen(mtx->eml_filepath, "a+");
        if (!mtx->eml_fp) {
            printf("warning can not create file %s(%m)\n", mtx->eml_filepath);
        } else {
            printf("tmp filepath: %s\n", mtx->eml_filepath);
        }
    }

    if (!mtx->eml_fp) {
        return SMFIS_CONTINUE;
    }

    fprintf(mtx->eml_fp, "%s: %s\r\n", headerf, headerv);

    return SMFIS_CONTINUE;
}

static sfsistat milter_stage_eoh(SMFICTX *smfi_ctx)
{
    my_milter_context_t *mtx = (my_milter_context_t *) smfi_getpriv(smfi_ctx);

    if (!mtx) {
        return SMFIS_CONTINUE;
    }

    char *queue_id = 0; /* postfix queue_id */
    if ((queue_id = smfi_getsymval(smfi_ctx, "i")) != NULL) {
        printf("queue_id: %s\n", queue_id);
    }

    if (mtx->eml_fp) {
        fputs("\r\n", mtx->eml_fp);
    }

    return SMFIS_CONTINUE;
}

static sfsistat milter_stage_body(SMFICTX *smfi_ctx, unsigned char *bodyp, size_t bodylen)
{
    my_milter_context_t *mtx = (my_milter_context_t *) smfi_getpriv(smfi_ctx);

    if (!mtx) {
        return SMFIS_CONTINUE;
    }

    if (mtx->eml_fp) {
        fwrite(bodyp, 1, bodylen, mtx->eml_fp);
    }

    return SMFIS_CONTINUE;
}

static sfsistat milter_stage_eom(SMFICTX *smfi_ctx)
{
    my_milter_context_t *mtx = (my_milter_context_t *) smfi_getpriv(smfi_ctx);

    if (!mtx) {
        return SMFIS_CONTINUE;
    }

    if (mtx->eml_fp) {
        fflush(mtx->eml_fp);
        fclose(mtx->eml_fp);
        mtx->eml_fp = 0;
    }

    if (mtx->eml_filepath) {
        /* 这里可以处理信件了 */
    }

    do {
        /* 还有一些其他API */
        
#if 0
        /* 追加/修改邮件头 */
        smfi_addheader(smfi_ctx, "X-abc", "value");
        smfi_chgheader(smfi_ctx, "X-abc", 0, "value"); /* 修改第0个X-abc为value */
#endif
    } while(0);

    return SMFIS_CONTINUE;

}

static sfsistat milter_stage_abort(SMFICTX *smfi_ctx)
{
    milter_cleanup(smfi_ctx);
    return SMFIS_CONTINUE;
}

static sfsistat milter_stage_close(SMFICTX *smfi_ctx)
{
    milter_cleanup(smfi_ctx);
    return SMFIS_CONTINUE;
}

static sfsistat milter_stage_data(SMFICTX *smfi_ctx)
{
    my_milter_context_t *mtx = (my_milter_context_t *) smfi_getpriv(smfi_ctx);
    if (!mtx) {
        return SMFIS_CONTINUE;
    }
    return SMFIS_CONTINUE;
}

int main(int argc, char **argv)
{
    progname = argv[0];
    if (argc != 3) {
        show_usage(1);
    }
    listen_address = argv[1];
    tmp_path = argv[2];
    if (tmp_path[strlen(tmp_path)-1] != '/') {
        show_usage(1);
    }

    signal(SIGPIPE, SIG_IGN);

    struct smfiDesc milter_smfilter = {
        "my_test_milter",               /* filter name */
        SMFI_VERSION,                   /* version code -- do not change */
        SMFIF_ADDHDRS | SMFIF_CHGHDRS,  /* flags */
        milter_stage_connect,           /* connection info filter */
        milter_stage_helo,              /* SMTP HELO command filter */
        milter_stage_envfrom,           /* envelope sender filter */
        milter_stage_envrcpt,           /* envelope recipient filter */
        milter_stage_header,            /* header filter */
        milter_stage_eoh,               /* end of header */
        milter_stage_body,              /* body block filter */
        milter_stage_eom,               /* end of message */
        milter_stage_abort,             /* message aborted */
        milter_stage_close,             /* connection cleanup */
        NULL,                           /* unknown/unimplemented SMTP commands */
        milter_stage_data,              /* DATA command filter */
        NULL                            /* option negotiation at connection startup */
    };

    if (smfi_setconn(listen_address) != MI_SUCCESS) {
        printf("ERR could not set milter socket");
        exit(1);
    }

    if (smfi_register(milter_smfilter) == MI_FAILURE) {
        printf("ERR milter register error");
        exit(1);
    }

    if (smfi_main() != MI_SUCCESS) {
        printf("ERR milter smfi_main error");
    }

    return 0;
}
