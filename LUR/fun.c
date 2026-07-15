#include "include.h"

// 定义哈希函数
unsigned int hash_func(char *str)
{
    if (str == NULL)
    {
        return 0;
    }
    unsigned int seed = 131;
    unsigned int hash = 0;
    while (*str)
    {
        hash = hash * seed + (*str++);
    }
    return hash % HASH_SIZE;
}

// 创建链表节点
DuLNode *create_list(char *key, int value)
{
    DuLNode *D = (DuLNode *)malloc(sizeof(DuLNode));
    if (NULL == D)
    {
        printf("内存分配失败\n");
        return NULL;
    }
    // 为字符串分配空间
    D->key = (char *)malloc(strlen(key) + 1);
    if (NULL == D->key)
    {
        free(D);
        printf("内存分配失败\n");
        return NULL;
    }
    // 将局部变量key的值拷贝进D->key中
    strcpy(D->key, key);
    D->value = value;
    D->next = NULL;
    D->prior = NULL;
    return D;
}

// 创建哈希表节点
HashNode *create_hash(DuLNode *list_Node)
{
    HashNode *H = (HashNode *)malloc(sizeof(HashNode));
    if (H == NULL)
    {
        printf("内存分配失败\n");
        return NULL;
    }
    H->list_Node = list_Node;
    H->next = NULL;
    return H;
}

// 初始化双向链表
void init_list(LRUcache *cache)
{
    cache->head = create_list("", 0);
    cache->tail = create_list("", 0);
    cache->head->next = cache->tail;
    cache->tail->prior = cache->head;
}

// 将节点插入到链表头部
void add_head(LRUcache *cache, DuLNode *node)
{
    node->prior = cache->head;
    node->next = cache->head->next;
    cache->head->next->prior = node;
    cache->head->next = node;
}

// 将节点删除
void remove_node(LRUcache *cache, DuLNode *node)
{
    node->prior->next = node->next;
    node->next->prior = node->prior;
}

// 查找哈希表
DuLNode *hash_find(LRUcache *cache, char *key)
{
    unsigned int number = hash_func(key);
    HashNode *current = cache->hash_table[number];
    while (current != NULL)
    {
        // 对比键是否相等
        if (0 == strcmp(current->list_Node->key, key))
        {
            return current->list_Node;
        }
        // 指针移动
        current = current->next;
    }
    return NULL;
}

// 插入更新
void hash_insert(LRUcache *cache, char *key, DuLNode *node)
{
    unsigned int number = hash_func(key);
    HashNode *current = cache->hash_table[number];
    // 检查是否已存在
    while (current != NULL)
    {
        if (0 == strcmp(current->list_Node->key, key))
        {
            DuLNode *old_node = current->list_Node;
            current->list_Node = node;
            if (old_node->key)
            {
                free(old_node->key);
                free(old_node);
            }
            return;
        }
        current = current->next;
    }
    // 没找到，创建新节点并成为头结点
    HashNode *new_hash = create_hash(node);
    if (NULL == new_hash)
        return;
    new_hash->next = cache->hash_table[number];
    cache->hash_table[number] = new_hash;
}

// 哈希表删除关键字
void hash_delete(LRUcache *cache, char *key)
{
    unsigned int number = hash_func(key);
    HashNode *current = cache->hash_table[number];
    HashNode *prior = NULL;
    while (current != NULL)
    {
        if (0 == strcmp(current->list_Node->key, key))
        {
            if (NULL == prior)
            {
                // 头结点更换
                cache->hash_table[number] = current->next;
            }
            else
            {
                prior->next = current->next;
            }
            // 释放内存
            free(current);
            return;
        }
        prior = current;
        current = current->next;
    }
}

// 初始化缓存结构体
LRUcache *LRU_create(int maxsize)
{
    LRUcache *cache = (LRUcache *)malloc(sizeof(LRUcache));
    if (NULL == cache)
    {
        printf("缓存结构体内存分配失败\n");
        return NULL;
    }
    cache->maxsize = maxsize;
    cache->size = 0;
    for (int i = 0; i < HASH_SIZE; i++)
    {
        cache->hash_table[i] = NULL;
    }
    init_list(cache);
    return cache;
}

// 获取数据
int lRUcache_Get(LRUcache *cache, char *key)
{
    if (NULL == cache || NULL == key)
    {
        return ERROR;
    }
    DuLNode *node = hash_find(cache, key);
    if (NULL == node)
    {
        return ERROR;
    }
    // 将访问的节点移动到头部
    remove_node(cache, node);
    add_head(cache, node);
    return node->value;
}

// 插入或更新数据
void lRUcache_Put(LRUcache *cache, char *key, int value)
{
    if (cache == NULL || key == NULL)
    {
        return;
    }
    DuLNode *node = hash_find(cache, key);
    if (NULL == node)
    {
        // 创建新节点
        node = create_list(key, value);
        if (NULL == node)
            return;
        // 添加到链表头部和哈希表
        hash_insert(cache, key, node);
        add_head(cache, node);
        cache->size++;
        // 检查容量，删除最久未使用的节点
        if (cache->size > cache->maxsize)
        {
            DuLNode *tail_node = cache->tail->prior;
            // 确保不删除哨兵头节点
            if (tail_node != cache->head)
            {
                remove_node(cache, tail_node);
                hash_delete(cache, tail_node->key);
                free(tail_node->key);
                free(tail_node);
                cache->size--;
            }
        }
    }
    else
    {
        free(node->key);
        node->key = (char *)malloc(strlen(key) + 1);
        if (node->key == NULL)
        {
            return;
        }
        strcpy(node->key, key);
        // 更新值并移动到头部
        node->value = value;
        remove_node(cache, node);
        add_head(cache, node);
    }
}

// 释放空间
void lRUcache_free(LRUcache *cache)
{
    if (NULL == cache)
    {
        return;
    }
    for (int i = 0; i < HASH_SIZE; i++)
    {
        HashNode *h = cache->hash_table[i];
        while (h != NULL)
        {
            HashNode *temp_h = h;
            h = h->next;
            // 释放 HashNode 自己的内存
            free(temp_h);
        }
    }
    DuLNode *current = cache->head;
    DuLNode *temp;
    // 释放双向链表所有节点
    while (current != NULL)
    {
        temp = current;
        current = current->next;
        // 释放key字符串
        if (temp->key)
        {
            free(temp->key);
        }
        // 释放结构体
        free(temp);
    }
    free(cache);
}

// 打印链表顺序
void print_list(LRUcache *cache)
{
    printf("当前链表顺序 (从最新到最旧): ");
    DuLNode *current = cache->head->next;
    int A = 1;
    while (current != cache->tail)
    {
        if (!A)
        {
            printf(" -> ");
        }
        printf("%s(%d)", current->key, current->value);
        current = current->next;
        A = 0;
    }
    if (A)
    {
        printf("空");
    }
    printf("\n");
}