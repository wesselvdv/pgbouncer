#include <ruby.h>
#include "bouncer.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

int loader = 0;
bool loaded = false;
bool error = false;
time_t load_time;
char* script = "/home/pg_ddm/pg_ddm/mask_ruby/parser.rb";

struct query_data {
	VALUE query_str;
	VALUE username;
	VALUE db;
	VALUE etcd_host;
	VALUE etcd_port;
	VALUE etcd_user;
	VALUE etcd_passwd;
	VALUE user_regex;
	VALUE tag_regex;
	VALUE tag_users;
	VALUE default_scheme;
};

VALUE* embeded(VALUE q) {
	VALUE result;
	VALUE parser;

	struct query_data *data = (struct query_data*) q;
	struct query_return qr;

	parser = rb_const_get(rb_cObject, rb_intern("PgQueryOpt"));

	VALUE new_cls = rb_funcall(parser, rb_intern("new"), 0);
	result = rb_funcall(new_cls, rb_intern("properties"), 11, data->query_str, data->username,
			data->db, data->etcd_host, data->etcd_port, data->etcd_user,
			data->etcd_passwd, data->user_regex, data->tag_regex,
			data->default_scheme, data->tag_users);

	return result;

}

VALUE* embeded_role(VALUE q) {
	VALUE result;
	VALUE parser;

	struct query_data *data = (struct query_data*) q;

	parser = rb_const_get(rb_cObject, rb_intern("PgQueryOpt"));

	VALUE new_cls = rb_funcall(parser, rb_intern("new"), 0);
	result = rb_funcall(new_cls, rb_intern("get_role"), 1, data->query_str);

	return result;

}

struct query_return rubycall_role(PgSocket *client, char *username,
		char *query_str) {

	VALUE *res;
	int state;
	struct query_return qr;
	qr.query = NULL;
	qr.role = "master";

	if (loader == 0) {
		RUBY_INIT_STACK;

		ruby_init();
		ruby_init_loadpath();
		ruby_script("RewriteQuery");
		rb_define_module("Gem");
		rb_require("rubygems");
		rb_eval_string_protect("require 'pg_ddm_sql_modifier'", &state);
		loader = 1;
	}

	struct query_data q;
	q.query_str = rb_str_new_cstr(query_str);

	res = rb_protect(embeded_role, (VALUE) (&q), &state);

	if (state) {
		slog_error(client, "Error when executed ruby code");
		ruby_cleanup(state);
		return qr;
	} else {
		if (TYPE(res) == T_STRING) {
			if (TYPE(res) == T_STRING) {
				qr.role = StringValueCStr(res);
			}
			return qr;
		}
	}

}

void ruby_error(PgSocket *client)
{
    error = true;
	/* print exception */
	VALUE exception = rb_errinfo();
	rb_set_errinfo(Qnil);

	if (RTEST(exception)) {
	    VALUE msg = rb_funcall(exception, rb_intern("full_message"), 0);

	    slog_error(client, "%s", RSTRING_PTR(msg));
	}
}

void load_ruby(PgSocket *client)
{
	/* get script modification time */
	struct stat script_stat;

	if (stat(script, &script_stat))
	{
        slog_debug(client, "failed to stat");
		if (loaded)
			slog_error(client, "Can't stat script\n");
		loaded = false;

		ruby_error(client);

		return;
	}

    slog_debug(client, "managed to stat");
    slog_debug(client, "loaded %s", loaded ? "true" : "false");
    slog_debug(client, "load_time %i", load_time);
    slog_debug(client, "script_stat.st_mtime %i", script_stat.st_mtime);

	/* nothing to do if we've already loaded the script and it hasn't been updated */
	if (loaded && load_time == script_stat.st_mtime) return;

	if (loaded)
		slog_debug(client, "Reloading ...\n");
	else
		slog_debug(client, "Loading ...\n");

	loaded = true;
	load_time = script_stat.st_mtime;

    error = false;

	int state;
	rb_load_protect(rb_str_new_cstr(script), 0, &state);

	if (state) ruby_error(client);
}

struct query_return rubycall(PgSocket *client, char *username, char *query_str) {

	VALUE *res;
	int state;
	struct query_return qr;

	qr.query = NULL;
	qr.role = "master";

	if (loader == 0) {
		RUBY_INIT_STACK;

		ruby_init();
		ruby_init_loadpath();
		ruby_script("RewriteQuery");
		rb_define_module("Gem");
		rb_require("rubygems");

		loader = 1;
	}

	load_ruby(client);

	struct query_data q;
	q.query_str = rb_str_new_cstr(query_str);
	q.username = rb_str_new_cstr(username);
	q.db = rb_str_new_cstr(client->db->name);
	q.etcd_host = rb_str_new_cstr(cf_etcd_host);
	q.etcd_port = rb_str_new_cstr(cf_etcd_port);
	q.etcd_user = rb_str_new_cstr(cf_etcd_user);
	q.etcd_passwd = rb_str_new_cstr(cf_etcd_passwd);
	q.user_regex = rb_str_new_cstr(cf_user_regex);
	q.tag_regex = rb_str_new_cstr(cf_tag_regex);
	q.tag_users = rb_str_new_cstr(cf_tag_users);
	q.default_scheme = rb_str_new_cstr(client->pool->db->search_path);


	res = rb_protect(embeded, (VALUE) (&q), &state);

	if (state) {
	    ruby_error(client);
		slog_error(client, "Error when executed ruby code 1: %s", query_str);
//		ruby_cleanup(state);
		return qr;
	} else {
		if (TYPE(res) == T_STRING) {
			qr.query = StringValueCStr(res);
			if(cf_pg_ddm_rewrite_route){
                res = rb_protect(embeded_role, (VALUE) (&q), &state);
                if (state) {
                    slog_error(client, "Error when executed ruby code 2: %s", query_str);
//                    ruby_cleanup(state);
                    return qr;
                } else {
                    if (TYPE(res) == T_STRING) {
                        qr.role = StringValueCStr(res);
                    }
                }
			}
		}
		return qr;
	}

}
