/* set user and group id and exec */

#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "static_assert.h"

STATIC_ASSERT( sizeof(uid_t) <= 4, "The size of uid_t is too big, the source must be updated" );
#define UID_T_STRING_SIZE 11 /* Worst case: "4294967295\0" */
#define UID_T_FMT "%lu"
#define UID_T_FMT_CAST (unsigned long)

static char *argv0;

static void usage(int exitcode)
{
	printf("Usage: %s user-spec command [args]\n", argv0);
	printf("Usage: %s -l\n\tShows license.\n", argv0);
	exit(exitcode);
}

#include "license.inc"
static void print_license()
{
	unsigned int i;
	for (i=0; i<LICENSE_len; i++)
		putchar(LICENSE[i]);
}

#ifdef linux
#define WARNING_setgroups " (on Linux you need CAP_SETGID)"
#define WARNING_setgid " (on Linux you need CAP_SETGID)"
#define WARNING_setuid " (on Linux you need CAP_SETUID)"
#else
#define WARNING_setgroups ""
#define WARNING_setgid ""
#define WARNING_setuid ""
#endif

static void print_eperm_warning(const char *extra_warn)
{
	int saved_errno;
	if (errno != EPERM)
		return;
	saved_errno = errno;
	fprintf(stderr, "Insufficient privilege, %s needs to run as root%s.\n", argv0, extra_warn);
	errno = saved_errno;
}

int main(int argc, char *argv[])
{
	char *user, *group, **cmdargv;
	char *end;
	char *env;

	uid_t uid = getuid();
	gid_t gid = getgid();

	argv0 = argv[0];
	if (argc == 2 && strcmp(argv[1], "-l") == 0) {
		print_license();
		return EXIT_SUCCESS;
	}
	if (argc < 3)
		usage(EXIT_FAILURE);

	user = argv[1];
	group = strchr(user, ':');
	if (group)
		*group++ = '\0';

	/* Check for env flag */
	if (strcmp(user, "-e") == 0 || strcmp(user, "--env") == 0) {
		/* Clear existing value */
		user = NULL;

		env = getenv("SUID");
		if (env != NULL)
			user = env;

		env = getenv("SGID");
		if (env != NULL)
			group = env;
		
		if (!user && !group) {
			err(1, "SUID and SGID environment variables unset");
		}
	}

	cmdargv = &argv[2];

	struct passwd *pw = NULL;
	if (user[0] != '\0') {
		uid_t nuid = strtol(user, &end, 10);
		if (*end == '\0')
			uid = nuid;
		else {
			pw = getpwnam(user);
			if (pw == NULL)
				err(1, "getpwnam(%s)", user);
		}
	}
	if (pw == NULL) {
		pw = getpwuid(uid);
	}
	if (pw != NULL) {
		uid = pw->pw_uid;
		gid = pw->pw_gid;
	}

	if (pw != NULL) {
		setenv("HOME", pw->pw_dir, 1);
		setenv("USER", pw->pw_name, 1);
		setenv("LOGNAME", pw->pw_name, 1);
	} else {
		char tmp[UID_T_STRING_SIZE];
		sprintf(tmp, UID_T_FMT, UID_T_FMT_CAST uid);
		setenv("HOME", "/", 1);
		setenv("USER", tmp, 1);
		setenv("LOGNAME", tmp, 1);
	}

	if (group && group[0] != '\0') {
		/* group was specified, ignore grouplist for setgroups later */
		pw = NULL;

		gid_t ngid = strtol(group, &end, 10);
		if (*end == '\0')
			gid = ngid;
		else {
			struct group *gr = getgrnam(group);
			if (gr == NULL)
				err(1, "getgrnam(%s)", group);
			gid = gr->gr_gid;
		}
	}

	if (pw == NULL) {
		if (setgroups(1, &gid) < 0) {
			print_eperm_warning(WARNING_setgroups);
			err(EXIT_FAILURE, "setgroups(%i)", gid);
		}
	} else {
		int ngroups = 0;
		gid_t *glist = NULL;

		while (1) {
			int r = getgrouplist(pw->pw_name, gid, glist, &ngroups);

			if (r >= 0) {
				if (setgroups(ngroups, glist) < 0) {
					print_eperm_warning(WARNING_setgroups);
					err(EXIT_FAILURE, "setgroups");
				}
				break;
			}

			glist = realloc(glist, ngroups * sizeof(gid_t));
			if (glist == NULL)
				err(EXIT_FAILURE, "malloc");
		}
	}

	if (setgid(gid) < 0) {
		print_eperm_warning(WARNING_setgid);
		err(EXIT_FAILURE, "setgid(%i)", gid);
	}

	if (setuid(uid) < 0) {
		print_eperm_warning(WARNING_setuid);
		err(EXIT_FAILURE, "setuid(%i)", uid);
	}

	execvp(cmdargv[0], cmdargv);
	err(EXIT_FAILURE, "%s", cmdargv[0]);

	return EXIT_FAILURE;
}
