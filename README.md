# 学生登记表系统 (C++ 版)

## 概述
一个简单的命令行学生信息管理系统，充分使用了 C++ 的核心语法特性：
- **struct** 定义学生数据结构
- **new / delete** 进行动态内存管理
- **指针** 操作动态数组
- **函数** 模块化组织代码
- **文件 I/O** 实现数据持久化

## 功能列表
| 编号 | 功能       | 说明                               |
|------|------------|------------------------------------|
| 1    | 添加学生   | 输入学号、姓名、性别、三门成绩     |
| 2    | 删除学生   | 按学号删除，确认后执行             |
| 3    | 查询学生   | 按学号查找并显示详细信息           |
| 4    | 显示全部   | 以表格形式列出所有学生             |
| 5    | 保存到文件 | 将所有数据写入 `students.dat`      |
| 6    | 退出系统   | 程序结束并释放所有动态内存         |

## 编译与运行

```bash
# 编译
g++ -std=c++11 -o StudentSystem.exe StudentSystem.cpp

# 运行
.\StudentSystem.exe
```

## 核心代码说明

### 1. struct 结构体定义
```cpp
struct Student {
    int    id;           // 学号
    string name;         // 姓名
    char   sex;          // 性别
    double score[3];     // 语文、数学、英语
    double total;        // 总成绩
    double average;      // 平均成绩
};
```

### 2. 动态内存分配 (new / delete)
- 程序启动时用 `new Student[10]` 动态申请数组空间
- 容量不足时通过 `resize()` 重新 `new` 更大的数组
- 程序退出时用 `delete[] g_students` 释放内存
- 加载文件时会 `delete[]` 旧数组再 `new` 新数组

### 3. 指针的使用
- `Student* g_students = nullptr;` — 指向动态数组的指针
- 初始化时赋值 `g_students = new Student[capacity]`
- 扩容时：`delete[]` 旧指针 → `new` 新数组 → 指针重新赋值
- 防止野指针：操作完成后将指针置为 `nullptr`

### 4. 关键函数
| 函数名           | 作用                                       |
|------------------|--------------------------------------------|
| `showMenu()`     | 打印主菜单                                 |
| `addStudent()`   | 添加一名学生（含重复学号检查）             |
| `deleteStudent()`| 按学号删除学生（确认后执行）               |
| `searchStudent()`| 按学号查询并显示学生详细信息               |
| `displayAll()`   | 格式化输出所有学生                         |
| `resize()`       | 动态扩容（核心演示 new/delete 指针操作）   |
| `loadFromFile()` | 从 `students.dat` 加载数据                 |
| `saveToFile()`   | 将数据写入 `students.dat`                  |
| `calcStats()`    | 计算总成绩和平均成绩                       |
| `clearInput()`   | 清除输入缓冲区                             |

## 项目文件
- `StudentSystem.cpp` — 完整源代码
- `students.dat` — 运行时自动生成的数据文件（可由 `students.dat` 持久保存）
