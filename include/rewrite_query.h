//
// Created by mehmet on 22.01.2019.
//

#ifndef pg_ddm_RUBY_REWRITE_QUERY_H
#define pg_ddm_RUBY_REWRITE_QUERY_H
bool rewrite_query(PgSocket *client, PktHdr *pkt);


void printHex(void *buffer, const unsigned int n) ;
#endif //pg_ddm_RUBY_REWRITE_QUERY_H
