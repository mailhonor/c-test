#include <features.h>
#include <syslog.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <unistd.h>
#include <openssl/md5.h>

#include <security/pam_appl.h>

#define PAM_SM_AUTH
#include <security/pam_modules.h>

#include "crypt.h"

static void _pam_log(int err, const char *format, ...) {
	va_list args;

	va_start(args, format);
	openlog("pam_diremail", LOG_CONS|LOG_PID, LOG_AUTH);
	vsyslog(err, format, args);
	va_end(args);
	closelog();
}
static int converse(pam_handle_t *pamh, int nargs, struct pam_message **message, struct pam_response **response ){
	int retval;
	struct pam_conv *conv;
	retval = pam_get_item(pamh, PAM_CONV,  (const void **) &conv) ;
	if(retval == PAM_SUCCESS){
		retval = conv->conv(nargs, ( const struct pam_message ** ) message, response, conv->appdata_ptr);
	}
	return retval;
}
static int _set_auth_tok(pam_handle_t *pamh, int flags, int argc, const char **argv) {
	int retval;
       	char *p;
       	struct pam_message msg[1],*pmsg[1];
       	struct pam_response *resp;
	pmsg[0] = &msg[0];
	msg[0].msg_style = PAM_PROMPT_ECHO_OFF;
	msg[0].msg = "Password: ";
	resp = NULL; 
	if((retval = converse( pamh, 1 , pmsg, &resp)) != PAM_SUCCESS)
	       	return retval;
	if(resp){
		if((flags & PAM_DISALLOW_NULL_AUTHTOK) && resp[0].resp == NULL){    
			free( resp );
			return PAM_AUTH_ERR;
		}   
		p = resp[ 0 ].resp;
		resp[ 0 ].resp = NULL;
	} else
		return PAM_CONV_ERR;
	free( resp );
	pam_set_item(pamh, PAM_AUTHTOK, p);
	return PAM_SUCCESS;
}
__attribute__((visibility("default")))
PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char **argv) {
	const char *username;
	char *password;
	char *basedir,pwdfn[512], buf[256], *p;
	int retval, passlen;
	int fd;

	(void) pam_fail_delay(pamh, 2000000);
	if(!argc){
		_pam_log(LOG_ERR, "pam conf error need parameter basedir");
		return PAM_AUTH_ERR;
	}
	basedir=argv[0];

	if ((retval = pam_get_user(pamh, &username, NULL)) != PAM_SUCCESS) {
		_pam_log(LOG_ERR, "username not found");
		return retval;
	}
	//_pam_log(LOG_ERR, "username: %s", username);
	
	pam_get_item(pamh, PAM_AUTHTOK, (const void **)&password);
	if(!password && (retval=_set_auth_tok(pamh, flags, argc, argv))!=PAM_SUCCESS){
		return retval;
	}
	pam_get_item(pamh, PAM_AUTHTOK, (const void **)&password);
	if ((retval = pam_get_item(pamh, PAM_AUTHTOK, (const void **)&password)) != PAM_SUCCESS) {
		_pam_log(LOG_ERR, "auth token not found");
		return retval;
	}
	//_pam_log(LOG_ERR, "password: %s", password);
	if (!password) {
		_pam_log(LOG_ERR,"password NULL");
		return PAM_AUTH_ERR;
	}
	passlen=strlen(password);
	if(passlen<2){
		_pam_log(LOG_ERR,"password too short");
		return PAM_AUTH_ERR;
	}
	snprintf(buf, 200, "%s", username);
	if(strchr(buf, '/') || strchr(buf, '\\') || strchr(buf, '|')){
		_pam_log(LOG_ERR,"email include /, \\, or |, critical ERROR");
		return PAM_AUTH_ERR;
	}
	p=strchr(buf, '@');
	if(p){
		*p=0;
		p++;
		snprintf(pwdfn, 512, "%sdomain/%s/user/%s/password", basedir, p, buf);
	}else{
		snprintf(pwdfn, 512, "%smisc/user/%s/password", basedir, buf);
	}
	fd=open(pwdfn, O_RDONLY);
	if(fd==-1){
		_pam_log(LOG_ERR,"can not open: %s", pwdfn);
		return PAM_AUTH_ERR;
	}
	buf[0]=0;
	retval=read(fd, buf, 128);
	if(retval==-1){
		_pam_log(LOG_ERR,"read error: %s", pwdfn);
		return PAM_AUTH_ERR;
	}
	buf[retval]=0;
	close(fd);
	/* here, should add crypt auth */
	if(strcmp(password, buf)){
		_pam_log(LOG_ERR,"password error");
		return PAM_AUTH_ERR;
	}
	return PAM_SUCCESS;
}

__attribute__((visibility("default")))
PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char **argv) {
	return PAM_SUCCESS;
}

__attribute__((visibility("default")))
PAM_EXTERN int pam_sm_acct_mgmt(pam_handle_t *pamh, int flags, int argc, const char **argv) {
	return PAM_SUCCESS;
}
#ifdef PAM_STATIC
struct pam_module _pam_listfile_modstruct = {
	"pam_diremail",
	pam_sm_authenticate,
	pam_sm_setcred,
	pam_sm_acct_mgmt,
	NULL,
	NULL,
	NULL,
};
#endif
