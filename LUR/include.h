#ifndef INCLUDE_H
#define INCLUDE_H
#define ERROR -1
#define HASH_SIZE 1009 // 哈希桶数量
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 定义双向链表
typedef struct DuLNode
{
    char *key;             // 程序名称
    int value;             // 存放的数据
    struct DuLNode *prior; // 前驱
    struct DuLNode *next;  // 后继
} DuLNode;

// 定义哈希表节点
typedef struct HashNode
{
    DuLNode *list_Node;    // 指向链表节点
    struct HashNode *next; // 链地址法解决冲突
} HashNode;

// LRU缓存结构体
typedef struct
{
    HashNode *hash_table[HASH_SIZE]; // 哈希表
    DuLNode *head;                   // 头结点：最近使用的程序
    DuLNode *tail;                   // 尾节点：最久未使用的程序
    int size;                        // 当前个数
    int maxsize;                     // 最大个数
} LRUcache;

// 定义哈希函数
unsigned int hash_func(char *str);

// 创建链表节点
DuLNode *create_list(char *key, int value);

// 创建哈希表节点
HashNode *create_hash(DuLNode *list_Node);

// 初始化双向链表
void init_list(LRUcache *cache);

// 将节点插入到链表头部
void add_head(LRUcache *cache, DuLNode *node);

// 将节点删除
void remove_node(LRUcache *cache, DuLNode *node);

// 查找哈希表
DuLNode *hash_find(LRUcache *cache, char *key);

// 插入更新
void hash_insert(LRUcache *cache, char *key, DuLNode *node);

// 哈希表删除关键字
void hash_delete(LRUcache *cache, char *key);

// 初始化缓存结构体
LRUcache *LRU_create(int maxsize);

// 获取数据
int lRUcache_Get(LRUcache *cache, char *key);

// 插入或更新数据
void lRUcache_Put(LRUcache *cache, char *key, int value);

// 释放空间
void lRUcache_free(LRUcache *cache);

// 打印链表顺序
void print_list(LRUcache *cache);

#endif