#include "bouncer.h"

#ifdef HAVE_PAM

#include <pwd.h>
#include <grp.h>
#include "mgetgroups.h"

char *nsgetgroups(char username[MAX_USERNAME])
{
	struct passwd *pwd = NULL;
	char *roles = NULL;
	gid_t *groups;
	int n_groups;

	log_debug("pam_auth_worker(): getpwnam for %s", username);
	pwd = getpwnam(username);

	if (pwd == NULL)
	{
		log_error("failed to get groups for user %s",
				  username);
		return NULL;
	}

	log_debug("pam_auth_worker(): xgetgroups for user %s, gid: %d", pwd->pw_name, pwd->pw_gid);
	n_groups = xgetgroups(pwd->pw_name, pwd->pw_gid, &groups);

	if (n_groups < 0)
	{
		if (pwd->pw_name)
		{
			log_error("failed to get groups for user %s",
					  pwd->pw_name);
		}
		else
		{
			log_error("failed to get groups for the current process");
		}
		return NULL;
	}

	for (int i = 0; i < n_groups; i++)
	{
		struct group *grp = NULL;
		char *group;

		log_debug("pam_auth_worker(): getgrgid for %d", groups[i]);
		grp = getgrgid(groups[i]);
		log_debug("pam_auth_worker(): found name %s for %d", grp->gr_name, groups[i]);
		if (grp == NULL)
		{
			log_error("cannot find name for group ID %lu",
					  (unsigned long int)groups[i]);

			return NULL;
		}

		if (asprintf(&roles, "%s%s%s", roles == NULL ? "" : roles, roles != NULL ? "," : "", grp->gr_name) < 0)
		{
			log_error("failed to allocate roles to pgUser struct for user %s",
					  username);
			return NULL;
		}
	}
	log_debug("pam_auth_worker(): found groups %s for user %s", roles, username);

	return roles;
}

#endif
