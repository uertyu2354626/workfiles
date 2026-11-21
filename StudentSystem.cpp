// ============================================================
// 学生登记表系统 v1.0
// 知识点：struct、new/delete、指针、函数、动态数组
// ============================================================

#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>
#include <fstream>
#include <limits>

using namespace std;

// ---------- 数据结构定义 ----------

struct Student {
    int    id;           // 学号
    string name;         // 姓名
    char   sex;          // 性别：'M'男 'F'女
    double score[3];     // 三门成绩：语文、数学、英语
    double total;        // 总成绩
    double average;      // 平均成绩
};

// ---------- 全局变量 ----------
Student* g_students   = nullptr;   // 动态数组
int      g_count      = 0;         // 当前人数
int      g_capacity   = 0;         // 当前容量

// ---------- 函数声明 ----------
void showMenu();
void addStudent();
void deleteStudent();
void searchStudent();
void displayAll();
void loadFromFile(const char* filename);
void saveToFile(const char* filename);
void resize(int newCap);
void calcStats(Student& s);
void clearInput();
bool getConfirmation(const char* msg);

// ============================================================
// 主函数
// ============================================================
int main() {
    // 初始化容量为 10
    g_capacity = 10;
    g_students = new Student[g_capacity];
    g_count    = 0;

    // 尝试从文件加载数据
    loadFromFile("students.dat");

    int choice = 0;
    do {
        showMenu();
        cout << "请输入选择: ";
        cin >> choice;
        clearInput();

        switch (choice) {
            case 1: addStudent();         break;
            case 2: deleteStudent();      break;
            case 3: searchStudent();      break;
            case 4: displayAll();         break;
            case 5: saveToFile("students.dat"); break;
            case 6: cout << "\n再见!\n"; break;
            default: cout << "无效选择，请重试。\n"; break;
        }
    } while (choice != 6);

    // 释放内存
    delete[] g_students;
    g_students = nullptr;

    return 0;
}

// ============================================================
// 显示菜单
// ============================================================
void showMenu() {
    cout << "\n\n========== 学生登记表系统 ==========\n";
    cout << "  1. 添加学生信息\n";
    cout << "  2. 删除学生信息\n";
    cout << "  3. 查询学生信息\n";
    cout << "  4. 显示所有学生\n";
    cout << "  5. 保存到文件\n";
    cout << "  6. 退出系统\n";
    cout << "====================================\n";
}

// ============================================================
// 清除输入缓冲区
// ============================================================
void clearInput() {
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

// ============================================================
// 动态扩容（确保有足够空间）
// ============================================================
void resize(int newCap) {
    if (newCap <= g_capacity) return;

    // 用 new 申请更大空间
    Student* newArr = new Student[newCap];

    // 复制原有数据
    for (int i = 0; i < g_count; ++i) {
        newArr[i] = g_students[i];
    }

    // 用 delete 释放旧空间
    delete[] g_students;
    g_students = newArr;
    g_capacity = newCap;
}

// ============================================================
// 计算某学生的总成绩与平均成绩
// ============================================================
void calcStats(Student& s) {
    s.total = s.score[0] + s.score[1] + s.score[2];
    s.average = s.total / 3.0;
}

// ============================================================
// 1. 添加学生
// ============================================================
void addStudent() {
    if (g_count >= g_capacity) {
        resize(g_capacity + 10);  // 每次增加 10 个容量
    }

    Student& s = g_students[g_count];

    // 学号：检查是否存在
    int id;
    bool found;
    do {
        cout << "请输入学号: ";
        cin >> id;
        clearInput();
        found = false;
        for (int i = 0; i < g_count; ++i) {
            if (g_students[i].id == id) {
                found = true;
                break;
            }
        }
        if (found) cout << "该学号已存在！\n";
    } while (found);

    s.id = id;

    // 姓名
    cout << "请输入姓名: ";
    getline(cin, s.name);

    // 性别
    char sex;
    do {
        cout << "请输入性别 (M/F): ";
        cin >> sex;
        clearInput();
        sex = toupper(sex);
        if (sex != 'M' && sex != 'F') cout << "输入错误，请重新输入。\n";
    } while (sex != 'M' && sex != 'F');
    s.sex = sex;

    // 三门成绩
    for (int i = 0; i < 3; ++i) {
        double sc;
        do {
            const char* names[] = {"语文", "数学", "英语"};
            cout << "请输入" << names[i] << "成绩 (0-100): ";
            cin >> sc;
            clearInput();
            if (sc < 0 || sc > 100) cout << "成绩必须在 0-100 之间。\n";
        } while (sc < 0 || sc > 100);
        s.score[i] = sc;
    }

    calcStats(s);
    ++g_count;
    cout << "学生 [" << s.name << "] 添加成功！\n";
}

// ============================================================
// 2. 删除学生
// ============================================================
void deleteStudent() {
    if (g_count == 0) {
        cout << "暂无学生信息。\n";
        return;
    }

    int id;
    cout << "请输入要删除的学生学号: ";
    cin >> id;
    clearInput();

    int idx = -1;
    for (int i = 0; i < g_count; ++i) {
        if (g_students[i].id == id) {
            idx = i;
            break;
        }
    }

    if (idx == -1) {
        cout << "未找到学号为 " << id << " 的学生。\n";
        return;
    }

    string message = "确定删除学生 " + g_students[idx].name + "？";
    if (!getConfirmation(message.c_str())) {
        cout << "已取消删除。\n";
        return;
    }

    // 删除：将最后一条移到当前位置，覆盖被删元素（无需 new/delete 单个对象）
    g_students[idx] = g_students[g_count - 1];
    --g_count;
    cout << "学生删除成功。\n";
}

// ============================================================
// 3. 查询学生
// ============================================================
void searchStudent() {
    if (g_count == 0) {
        cout << "暂无学生信息。\n";
        return;
    }

    int id;
    cout << "请输入要查询的学生学号: ";
    cin >> id;
    clearInput();

    for (int i = 0; i < g_count; ++i) {
        if (g_students[i].id == id) {
            const Student& s = g_students[i];
            cout << "\n--- 查询结果 ---\n";
            cout << "  学号: "   << s.id    << "\n";
            cout << "  姓名: "   << s.name << "\n";
            cout << "  性别: "   << (s.sex == 'M' ? "男" : "女") << "\n";
            cout << "  语文: "   << fixed << setprecision(1) << s.score[0] << "\n";
            cout << "  数学: "   << fixed << setprecision(1) << s.score[1] << "\n";
            cout << "  英语: "   << fixed << setprecision(1) << s.score[2] << "\n";
            cout << "  总成绩: " << fixed << setprecision(1) << s.total << "\n";
            cout << "  平均成绩: " << fixed << setprecision(1) << s.average << "\n";
            return;
        }
    }
    cout << "未找到学号为 " << id << " 的学生。\n";
}

// ============================================================
// 4. 显示所有学生
// ============================================================
void displayAll() {
    if (g_count == 0) {
        cout << "暂无学生信息。\n";
        return;
    }

    cout << "\n";
    cout << left << setw(8) << "学号"
         << setw(12) << "姓名"
         << setw(6) << "性别"
         << setw(8) << "语文"
         << setw(8) << "数学"
         << setw(8) << "英语"
         << setw(10) << "总成绩"
         << setw(10) << "平均成绩"
         << endl;
    cout << string(70, '-') << endl;

    for (int i = 0; i < g_count; ++i) {
        const Student& s = g_students[i];
        cout << left << setw(8) << s.id
             << setw(12) << s.name
             << setw(6) << (s.sex == 'M' ? "男" : "女")
             << setw(8) << fixed << setprecision(1) << s.score[0]
             << setw(8) << s.score[1]
             << setw(8) << s.score[2]
             << setw(10) << s.total
             << setw(10) << s.average
             << endl;
    }
    cout << "共 " << g_count << " 名学生。\n";
}

// ============================================================
// 5. 从文件加载数据
// ============================================================
void loadFromFile(const char* filename) {
    ifstream fin(filename);
    if (!fin.is_open()) {
        cout << "未找到存档文件，将从空白开始。\n";
        return;
    }

    // 先清空现有数据
    g_count = 0;
    g_capacity = 0;
    delete[] g_students;
    g_students = nullptr;

    int cap = 10;
    g_students = new Student[cap];
    g_capacity = cap;

    int id;
    string name;
    char sex;
    double s0, s1, s2;

    while (fin >> id >> name >> sex >> s0 >> s1 >> s2) {
        if (g_count >= g_capacity) {
            resize(g_capacity + 10);
        }
        g_students[g_count].id     = id;
        g_students[g_count].name   = name;
        g_students[g_count].sex    = sex;
        g_students[g_count].score[0] = s0;
        g_students[g_count].score[1] = s1;
        g_students[g_count].score[2] = s2;
        calcStats(g_students[g_count]);
        ++g_count;
    }
    fin.close();
    cout << "已从文件加载 " << g_count << " 条记录。\n";
}

// ============================================================
// 6. 保存数据到文件
// ============================================================
void saveToFile(const char* filename) {
    ofstream fout(filename);
    if (!fout.is_open()) {
        cout << "保存失败！\n";
        return;
    }
    for (int i = 0; i < g_count; ++i) {
        const Student& s = g_students[i];
        fout << s.id << " "
             << s.name << " "
             << s.sex << " "
             << s.score[0] << " "
             << s.score[1] << " "
             << s.score[2] << "\n";
    }
    fout.close();
    cout << "数据已保存到文件，共 " << g_count << " 条记录。\n";
}

// ============================================================
// 工具函数：获取用户确认
// ============================================================
bool getConfirmation(const char* msg) {
    cout << msg << " (y/n): ";
    char ch;
    cin >> ch;
    clearInput();
    return (ch == 'y' || ch == 'Y');
}
