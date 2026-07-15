#include "include.h"

// 测试主函数
int main()
{
    char key[50];
    int choice = 0;
    int value = 0;
    LRUcache *cache = NULL;
    // 初始化缓存容量
    printf("请输入缓存的最大容量: ");
    scanf("%d", &value);
    cache = LRU_create(value);
    if (cache == NULL)
    {
        return -1;
    }
    // 进行选择交互
    while (1)
    {
        printf("\n========= LRU 缓存模拟器 =========\n");
        printf("1.========== 存入数据 ============\n");
        printf("2.========== 读取数据 ============\n");
        printf("3.====== 查看当前缓存顺序 ========\n");
        printf("4.========== 退出程序 ============\n");
        printf("======== 请选择操作 (1-4) ========\n");
        scanf("%d", &choice);
        switch (choice)
        {
        case 1:
            printf("请输入名称 (key): ");
            scanf("%s", key);
            printf("请输入值 (value): ");
            scanf("%d", &value);
            lRUcache_Put(cache, key, value);
            printf("操作完成。\n");
            break;
        case 2:
            printf("请输入要查找的程序 (key): ");
            scanf("%s", key);
            value = lRUcache_Get(cache, key);
            if (value == ERROR)
            {
                printf("结果: 错误查找 '%s' 不存在\n", key);
            }
            else
            {
                printf("结果: 找到'%s', 值为 %d\n", key, value);
            }
            break;
        case 3:
            print_list(cache);
            break;
        case 4:
            printf("正在释放内存并退出。\n");
            lRUcache_free(cache);
            return 0;
        default:
            printf("无效输入，请输入 1-4 之间的数字。\n");
        }
        // 显示当前状态
        print_list(cache);
    }
    return 0;
}