#include "bouncer.h"
#include <jni.h>
#include <pg_query.h>

#define PATH_SEPARATOR ":"
#define USER_CLASSPATH "/home/pg_ddm/pg_ddm/pg-rewriter/target/scala-3.1.1/PgRewriter-assembly-0.1.0-SNAPSHOT.jar"

JavaVM *jvm;
jclass cls;

char *className = "nl/politie/PgRewriter/Main";
char *staticMethodName = "rewriteUnsafe";
char *staticMethodArgSignature = "([BLjava/lang/String;Ljava/lang/String;)[B";

void init(PgSocket *client)
{
  if (jvm != NULL)
  {
    slog_noise(client, "JVM already initialized");
    return;
  }

  JavaVMInitArgs vm_args;
  JavaVMOption options[2];
  // JavaVMOption options[1];
  JNIEnv *env;
  jmethodID cid;

  options[0].optionString =
      "-Djava.class.path=" USER_CLASSPATH;
  options[1].optionString =
      "-verbose";
  vm_args.version = JNI_VERSION_10;
  vm_args.options = options;
  // vm_args.nOptions = 1;
  vm_args.nOptions = 2;
  vm_args.ignoreUnrecognized = JNI_FALSE;

  if (JNI_CreateJavaVM(&jvm, (void **)&env, &vm_args) < 0)
  {
    slog_error(client, "Can't create Java VM");
  }

  cls = (*env)->FindClass(env, className);

  if (cls != NULL)
  {
    slog_noise(client, "Found class %s", className);
    cid = (*env)->GetStaticMethodID(env, cls, "runUnsafe", "()V");

    if (cid != NULL)
    {
      slog_noise(client, "Found static class method runUnsafe with signature ()V of %s", className);
      (*env)->CallStaticVoidMethod(env, cls, cid);
    }
    else
    {
      slog_noise(client, "Unable to find static class method runUnsafe with signature ()V of %s", className);
    }
  }
}

char *rewrite(PgSocket *client, PgUser *user, char *query)
{
  init(client);

  JNIEnv *env;

  if ((*jvm)->GetEnv(jvm, (void **)&env, JNI_VERSION_10))
  {
    slog_error(client, "Unable to acquire JNIenv");
  }

  jmethodID cid;
  jbyteArray rewrittenQuery;
  jvalue *args = malloc(3 * sizeof(jvalue));
  char *res = NULL;

  /* if cls is NULL, an exception has already been thrown */
  if (cls != NULL)
  {
    cid = (*env)->GetStaticMethodID(env, cls, staticMethodName,
                                    staticMethodArgSignature);

    if (cid != NULL)
    {
      slog_noise(client, "Found static class method %s with signature %s of %s", staticMethodName, staticMethodArgSignature, className);

      args[0].l = parse(client, query, env);
      args[1].l = (*env)->NewStringUTF(env, user->roles);
      args[2].l = (*env)->NewStringUTF(env, user->name);

      slog_noise(client, "Rewriting query => %s", query);
      rewrittenQuery = (jbyteArray)(*env)->CallStaticObjectMethodA(env, cls, cid, args);
      slog_noise(client, "Successfully rewritten query => %s", query);
    }

    if (rewrittenQuery != NULL)
    {
      res = stringify(client, rewrittenQuery, env);
    }
    else
    {
      slog_error(client, "unable to rewrite query");
    }
  }
  else
  {
    slog_error(client, "Unable to find class %s", className);
  }

  return res;
}

jbyteArray parse(PgSocket *client, char *str, JNIEnv *env)
{

  slog_noise(client, "Parsing query => %s", str);
  PgQueryProtobufParseResult result = pg_query_parse_protobuf(str);

  if (result.error)
  {
    slog_error(client, "Can't parse query");
    pg_query_free_protobuf_parse_result(result);

    return NULL;
  }

  jbyteArray res = (*env)->NewByteArray(env, result.parse_tree.len);

  if (res == NULL)
  {
    return NULL; /* OutOfMemoryError already thrown */
  }

  (*env)->SetByteArrayRegion(env, res, 0, result.parse_tree.len, (const jbyte *)result.parse_tree.data);

  if (res == NULL)
  {
    return NULL; /* ArrayIndexOutOfBoundsException already thrown */
  }

  pg_query_free_protobuf_parse_result(result);

  slog_noise(client, "Successfully parsed query => %s", str);

  return res;
}

char *stringify(PgSocket *client, jbyteArray byteArray, JNIEnv *env)
{
  PgQueryProtobuf protobuf;
  jbyte *elements = (*env)->GetByteArrayElements(env, byteArray, NULL);

  if (elements == NULL)
  {
    slog_error(client, "Unable to allocate enough memory for byte array");
    return NULL; /* OutOfMemoryError already thrown */
  }

  jsize num_bytes = (*env)->GetArrayLength(env, byteArray);

  protobuf.data = malloc(num_bytes + 1);

  memcpy(protobuf.data, elements, num_bytes);

  protobuf.data[num_bytes] = 0;
  protobuf.len = num_bytes;

  (*env)->ReleaseByteArrayElements(env, byteArray, elements, 0);

  PgQueryDeparseResult result = pg_query_deparse_protobuf(protobuf);
  free(protobuf.data);

  if (result.error)
  {
    slog_error(client, "Can't stringify query");
    pg_query_free_deparse_result(result);

    return NULL;
  }

  char *res = (char *)malloc(strlen(result.query) + 1);
  strcpy(res, result.query);
  pg_query_free_deparse_result(result);

  return res;
}
