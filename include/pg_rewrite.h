#include <jni.h>

char *rewrite(PgSocket *client, PgUser *user, char *query);
char *stringify(PgSocket *client, jbyteArray byteArray, JNIEnv *env);
jbyteArray parse(PgSocket *client, char *str, JNIEnv *env);
void init(PgSocket *client);
