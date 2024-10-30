#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <mysql/mysql.h>

#define HOST    "127.0.0.1"
#define USER    "root"
#define PASSWD  "XMysql12345"
#define DBNAME  "test_db"



// 实现数据库的增删改查操作
// add
void add(MYSQL* mysql)
{
    // 1. 拼装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into student values(%d, %d, '%s')", 1, 18, "zhangsan");
    // 2. 执行sql语句
    int ret = mysql_query(mysql, sql);
    if(ret != 0)
    {
        printf("insert error: %s\n", mysql_error(mysql));
        return;
    }
    printf("insert success\n");
}

// del
void del(MYSQL* mysql)
{
    // 1. 拼装sql语句
    char sql[1024] = {0};
    sprintf(sql, "delete from student where id=%d", 1);
    // 2. 执行sql语句
    int ret = mysql_query(mysql, sql);
    if(ret != 0)
    {
        printf("delete error: %s\n", mysql_error(mysql));
        return;
    }
    printf("delete success\n");
}

// update
void mod(MYSQL* mysql)
{
    // 1. 拼装sql语句
    char sql[1024] = {0};
    sprintf(sql, "update student set name='%s', age=%d where id=%d", "lisi", 20, 1);
    // 2. 执行sql语句
    int ret = mysql_query(mysql, sql);
    if(ret != 0)
    {
        printf("update error: %s\n", mysql_error(mysql));
        return;
    }
    printf("update success\n");
}


// select + print
void select_print(MYSQL* mysql)
{
    // 1. 拼装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select * from student");
    // 2. 执行sql语句
    int ret = mysql_query(mysql, sql);
    if(ret != 0)
    {
        printf("select error: %s\n", mysql_error(mysql));
        return;
    }
    printf("select success\n");

    // 3. 获取结果集
    MYSQL_RES* result = mysql_store_result(mysql);
    if(result == NULL)
    {
        printf("get result error: %s\n", mysql_error(mysql));
        return;
    }
    // 4. 获取结果集中的行数和列数
    int rows = mysql_num_rows(result);
    int fields = mysql_num_fields(result);
    // 5. 获取结果集中的列名
    printf("%-10s\t%-10s\t%-10s\n", "id", "name", "age");
    for(int i = 0; i < rows; ++i)
    {
        MYSQL_ROW row = mysql_fetch_row(result);
        for(int j = 0; j < fields; ++j)
        {
            printf("%-10s\t", row[j]);
        }
        printf("\n");
        
    }
    mysql_free_result(result);
    return;
}


int main()
{
    // 1. 初始化mysql句柄
    MYSQL* mysql = mysql_init(NULL);
    if(mysql == NULL)
    {
        printf("mysql init error\n");
        return -1;
    }
    printf("mysql init success\n");

    // 2. 连接mysql服务器
    if(mysql_real_connect(mysql, HOST, USER, PASSWD, DBNAME, 0, NULL, 0) == NULL)
    {
        printf("connect mysql server error: %s\n", mysql_error(mysql));
        return -1;
    }
    printf("connect mysql server success\n");

    // 3. 设置字符集
    if(mysql_set_character_set(mysql, "utf8") != 0)
    {
        printf("set character error: %s\n", mysql_error(mysql));
        return -1;
    }
    printf("set character success\n");

    // 4. 选择数据库
    if(mysql_select_db(mysql, DBNAME) != 0)
    {
        printf("select db error: %s\n", mysql_error(mysql));
        return -1;
    }
    printf("select db success\n");

    // 5. 执行sql语句
    add(mysql);
    mod(mysql);
    select_print(mysql);
    del(mysql);
    select_print(mysql);
    
    // 6. 关闭mysql句柄
    mysql_close(mysql);
    return 0;
}