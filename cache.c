#include "cache.h"

cache_list * init_cache(){
    cache_list* return_p = NULL;
    return_p = (cache_list*)Malloc(sizeof (cache_list));
    return_p->front = NULL;
    return_p->rear = NULL;
    return_p->cache_count = 0;
    return return_p;
}

// 요청 url이 캐시 리스트에 있는지 확인하는 함수
char* find_cache(cache_list* list, char* url){
    // 캐시 리스트에 캐시가 없으면
    if (list->cache_count == 0) {
        return NULL;
    }
    cache_node* currentNode = list->front;
    for (int i = 0; i < list->cache_count; ++i) {
        // 요청 url이 캐시 리스트에 있다면
        if (strcmp(currentNode->url, url) == 0) {
            if (currentNode == list->rear) {
                push(list, pop(list));
            }else if(currentNode != list->front){
                delete(list, currentNode); // 중간 노드의 뒤와 앞을 서로 연결
                push(list, currentNode); // 중간 노드를 앞에 옮긴다.
            }
            return currentNode->data; // 클라이언트에게 보낼 데이터 리턴
        }
        currentNode = currentNode->next; // 다음 캐시로 이동
    }
    return NULL; // 찾은 캐시가 없다면
}

// 캐시 리스트에 캐시를 넣어주는 함수
void insert_cache(cache_list *list, char *url, char* response) {
    cache_node *newNode = NULL;

    newNode = (cache_node *) Malloc(sizeof(cache_node));
    strcpy(newNode->url, url); // url을 캐시에 넣어준다.
    strcpy(newNode->data, response); // 응답 데이터를 캐시에 넣어준다.
    // 캐시가 가득 찬 경우
    if(list->cache_count == MAX_CACHE_COUNT) {
        Free(pop(list)); // 제일 마지막 캐시 삭제후 free
    }
    push(list, newNode);
}

// 제일 마지막 캐시 삭제 함수
cache_node* pop(cache_list *list) {
    cache_node *targetNode = list->rear;
    targetNode->prev->next = NULL;
    list->rear = targetNode->prev;
    list->cache_count--;
    return targetNode;
}


void push(cache_list* list, cache_node* node){
    // 캐시 리스트에 캐시가 없다면
    if (list->cache_count == 0) {
        list->front = node;
        node->prev = NULL;
        list->cache_count++;
        return;
    }
    // 캐시를 제일 앞에 옮긴다.
    list->front->prev = node;
    node->next = list->front;
    node->prev = NULL;
    list->front = node;
    list->cache_count++;
}

// 캐시의 앞과 뒤를 서로 연결하는 함수
void delete(cache_list* list, cache_node* node){
    node->prev->next = node->next;
    node->next->prev = node->prev;
    list->cache_count--;
}

// 캐시 리스트 삭제 함수
void delete_cache_list(cache_list *list){
    if (list->cache_count != 0) {
        cache_node *target = list->front;
        cache_node *temp = NULL;
        while (target->next != NULL) {
            temp = target;
            Free(target);
            target = temp->next;
        }
        Free(list);
    }
}