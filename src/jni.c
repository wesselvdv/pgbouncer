#include "bouncer.h"
#include <jni.h>

#define PATH_SEPARATOR ';' /* define it to be ':' on Solaris */
#define USER_CLASSPATH "." /* where Prog.class is */

JavaVM *jvm;

void init(PgSocket *client)
{
	if (jvm != NULL)
	{
		slog_info(client, "JVM already initialized");
		return;
	}

	JNIEnv *env;
	JavaVMInitArgs vm_args;
	JavaVMOption options[1];

	options[0].optionString =
		"-Djava.class.path=" USER_CLASSPATH;
	vm_args.version = JNI_VERSION_10;
	vm_args.options = options;
	vm_args.nOptions = 1;
	vm_args.ignoreUnrecognized = JNI_TRUE;

	free(options);

	if (JNI_CreateJavaVM(&jvm, (void **)&env, &vm_args) < 0)
	{
		slog_error(client, "Can't create Java VM\n");
	}
}
