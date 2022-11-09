#include "csapp.h"

#ifndef PROXY_WEB_SERVER_CACHE_H
#define PROXY_WEB_SERVER_CACHE_H

#define MAX_CACHE_SIZE      1049000
#define MAX_OBJECT_SIZE     102400
#define MAX_CACHE_COUNT     10

typedef struct cache_node{
    char url[MAX_OBJECT_SIZE]; // 요청 url
    char data[MAX_CACHE_SIZE]; // 클라이언트에게 보낼 데이터
    struct cache_node* prev; // 이전 캐시 노드
    struct cache_node* next; // 다음 캐시 노드
}cache_node;

typedef struct cache_list{
    int cache_count;
    cache_node* front;
    cache_node* rear;
}cache_list;

cache_list * init_cache();
void insert_cache(cache_list *list, char *url, char* response);
void delete_cache(cache_list *list);
char* find_cache(cache_list* list, char* url);
cache_node* pop(cache_list *list);
void push(cache_list* list, cache_node* node);
void delete(cache_list* list, cache_node* node);

#endif //PROXY_WEB_SERVER_CACHE_H