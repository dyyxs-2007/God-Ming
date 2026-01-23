#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>
#include <conio.h>
#include <io.h>
#include <fcntl.h>

#define MAX_INPUT 100
#define MAX_DATA 20
#define MAX_NAME 50
#define MAX_COURSE 20
#define MAX_SCORE 100

// 用户角色定义
typedef enum {
    ROLE_STUDENT = 1,
    ROLE_TEACHER = 2,
    ROLE_ADMIN = 3
} UserRole;

// 基础用户结构
typedef struct {
    char id[MAX_DATA];
    char pwd[MAX_DATA];
    char name[MAX_NAME];
    UserRole role;
} User;

// 成绩记录结构
typedef struct {
    char courseName[MAX_NAME];
    double score;
    char examDate[20];
} ScoreRecord;

// 学生结构（继承User）
typedef struct {
    User base;
    char className[MAX_NAME];
    ScoreRecord scores[MAX_COURSE];
    int scoreCount;
    double totalScore;
    double averageScore;
    int rank;
} Student;

// 班级结构
typedef struct {
    char className[MAX_NAME];
    char teacherId[MAX_DATA];
    char studentIds[MAX_COURSE][MAX_DATA];
    int studentCount;
} ClassInfo;

// 教师结构（继承User）
typedef struct {
    User base;
    char classNames[MAX_COURSE][MAX_NAME];
    int classCount;
    char courses[MAX_COURSE][MAX_NAME];
    int courseCount;
} Teacher;

// 管理员结构（继承User）
typedef struct {
    User base;
    char department[MAX_NAME];
} Admin;

// 全局变量
User currentUser = {"", "", "", 0}; // 当前登录用户
void* currentUserData = NULL;       // 当前登录用户的完整数据
Student* students = NULL;
Teacher* teachers = NULL;
Admin* admins = NULL;
ClassInfo* classes = NULL;
int studentCount = 0;
int teacherCount = 0;
int adminCount = 0;
int classCount = 0;

// 工具函数
void clean() {
    system("cls");
}

void wait() {
    system("pause");
}

// 安全输入函数
void safe_input(char* buffer, int max_length) {
    if (buffer == NULL || max_length <= 1) {
        return;
    }
    if (fgets(buffer, max_length, stdin) != NULL) {
        buffer[strcspn(buffer, "\n")] = 0;
        if (strlen(buffer) == max_length - 1 && buffer[max_length - 2] != '\n') {
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
        }
    } else {
        buffer[0] = '\0';
    }
}

// 密码输入函数（Windows版本）
void safe_input_pwd(char* buffer, int max_len) {
    int i = 0;
    char ch;
    while (i < max_len - 1) {
        ch = getch();
        if (ch == '\r') {
            break;
        }
        if (ch == '\b') {
            if (i > 0) {
                i--;
                printf("\b \b");
            }
            continue;
        }
        buffer[i++] = ch;
        printf("*");
    }
    buffer[i] = '\0';
    printf("\n");
}

// 安全整数输入
int safe_int_input() {
    char buffer[MAX_INPUT];
    int value;
    char extra_char;
    while (1) {
        safe_input(buffer, sizeof(buffer));
        if (sscanf(buffer, "%d%c", &value, &extra_char) == 1) {
            return value;
        }
        printf("输入无效，请重新输入：");
    }
}

// 安全浮点数输入
double safe_double_input() {
    char buffer[MAX_INPUT];
    double value;
    char extra_char;
    while (1) {
        safe_input(buffer, sizeof(buffer));
        if (sscanf(buffer, "%lf%c", &value, &extra_char) == 1) {
            return value;
        }
        printf("输入无效，重新输入：");
    }
}

// 通用文件检查函数
int checkUserId(const char* id, UserRole role) {
    char filename[20];
    switch (role) {
        case ROLE_STUDENT: strcpy(filename, "students.bin"); break;
        case ROLE_TEACHER: strcpy(filename, "teachers.bin"); break;
        case ROLE_ADMIN: strcpy(filename, "admins.bin"); break;
        default: return 0;
    }

    FILE* fp = fopen(filename, "rb");
    if (fp == NULL) {
        return 0;
    }

    User user;
    size_t size = (role == ROLE_STUDENT) ? sizeof(Student) : 
                  (role == ROLE_TEACHER) ? sizeof(Teacher) : sizeof(Admin);

    while (fread(&user, sizeof(User), 1, fp) == 1) {
        if (strcmp(user.id, id) == 0) {
            fclose(fp);
            return 1;
        }
        // 跳过额外数据
        fseek(fp, size - sizeof(User), SEEK_CUR);
    }

    fclose(fp);
    return 0;
}

// 通用密码更新函数
void updateUserPwd(const char* id, const char* new_pwd, UserRole role) {
    char filename[20];
    switch (role) {
        case ROLE_STUDENT: strcpy(filename, "students.bin"); break;
        case ROLE_TEACHER: strcpy(filename, "teachers.bin"); break;
        case ROLE_ADMIN: strcpy(filename, "admins.bin"); break;
        default: return;
    }

    FILE* fp = fopen(filename, "rb+");
    if (fp == NULL) {
        printf("出现错误\n");
        return;
    }

    User user;
    size_t size = (role == ROLE_STUDENT) ? sizeof(Student) : 
                  (role == ROLE_TEACHER) ? sizeof(Teacher) : sizeof(Admin);

    while (fread(&user, sizeof(User), 1, fp) == 1) {
        if (strcmp(user.id, id) == 0) {
            strncpy(user.pwd, new_pwd, sizeof(user.pwd) - 1);
            fseek(fp, -sizeof(User), SEEK_CUR);
            fwrite(&user, sizeof(User), 1, fp);
            printf("密码更新成功\n");
            
            // 如果是当前用户，同时更新内存中的密码
            if (strcmp(currentUser.id, id) == 0 && currentUser.role == role) {
                strcpy(currentUser.pwd, new_pwd);
            }
            
            fclose(fp);
            return;
        }
        // 跳过额外数据
        fseek(fp, size - sizeof(User), SEEK_CUR);
    }

    fclose(fp);
    printf("更新失败！\n");
}

// 通用用户添加函数
int addUser(const char* id, const char* pwd, const char* name, UserRole role, void* extra_data) {
    char filename[20];
    switch (role) {
        case ROLE_STUDENT: strcpy(filename, "students.bin"); break;
        case ROLE_TEACHER: strcpy(filename, "teachers.bin"); break;
        case ROLE_ADMIN: strcpy(filename, "admins.bin"); break;
        default: return 0;
    }

    // 检查ID是否已存在
    if (checkUserId(id, role)) {
        printf("ID已存在！\n");
        return 0;
    }

    FILE* fp = fopen(filename, "ab");
    if (fp == NULL) {
        printf("出现错误！\n");
        return 0;
    }

    size_t size;
    switch (role) {
        case ROLE_STUDENT: {
            Student student;
            strncpy(student.base.id, id, sizeof(student.base.id) - 1);
            strncpy(student.base.pwd, pwd, sizeof(student.base.pwd) - 1);
            strncpy(student.base.name, name, sizeof(student.base.name) - 1);
            student.base.role = role;
            student.scoreCount = 0;
            student.totalScore = 0.0;
            student.averageScore = 0.0;
            student.rank = 0;
            memset(student.scores, 0, sizeof(student.scores));
            if (extra_data) memcpy(&student, extra_data, sizeof(Student));
            size = fwrite(&student, sizeof(Student), 1, fp);
            break;
        }
        case ROLE_TEACHER: {
            Teacher teacher;
            strncpy(teacher.base.id, id, sizeof(teacher.base.id) - 1);
            strncpy(teacher.base.pwd, pwd, sizeof(teacher.base.pwd) - 1);
            strncpy(teacher.base.name, name, sizeof(teacher.base.name) - 1);
            teacher.base.role = role;
            teacher.classCount = 0;
            teacher.courseCount = 0;
            memset(teacher.classNames, 0, sizeof(teacher.classNames));
            memset(teacher.courses, 0, sizeof(teacher.courses));
            if (extra_data) memcpy(&teacher, extra_data, sizeof(Teacher));
            size = fwrite(&teacher, sizeof(Teacher), 1, fp);
            break;
        }
        case ROLE_ADMIN: {
            Admin admin;
            strncpy(admin.base.id, id, sizeof(admin.base.id) - 1);
            strncpy(admin.base.pwd, pwd, sizeof(admin.base.pwd) - 1);
            strncpy(admin.base.name, name, sizeof(admin.base.name) - 1);
            admin.base.role = role;
            strcpy(admin.department, "教务处");
            if (extra_data) memcpy(&admin, extra_data, sizeof(Admin));
            size = fwrite(&admin, sizeof(Admin), 1, fp);
            break;
        }
    }

    fclose(fp);
    if (size == 1) {
        printf("添加成功！\n");
        return 1;
    } else {
        printf("信息录入失败！\n");
        return 0;
    }
}

// 通用登录函数 - 修复：保存完整的用户数据
int loginUser(char* id, char* pwd, UserRole role) {
    char filename[20];
    switch (role) {
        case ROLE_STUDENT: strcpy(filename, "students.bin"); break;
        case ROLE_TEACHER: strcpy(filename, "teachers.bin"); break;
        case ROLE_ADMIN: strcpy(filename, "admins.bin"); break;
        default: return 0;
    }

    FILE* fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("系统未录入该类型用户，请联系管理员\n");
        return 0;
    }

    User user;
    size_t size = (role == ROLE_STUDENT) ? sizeof(Student) : 
                  (role == ROLE_TEACHER) ? sizeof(Teacher) : sizeof(Admin);

    int found = 0;
    long filePos = 0;
    
    while (1) {
        filePos = ftell(fp);
        if (fread(&user, sizeof(User), 1, fp) != 1) {
            break;
        }
        
        if (strcmp(user.id, id) == 0) {
            found = 1;
            if (strcmp(user.pwd, pwd) == 0) {
                // 登录成功，保存当前用户信息
                memcpy(&currentUser, &user, sizeof(User));
                
                // 读取完整的用户数据到内存
                fseek(fp, filePos, SEEK_SET);
                
                if (currentUserData != NULL) {
                    free(currentUserData);
                    currentUserData = NULL;
                }
                
                switch (role) {
                    case ROLE_STUDENT: {
                        Student* student = (Student*)malloc(sizeof(Student));
                        if (fread(student, sizeof(Student), 1, fp) == 1) {
                            currentUserData = student;
                        } else {
                            free(student);
                        }
                        break;
                    }
                    case ROLE_TEACHER: {
                        Teacher* teacher = (Teacher*)malloc(sizeof(Teacher));
                        if (fread(teacher, sizeof(Teacher), 1, fp) == 1) {
                            currentUserData = teacher;
                        } else {
                            free(teacher);
                        }
                        break;
                    }
                    case ROLE_ADMIN: {
                        Admin* admin = (Admin*)malloc(sizeof(Admin));
                        if (fread(admin, sizeof(Admin), 1, fp) == 1) {
                            currentUserData = admin;
                        } else {
                            free(admin);
                        }
                        break;
                    }
                }
                
                fclose(fp);
                return 1;
            } else {
                fclose(fp);
                printf("密码错误\n");
                return 0;
            }
        }
        // 跳过额外数据
        fseek(fp, size - sizeof(User), SEEK_CUR);
    }

    fclose(fp);
    if (!found) {
        printf("未查询到该用户！\n");
    }
    return 0;
}

// 显示学生信息 - 新增实现
void showStudentInfo(const char* studentId) {
    clean();
    printf("=== 学生个人信息 ===\n");
    
    if (currentUser.role == ROLE_STUDENT && currentUserData != NULL) {
        Student* student = (Student*)currentUserData;
        printf("学号: %s\n", student->base.id);
        printf("姓名: %s\n", student->base.name);
        printf("班级: %s\n", student->className);
        printf("成绩数量: %d\n", student->scoreCount);
        printf("总成绩: %.2f\n", student->totalScore);
        printf("平均成绩: %.2f\n", student->averageScore);
        printf("班级排名: %d\n", student->rank);
        
        if (student->scoreCount > 0) {
            printf("\n=== 成绩列表 ===\n");
            printf("%-20s %-10s %-15s\n", "课程名称", "分数", "考试日期");
            printf("------------------------------------------------\n");
            for (int i = 0; i < student->scoreCount; i++) {
                printf("%-20s %-10.2f %-15s\n", 
                       student->scores[i].courseName,
                       student->scores[i].score,
                       student->scores[i].examDate);
            }
        }
    } else {
        // 从文件读取
        FILE* fp = fopen("students.bin", "rb");
        if (fp != NULL) {
            Student student;
            while (fread(&student, sizeof(Student), 1, fp) == 1) {
                if (strcmp(student.base.id, studentId) == 0) {
                    printf("学号: %s\n", student.base.id);
                    printf("姓名: %s\n", student.base.name);
                    printf("班级: %s\n", student.className);
                    printf("成绩数量: %d\n", student.scoreCount);
                    printf("总成绩: %.2f\n", student.totalScore);
                    printf("平均成绩: %.2f\n", student.averageScore);
                    printf("班级排名: %d\n", student.rank);
                    
                    if (student.scoreCount > 0) {
                        printf("\n=== 成绩列表 ===\n");
                        printf("%-20s %-10s %-15s\n", "课程名称", "分数", "考试日期");
                        printf("------------------------------------------------\n");
                        for (int i = 0; i < student.scoreCount; i++) {
                            printf("%-20s %-10.2f %-15s\n", 
                                   student.scores[i].courseName,
                                   student.scores[i].score,
                                   student.scores[i].examDate);
                        }
                    }
                    break;
                }
            }
            fclose(fp);
        }
    }
}

// 修改学生密码 - 新增实现
void changeStudentPassword(const char* studentId) {
    char old_pwd[MAX_DATA], new_pwd[MAX_DATA], confirm_pwd[MAX_DATA];
    clean();
    printf("=== 修改密码 ===\n");
    
    printf("请输入原密码: ");
    safe_input_pwd(old_pwd, sizeof(old_pwd));
    
    // 验证原密码
    if (strcmp(old_pwd, currentUser.pwd) != 0) {
        printf("原密码错误！\n");
        return;
    }
    
    printf("请输入新密码: ");
    safe_input_pwd(new_pwd, sizeof(new_pwd));
    printf("请确认新密码: ");
    safe_input_pwd(confirm_pwd, sizeof(confirm_pwd));
    
    if (strcmp(new_pwd, confirm_pwd) != 0) {
        printf("两次输入的密码不一致！\n");
        return;
    }
    
    if (!validatePassword(new_pwd)) {
        printf("密码不符合要求！密码至少6位，包含字母和数字\n");
        return;
    }
    
    updateUserPwd(studentId, new_pwd, ROLE_STUDENT);
}

// 查询个人成绩 - 新增实现
void queryPersonalScores(const char* studentId) {
    clean();
    printf("=== 个人成绩查询 ===\n");
    
    if (currentUser.role == ROLE_STUDENT && currentUserData != NULL) {
        Student* student = (Student*)currentUserData;
        if (student->scoreCount > 0) {
            printf("学号: %s\n", student->base.id);
            printf("姓名: %s\n", student->base.name);
            printf("班级: %s\n", student->className);
            printf("\n=== 成绩列表 ===\n");
            printf("%-20s %-10s %-15s\n", "课程名称", "分数", "考试日期");
            printf("------------------------------------------------\n");
            for (int i = 0; i < student->scoreCount; i++) {
                printf("%-20s %-10.2f %-15s\n", 
                       student->scores[i].courseName,
                       student->scores[i].score,
                       student->scores[i].examDate);
            }
            printf("\n平均成绩: %.2f\n", student->averageScore);
        } else {
            printf("暂无成绩记录！\n");
        }
    } else {
        // 从文件读取
        FILE* fp = fopen("students.bin", "rb");
        if (fp != NULL) {
            Student student;
            while (fread(&student, sizeof(Student), 1, fp) == 1) {
                if (strcmp(student.base.id, studentId) == 0) {
                    if (student.scoreCount > 0) {
                        printf("学号: %s\n", student.base.id);
                        printf("姓名: %s\n", student.base.name);
                        printf("班级: %s\n", student.className);
                        printf("\n=== 成绩列表 ===\n");
                        printf("%-20s %-10s %-15s\n", "课程名称", "分数", "考试日期");
                        printf("------------------------------------------------\n");
                        for (int i = 0; i < student.scoreCount; i++) {
                            printf("%-20s %-10.2f %-15s\n", 
                                   student.scores[i].courseName,
                                   student.scores[i].score,
                                   student.scores[i].examDate);
                        }
                        printf("\n平均成绩: %.2f\n", student.averageScore);
                    } else {
                        printf("暂无成绩记录！\n");
                    }
                    break;
                }
            }
            fclose(fp);
        }
    }
}

// 查询班级成绩排名 - 新增实现
void queryClassRanking(const char* studentId) {
    clean();
    printf("=== 班级成绩排名 ===\n");
    
    char className[MAX_NAME] = "";
    
    // 获取学生所在班级
    if (currentUser.role == ROLE_STUDENT && currentUserData != NULL) {
        Student* student = (Student*)currentUserData;
        strcpy(className, student->className);
    } else {
        FILE* fp = fopen("students.bin", "rb");
        if (fp != NULL) {
            Student student;
            while (fread(&student, sizeof(Student), 1, fp) == 1) {
                if (strcmp(student.base.id, studentId) == 0) {
                    strcpy(className, student.className);
                    break;
                }
            }
            fclose(fp);
        }
    }
    
    if (strlen(className) == 0) {
        printf("无法获取学生班级信息！\n");
        return;
    }
    
    printf("班级: %s\n\n", className);
    
    // 读取班级所有学生成绩并排序
    Student* classStudents = NULL;
    int classStudentCount = 0;
    
    FILE* fp = fopen("students.bin", "rb");
    if (fp != NULL) {
        Student student;
        while (fread(&student, sizeof(Student), 1, fp) == 1) {
            if (strcmp(student.className, className) == 0) {
                classStudents = (Student*)realloc(classStudents, (classStudentCount + 1) * sizeof(Student));
                if (classStudents != NULL) {
                    classStudents[classStudentCount] = student;
                    classStudentCount++;
                }
            }
        }
        fclose(fp);
    }
    
    if (classStudentCount == 0) {
        printf("班级中暂无学生！\n");
        if (classStudents != NULL) free(classStudents);
        return;
    }
    
    // 按平均成绩排序
    for (int i = 0; i < classStudentCount - 1; i++) {
        for (int j = i + 1; j < classStudentCount; j++) {
            if (classStudents[i].averageScore < classStudents[j].averageScore) {
                Student temp = classStudents[i];
                classStudents[i] = classStudents[j];
                classStudents[j] = temp;
            }
        }
    }
    
    // 显示排名
    printf("%-5s %-15s %-20s %-10s\n", "排名", "学号", "姓名", "平均成绩");
    printf("----------------------------------------------------------\n");
    for (int i = 0; i < classStudentCount; i++) {
        printf("%-5d %-15s %-20s %-10.2f\n", 
               i + 1,
               classStudents[i].base.id,
               classStudents[i].base.name,
               classStudents[i].averageScore);
    }
    
    free(classStudents);
}

// 分析学生成绩 - 新增实现
void analyzeStudentScores(const char* studentId) {
    clean();
    printf("=== 成绩分析 ===\n");
    
    Student* student = NULL;
    
    if (currentUser.role == ROLE_STUDENT && currentUserData != NULL) {
        student = (Student*)currentUserData;
    } else {
        FILE* fp = fopen("students.bin", "rb");
        if (fp != NULL) {
            Student tempStudent;
            while (fread(&tempStudent, sizeof(Student), 1, fp) == 1) {
                if (strcmp(tempStudent.base.id, studentId) == 0) {
                    student = (Student*)malloc(sizeof(Student));
                    if (student != NULL) {
                        memcpy(student, &tempStudent, sizeof(Student));
                    }
                    break;
                }
            }
            fclose(fp);
        }
    }
    
    if (student == NULL) {
        printf("无法获取学生信息！\n");
        return;
    }
    
    printf("学号: %s\n", student->base.id);
    printf("姓名: %s\n", student->base.name);
    printf("班级: %s\n\n", student->className);
    
    if (student->scoreCount == 0) {
        printf("暂无成绩记录，无法进行分析！\n");
        if (currentUser.role != ROLE_STUDENT) free(student);
        return;
    }
    
    // 计算各等级成绩数量
    int excellentCount = 0, goodCount = 0, averageCount = 0, passCount = 0, failCount = 0;
    double maxScore = 0, minScore = 100;
    char maxCourse[MAX_NAME] = "", minCourse[MAX_NAME] = "";
    
    for (int i = 0; i < student->scoreCount; i++) {
        double score = student->scores[i].score;
        
        if (score >= 90) excellentCount++;
        else if (score >= 80) goodCount++;
        else if (score >= 70) averageCount++;
        else if (score >= 60) passCount++;
        else failCount++;
        
        if (score > maxScore) {
            maxScore = score;
            strcpy(maxCourse, student->scores[i].courseName);
        }
        if (score < minScore) {
            minScore = score;
            strcpy(minCourse, student->scores[i].courseName);
        }
    }
    
    printf("=== 成绩统计 ===\n");
    printf("总课程数: %d\n", student->scoreCount);
    printf("平均成绩: %.2f\n", student->averageScore);
    printf("最高成绩: %.2f (%s)\n", maxScore, maxCourse);
    printf("最低成绩: %.2f (%s)\n", minScore, minCourse);
    printf("\n=== 成绩分布 ===\n");
    printf("优秀(90-100): %d 门 (%.1f%%)\n", excellentCount, (double)excellentCount / student->scoreCount * 100);
    printf("良好(80-89):  %d 门 (%.1f%%)\n", goodCount, (double)goodCount / student->scoreCount * 100);
    printf("中等(70-79):  %d 门 (%.1f%%)\n", averageCount, (double)averageCount / student->scoreCount * 100);
    printf("及格(60-69):  %d 门 (%.1f%%)\n", passCount, (double)passCount / student->scoreCount * 100);
    printf("不及格(<60):  %d 门 (%.1f%%)\n", failCount, (double)failCount / student->scoreCount * 100);
    
    printf("\n=== 学习建议 ===\n");
    if (student->averageScore >= 85) {
        printf("整体成绩优秀，建议继续保持并争取更好成绩！\n");
    } else if (student->averageScore >= 75) {
        printf("成绩良好，建议加强薄弱科目，争取更大进步！\n");
    } else if (student->averageScore >= 60) {
        printf("成绩及格，建议更加努力学习，提高整体成绩！\n");
    } else {
        printf("多门课程不及格，建议认真复习，必要时寻求老师帮助！\n");
    }
    
    if (failCount > 0) {
        printf("需要重点关注不及格科目，制定学习计划！\n");
    }
    
    if (currentUser.role != ROLE_STUDENT) free(student);
}

// 显示教师信息 - 新增实现
void showTeacherInfo(const char* teacherId) {
    clean();
    printf("=== 教师个人信息 ===\n");
    
    if (currentUser.role == ROLE_TEACHER && currentUserData != NULL) {
        Teacher* teacher = (Teacher*)currentUserData;
        printf("工号: %s\n", teacher->base.id);
        printf("姓名: %s\n", teacher->base.name);
        printf("管理班级数: %d\n", teacher->classCount);
        printf("教授课程数: %d\n", teacher->courseCount);
        
        if (teacher->classCount > 0) {
            printf("\n=== 管理班级 ===\n");
            for (int i = 0; i < teacher->classCount; i++) {
                printf("%d. %s\n", i + 1, teacher->classNames[i]);
            }
        }
        
        if (teacher->courseCount > 0) {
            printf("\n=== 教授课程 ===\n");
            for (int i = 0; i < teacher->courseCount; i++) {
                printf("%d. %s\n", i + 1, teacher->courses[i]);
            }
        }
    } else {
        // 从文件读取
        FILE* fp = fopen("teachers.bin", "rb");
        if (fp != NULL) {
            Teacher teacher;
            while (fread(&teacher, sizeof(Teacher), 1, fp) == 1) {
                if (strcmp(teacher.base.id, teacherId) == 0) {
                    printf("工号: %s\n", teacher.base.id);
                    printf("姓名: %s\n", teacher.base.name);
                    printf("管理班级数: %d\n", teacher.classCount);
                    printf("教授课程数: %d\n", teacher.courseCount);
                    
                    if (teacher.classCount > 0) {
                        printf("\n=== 管理班级 ===\n");
                        for (int i = 0; i < teacher.classCount; i++) {
                            printf("%d. %s\n", i + 1, teacher.classNames[i]);
                        }
                    }
                    
                    if (teacher.courseCount > 0) {
                        printf("\n=== 教授课程 ===\n");
                        for (int i = 0; i < teacher.courseCount; i++) {
                            printf("%d. %s\n", i + 1, teacher.courses[i]);
                        }
                    }
                    break;
                }
            }
            fclose(fp);
        }
    }
}

// 修改教师密码 - 新增实现
void changeTeacherPassword(const char* teacherId) {
    char old_pwd[MAX_DATA], new_pwd[MAX_DATA], confirm_pwd[MAX_DATA];
    clean();
    printf("=== 修改密码 ===\n");
    
    printf("请输入原密码: ");
    safe_input_pwd(old_pwd, sizeof(old_pwd));
    
    // 验证原密码
    if (strcmp(old_pwd, currentUser.pwd) != 0) {
        printf("原密码错误！\n");
        return;
    }
    
    printf("请输入新密码: ");
    safe_input_pwd(new_pwd, sizeof(new_pwd));
    printf("请确认新密码: ");
    safe_input_pwd(confirm_pwd, sizeof(confirm_pwd));
    
    if (strcmp(new_pwd, confirm_pwd) != 0) {
        printf("两次输入的密码不一致！\n");
        return;
    }
    
    if (!validatePassword(new_pwd)) {
        printf("密码不符合要求！密码至少6位，包含字母和数字\n");
        return;
    }
    
    updateUserPwd(teacherId, new_pwd, ROLE_TEACHER);
}

// 教师学生管理 - 新增实现
void teacherStudentManagement(const char* teacherId) {
    int choice;
    do {
        clean();
        printf("=== 学生管理 ===\n");
        printf("1. 查看班级学生列表\n");
        printf("2. 搜索学生\n");
        printf("3. 添加学生成绩\n");
        printf("4. 修改学生成绩\n");
        printf("5. 返回上一级\n");
        printf("请选择: ");
        choice = safe_int_input();

        switch (choice) {
            case 1:
                // 查看班级学生列表
                if (currentUserData != NULL && currentUser.role == ROLE_TEACHER) {
                    Teacher* teacher = (Teacher*)currentUserData;
                    if (teacher->classCount > 0) {
                        printf("\n请选择要查看的班级：\n");
                        for (int i = 0; i < teacher->classCount; i++) {
                            printf("%d. %s\n", i + 1, teacher->classNames[i]);
                        }
                        printf("请选择: ");
                        int classIndex = safe_int_input() - 1;
                        if (classIndex >= 0 && classIndex < teacher->classCount) {
                            listStudentsByClass(teacher->classNames[classIndex]);
                        } else {
                            printf("无效的选择！\n");
                        }
                    } else {
                        printf("您还没有管理的班级！\n");
                    }
                }
                wait();
                break;
            case 2:
                // 搜索学生
                searchStudentByTeacher();
                wait();
                break;
            case 3:
                // 添加学生成绩
                addStudentScoreByTeacher();
                wait();
                break;
            case 4:
                // 修改学生成绩
                updateStudentScoreByTeacher();
                wait();
                break;
            case 5:
                printf("返回上一级...\n");
                break;
            default:
                printf("无效输入，请重新选择！\n");
                wait();
        }
    } while (choice != 5);
}

// 按班级列出学生
void listStudentsByClass(const char* className) {
    clean();
    printf("=== %s 学生列表 ===\n", className);
    
    FILE* fp = fopen("students.bin", "rb");
    if (fp == NULL) {
        printf("暂无学生数据\n");
        return;
    }

    Student student;
    int count = 0;
    printf("%-5s %-15s %-20s %-10s\n", "序号", "学号", "姓名", "平均成绩");
    printf("----------------------------------------------------------\n");

    while (fread(&student, sizeof(Student), 1, fp) == 1) {
        if (strcmp(student.className, className) == 0) {
            count++;
            printf("%-5d %-15s %-20s %-10.2f\n", 
                   count, student.base.id, student.base.name, student.averageScore);
        }
    }

    fclose(fp);
    
    if (count == 0) {
        printf("该班级暂无学生！\n");
    } else {
        printf("\n共 %d 名学生\n", count);
    }
}

// 教师搜索学生
void searchStudentByTeacher() {
    char studentId[MAX_DATA];
    clean();
    printf("=== 搜索学生 ===\n");
    printf("请输入学生学号：");
    safe_input(studentId, sizeof(studentId));
    
    if (strlen(studentId) == 0) {
        printf("学号不能为空！\n");
        return;
    }
    
    FILE* fp = fopen("students.bin", "rb");
    if (fp != NULL) {
        Student student;
        while (fread(&student, sizeof(Student), 1, fp) == 1) {
            if (strcmp(student.base.id, studentId) == 0) {
                printf("\n找到学生：\n");
                printf("学号: %s\n", student.base.id);
                printf("姓名: %s\n", student.base.name);
                printf("班级: %s\n", student.className);
                printf("平均成绩: %.2f\n", student.averageScore);
                
                if (student.scoreCount > 0) {
                    printf("\n=== 成绩列表 ===\n");
                    printf("%-20s %-10s %-15s\n", "课程名称", "分数", "考试日期");
                    printf("------------------------------------------------\n");
                    for (int i = 0; i < student.scoreCount; i++) {
                        printf("%-20s %-10.2f %-15s\n", 
                               student.scores[i].courseName,
                               student.scores[i].score,
                               student.scores[i].examDate);
                    }
                }
                fclose(fp);
                return;
            }
        }
        fclose(fp);
    }
    
    printf("未找到学号为 %s 的学生！\n", studentId);
}

// 教师添加学生成绩
void addStudentScoreByTeacher() {
    char studentId[MAX_DATA], courseName[MAX_NAME], examDate[20];
    double score;
    
    clean();
    printf("=== 添加学生成绩 ===\n");
    printf("请输入学生学号：");
    safe_input(studentId, sizeof(studentId));
    
    if (!checkUserId(studentId, ROLE_STUDENT)) {
        printf("学生不存在！\n");
        return;
    }
    
    printf("请输入课程名称：");
    safe_input(courseName, sizeof(courseName));
    
    if (strlen(courseName) == 0) {
        printf("课程名称不能为空！\n");
        return;
    }
    
    printf("请输入考试日期（格式：YYYY-MM-DD）：");
    safe_input(examDate, sizeof(examDate));
    
    if (strlen(examDate) != 10 || examDate[4] != '-' || examDate[7] != '-') {
        printf("日期格式不正确！\n");
        return;
    }
    
    printf("请输入分数：");
    score = safe_double_input();
    
    if (score < 0 || score > 100) {
        printf("分数必须在0-100之间！\n");
        return;
    }
    
    // 备份当前文件用于恢复
    if (CopyFile("students.bin", "students_backup.bin", TRUE)) {
        FILE* fp = fopen("students.bin", "rb+");
        if (fp != NULL) {
            Student student;
            while (fread(&student, sizeof(Student), 1, fp) == 1) {
                if (strcmp(student.base.id, studentId) == 0) {
                    // 检查是否已有该课程成绩
                    int existingIndex = -1;
                    for (int i = 0; i < student.scoreCount; i++) {
                        if (strcmp(student.scores[i].courseName, courseName) == 0) {
                            existingIndex = i;
                            break;
                        }
                    }
                    
                    if (existingIndex != -1) {
                        // 更新已有成绩
                        student.scores[existingIndex].score = score;
                        strcpy(student.scores[existingIndex].examDate, examDate);
                    } else {
                        // 添加新成绩
                        if (student.scoreCount >= MAX_COURSE) {
                            printf("成绩数量已达上限！\n");
                            fclose(fp);
                            return;
                        }
                        
                        strcpy(student.scores[student.scoreCount].courseName, courseName);
                        student.scores[student.scoreCount].score = score;
                        strcpy(student.scores[student.scoreCount].examDate, examDate);
                        student.scoreCount++;
                    }
                    
                    // 重新计算统计信息
                    student.totalScore = 0;
                    for (int i = 0; i < student.scoreCount; i++) {
                        student.totalScore += student.scores[i].score;
                    }
                    student.averageScore = student.scoreCount > 0 ? student.totalScore / student.scoreCount : 0;
                    
                    // 写回文件
                    fseek(fp, -sizeof(Student), SEEK_CUR);
                    if (fwrite(&student, sizeof(Student), 1, fp) == 1) {
                        printf("成绩添加成功！\n");
                        
                        // 如果是当前登录学生，更新内存中的数据
                        if (currentUser.role == ROLE_STUDENT && currentUserData != NULL && 
                            strcmp(((Student*)currentUserData)->base.id, studentId) == 0) {
                            memcpy(currentUserData, &student, sizeof(Student));
                        }
                    } else {
                        printf("成绩添加失败！\n");
                        // 恢复备份文件
                        CopyFile("students_backup.bin", "students.bin", TRUE);
                    }
                    fclose(fp);
                    return;
                }
            }
            fclose(fp);
        }
    } else {
        printf("创建备份文件失败，操作取消！\n");
    }
    
    printf("操作失败！\n");
}

// 教师修改学生成绩
void updateStudentScoreByTeacher() {
    char studentId[MAX_DATA], courseName[MAX_NAME];
    
    clean();
    printf("=== 修改学生成绩 ===\n");
    printf("请输入学生学号：");
    safe_input(studentId, sizeof(studentId));
    
    if (!checkUserId(studentId, ROLE_STUDENT)) {
        printf("学生不存在！\n");
        return;
    }
    
    // 显示学生当前成绩
    FILE* fp = fopen("students.bin", "rb");
    if (fp != NULL) {
        Student student;
        while (fread(&student, sizeof(Student), 1, fp) == 1) {
            if (strcmp(student.base.id, studentId) == 0) {
                if (student.scoreCount == 0) {
                    printf("该学生暂无成绩记录！\n");
                    fclose(fp);
                    return;
                }
                
                printf("\n=== 当前成绩列表 ===\n");
                printf("%-5s %-20s %-10s %-15s\n", "序号", "课程名称", "分数", "考试日期");
                printf("----------------------------------------------------------\n");
                for (int i = 0; i < student.scoreCount; i++) {
                    printf("%-5d %-20s %-10.2f %-15s\n", 
                           i + 1,
                           student.scores[i].courseName,
                           student.scores[i].score,
                           student.scores[i].examDate);
                }
                
                printf("\n请选择要修改的成绩序号：");
                int choice = safe_int_input() - 1;
                
                if (choice < 0 || choice >= student.scoreCount) {
                    printf("无效的选择！\n");
                    fclose(fp);
                    return;
                }
                
                strcpy(courseName, student.scores[choice].courseName);
                
                fclose(fp);
                
                // 修改成绩
                printf("\n当前分数：%.2f\n", student.scores[choice].score);
                printf("请输入新分数：");
                double newScore = safe_double_input();
                
                if (newScore < 0 || newScore > 100) {
                    printf("分数必须在0-100之间！\n");
                    return;
                }
                
                // 备份并更新
                if (CopyFile("students.bin", "students_backup.bin", TRUE)) {
                    FILE* updateFp = fopen("students.bin", "rb+");
                    if (updateFp != NULL) {
                        Student updateStudent;
                        while (fread(&updateStudent, sizeof(Student), 1, updateFp) == 1) {
                            if (strcmp(updateStudent.base.id, studentId) == 0) {
                                // 更新指定课程成绩
                                for (int i = 0; i < updateStudent.scoreCount; i++) {
                                    if (strcmp(updateStudent.scores[i].courseName, courseName) == 0) {
                                        updateStudent.scores[i].score = newScore;
                                        
                                        // 重新计算统计信息
                                        updateStudent.totalScore = 0;
                                        for (int j = 0; j < updateStudent.scoreCount; j++) {
                                            updateStudent.totalScore += updateStudent.scores[j].score;
                                        }
                                        updateStudent.averageScore = updateStudent.totalScore / updateStudent.scoreCount;
                                        
                                        // 写回文件
                                        fseek(updateFp, -sizeof(Student), SEEK_CUR);
                                        if (fwrite(&updateStudent, sizeof(Student), 1, updateFp) == 1) {
                                            printf("成绩修改成功！\n");
                                            
                                            // 更新内存数据
                                            if (currentUser.role == ROLE_STUDENT && currentUserData != NULL && 
                                                strcmp(((Student*)currentUserData)->base.id, studentId) == 0) {
                                                memcpy(currentUserData, &updateStudent, sizeof(Student));
                                            }
                                        } else {
                                            printf("成绩修改失败！\n");
                                            CopyFile("students_backup.bin", "students.bin", TRUE);
                                        }
                                        fclose(updateFp);
                                        return;
                                    }
                                }
                            }
                        }
                        fclose(updateFp);
                    }
                } else {
                    printf("创建备份文件失败，操作取消！\n");
                }
                return;
            }
        }
        fclose(fp);
    }
    
    printf("操作失败！\n");
}

// 教师班级管理 - 新增实现
void teacherClassManagement(const char* teacherId) {
    int choice;
    do {
        clean();
        printf("=== 班级管理 ===\n");
        printf("1. 查看管理班级\n");
        printf("2. 查看班级详情\n");
        printf("3. 添加班级课程\n");
        printf("4. 返回上一级\n");
        printf("请选择: ");
        choice = safe_int_input();

        switch (choice) {
            case 1:
                // 查看管理班级
                if (currentUserData != NULL && currentUser.role == ROLE_TEACHER) {
                    Teacher* teacher = (Teacher*)currentUserData;
                    if (teacher->classCount > 0) {
                        printf("\n=== 管理班级列表 ===\n");
                        for (int i = 0; i < teacher->classCount; i++) {
                            printf("%d. %s\n", i + 1, teacher->classNames[i]);
                        }
                    } else {
                        printf("您还没有管理的班级！\n");
                    }
                }
                wait();
                break;
            case 2:
                // 查看班级详情
                if (currentUserData != NULL && currentUser.role == ROLE_TEACHER) {
                    Teacher* teacher = (Teacher*)currentUserData;
                    if (teacher->classCount > 0) {
                        printf("\n请选择要查看的班级：\n");
                        for (int i = 0; i < teacher->classCount; i++) {
                            printf("%d. %s\n", i + 1, teacher->classNames[i]);
                        }
                        printf("请选择: ");
                        int classIndex = safe_int_input() - 1;
                        if (classIndex >= 0 && classIndex < teacher->classCount) {
                            showClassDetails(teacher->classNames[classIndex]);
                        } else {
                            printf("无效的选择！\n");
                        }
                    } else {
                        printf("您还没有管理的班级！\n");
                    }
                }
                wait();
                break;
            case 3:
                // 添加班级课程
                addClassCourseByTeacher();
                wait();
                break;
            case 4:
                printf("返回上一级...\n");
                break;
            default:
                printf("无效输入，请重新选择！\n");
                wait();
        }
    } while (choice != 4);
}

// 显示班级详情
void showClassDetails(const char* className) {
    clean();
    printf("=== %s 班级详情 ===\n", className);
    
    // 统计班级信息
    int studentCount = 0;
    double totalScore = 0;
    double maxScore = 0;
    double minScore = 100;
    char maxStudent[MAX_NAME] = "", minStudent[MAX_NAME] = "";
    
    FILE* fp = fopen("students.bin", "rb");
    if (fp != NULL) {
        Student student;
        while (fread(&student, sizeof(Student), 1, fp) == 1) {
            if (strcmp(student.className, className) == 0) {
                studentCount++;
                totalScore += student.averageScore;
                
                if (student.averageScore > maxScore) {
                    maxScore = student.averageScore;
                    strcpy(maxStudent, student.base.name);
                }
                if (student.averageScore < minScore) {
                    minScore = student.averageScore;
                    strcpy(minStudent, student.base.name);
                }
            }
        }
        fclose(fp);
    }
    
    printf("班级人数: %d\n", studentCount);
    if (studentCount > 0) {
        printf("平均成绩: %.2f\n", totalScore / studentCount);
        printf("最高平均成绩: %.2f (%s)\n", maxScore, maxStudent);
        printf("最低平均成绩: %.2f (%s)\n", minScore, minStudent);
    }
    
    printf("\n按任意键查看学生列表...\n");
    getch();
    
    listStudentsByClass(className);
}

// 教师添加班级课程
void addClassCourseByTeacher() {
    if (currentUserData == NULL || currentUser.role != ROLE_TEACHER) {
        printf("无法获取教师信息！\n");
        return;
    }
    
    Teacher* teacher = (Teacher*)currentUserData;
    
    if (teacher->courseCount >= MAX_COURSE) {
        printf("课程数量已达上限！\n");
        return;
    }
    
    char courseName[MAX_NAME];
    clean();
    printf("=== 添加课程 ===\n");
    printf("请输入课程名称：");
    safe_input(courseName, sizeof(courseName));
    
    if (strlen(courseName) == 0) {
        printf("课程名称不能为空！\n");
        return;
    }
    
    // 检查课程是否已存在
    for (int i = 0; i < teacher->courseCount; i++) {
        if (strcmp(teacher->courses[i], courseName) == 0) {
            printf("课程已存在！\n");
            return;
        }
    }
    
    // 备份并更新
    if (CopyFile("teachers.bin", "teachers_backup.bin", TRUE)) {
        FILE* fp = fopen("teachers.bin", "rb+");
        if (fp != NULL) {
            Teacher updateTeacher;
            while (fread(&updateTeacher, sizeof(Teacher), 1, fp) == 1) {
                if (strcmp(updateTeacher.base.id, teacher->base.id) == 0) {
                    // 添加新课程
                    strcpy(updateTeacher.courses[updateTeacher.courseCount], courseName);
                    updateTeacher.courseCount++;
                    
                    // 写回文件
                    fseek(fp, -sizeof(Teacher), SEEK_CUR);
                    if (fwrite(&updateTeacher, sizeof(Teacher), 1, fp) == 1) {
                        printf("课程添加成功！\n");
                        
                        // 更新内存数据
                        memcpy(teacher, &updateTeacher, sizeof(Teacher));
                    } else {
                        printf("课程添加失败！\n");
                        CopyFile("teachers_backup.bin", "teachers.bin", TRUE);
                    }
                    fclose(fp);
                    return;
                }
            }
            fclose(fp);
        }
    } else {
        printf("创建备份文件失败，操作取消！\n");
    }
}

// 教师成绩管理 - 新增实现
void teacherScoreManagement(const char* teacherId) {
    int choice;
    do {
        clean();
        printf("=== 成绩管理 ===\n");
        printf("1. 按班级查看成绩\n");
        printf("2. 按课程查看成绩\n");
        printf("3. 成绩统计分析\n");
        printf("4. 导出成绩数据\n");
        printf("5. 返回上一级\n");
        printf("请选择: ");
        choice = safe_int_input();

        switch (choice) {
            case 1:
                // 按班级查看成绩
                viewScoresByClass();
                wait();
                break;
            case 2:
                // 按课程查看成绩
                viewScoresByCourse();
                wait();
                break;
            case 3:
                // 成绩统计分析
                analyzeScoresByTeacher();
                wait();
                break;
            case 4:
                // 导出成绩数据
                exportScoresByTeacher();
                wait();
                break;
            case 5:
                printf("返回上一级...\n");
                break;
            default:
                printf("无效输入，请重新选择！\n");
                wait();
        }
    } while (choice != 5);
}

// 按班级查看成绩
void viewScoresByClass() {
    if (currentUserData == NULL || currentUser.role != ROLE_TEACHER) {
        printf("无法获取教师信息！\n");
        return;
    }
    
    Teacher* teacher = (Teacher*)currentUserData;
    
    if (teacher->classCount == 0) {
        printf("您还没有管理的班级！\n");
        return;
    }
    
    printf("\n请选择要查看的班级：\n");
    for (int i = 0; i < teacher->classCount; i++) {
        printf("%d. %s\n", i + 1, teacher->classNames[i]);
    }
    printf("请选择: ");
    int classIndex = safe_int_input() - 1;
    
    if (classIndex < 0 || classIndex >= teacher->classCount) {
        printf("无效的选择！\n");
        return;
    }
    
    char* className = teacher->classNames[classIndex];
    
    // 显示班级成绩
    clean();
    printf("=== %s 班级成绩 ===\n", className);
    
    FILE* fp = fopen("students.bin", "rb");
    if (fp != NULL) {
        Student student;
        while (fread(&student, sizeof(Student), 1, fp) == 1) {
            if (strcmp(student.className, className) == 0 && student.scoreCount > 0) {
                printf("\n学号: %s  姓名: %s\n", student.base.id, student.base.name);
                printf("%-20s %-10s %-15s\n", "课程名称", "分数", "考试日期");
                printf("------------------------------------------------\n");
                for (int i = 0; i < student.scoreCount; i++) {
                    printf("%-20s %-10.2f %-15s\n", 
                           student.scores[i].courseName,
                           student.scores[i].score,
                           student.scores[i].examDate);
                }
                printf("平均成绩: %.2f\n", student.averageScore);
                printf("----------------------------------------\n");
            }
        }
        fclose(fp);
    }
}

// 按课程查看成绩
void viewScoresByCourse() {
    if (currentUserData == NULL || currentUser.role != ROLE_TEACHER) {
        printf("无法获取教师信息！\n");
        return;
    }
    
    Teacher* teacher = (Teacher*)currentUserData;
    
    if (teacher->courseCount == 0) {
        printf("您还没有教授的课程！\n");
        return;
    }
    
    printf("\n请选择要查看的课程：\n");
    for (int i = 0; i < teacher->courseCount; i++) {
        printf("%d. %s\n", i + 1, teacher->courses[i]);
    }
    printf("请选择: ");
    int courseIndex = safe_int_input() - 1;
    
    if (courseIndex < 0 || courseIndex >= teacher->courseCount) {
        printf("无效的选择！\n");
        return;
    }
    
    char* courseName = teacher->courses[courseIndex];
    
    // 显示课程成绩
    clean();
    printf("=== %s 课程成绩 ===\n", courseName);
    
    int count = 0;
    double totalScore = 0;
    
    FILE* fp = fopen("students.bin", "rb");
    if (fp != NULL) {
        Student student;
        printf("%-15s %-20s %-10s %-15s\n", "学号", "姓名", "分数", "考试日期");
        printf("----------------------------------------------------------\n");
        
        while (fread(&student, sizeof(Student), 1, fp) == 1) {
            for (int i = 0; i < student.scoreCount; i++) {
                if (strcmp(student.scores[i].courseName, courseName) == 0) {
                    count++;
                    totalScore += student.scores[i].score;
                    printf("%-15s %-20s %-10.2f %-15s\n", 
                           student.base.id,
                           student.base.name,
                           student.scores[i].score,
                           student.scores[i].examDate);
                    break;
                }
            }
        }
        fclose(fp);
    }
    
    if (count > 0) {
        printf("\n共 %d 名学生，平均分数：%.2f\n", count, totalScore / count);
    } else {
        printf("暂无该课程的成绩记录！\n");
    }
}

// 教师成绩统计分析
void analyzeScoresByTeacher() {
    if (currentUserData == NULL || currentUser.role != ROLE_TEACHER) {
        printf("无法获取教师信息！\n");
        return;
    }
    
    Teacher* teacher = (Teacher*)currentUserData;
    
    if (teacher->courseCount == 0) {
        printf("您还没有教授的课程！\n");
        return;
    }
    
    printf("\n请选择要分析的课程：\n");
    for (int i = 0; i < teacher->courseCount; i++) {
        printf("%d. %s\n", i + 1, teacher->courses[i]);
    }
    printf("请选择: ");
    int courseIndex = safe_int_input() - 1;
    
    if (courseIndex < 0 || courseIndex >= teacher->courseCount) {
        printf("无效的选择！\n");
        return;
    }
    
    char* courseName = teacher->courses[courseIndex];
    
    // 分析课程成绩
    analyzeCourseScores(courseName);
}

// 分析课程成绩
void analyzeCourseScores(const char* courseName) {
    clean();
    printf("=== %s 课程成绩分析 ===\n", courseName);
    
    int totalStudents = 0;
    double totalScore = 0;
    double maxScore = 0;
    double minScore = 100;
    int excellentCount = 0, goodCount = 0, averageCount = 0, passCount = 0, failCount = 0;
    char maxStudent[MAX_NAME] = "", minStudent[MAX_NAME] = "";
    
    FILE* fp = fopen("students.bin", "rb");
    if (fp != NULL) {
        Student student;
        while (fread(&student, sizeof(Student), 1, fp) == 1) {
            for (int i = 0; i < student.scoreCount; i++) {
                if (strcmp(student.scores[i].courseName, courseName) == 0) {
                    totalStudents++;
                    double score = student.scores[i].score;
                    totalScore += score;
                    
                    if (score >= 90) excellentCount++;
                    else if (score >= 80) goodCount++;
                    else if (score >= 70) averageCount++;
                    else if (score >= 60) passCount++;
                    else failCount++;
                    
                    if (score > maxScore) {
                        maxScore = score;
                        strcpy(maxStudent, student.base.name);
                    }
                    if (score < minScore) {
                        minScore = score;
                        strcpy(minStudent, student.base.name);
                    }
                    break;
                }
            }
        }
        fclose(fp);
    }
    
    if (totalStudents == 0) {
        printf("暂无该课程的成绩记录！\n");
        return;
    }
    
    double averageScore = totalScore / totalStudents;
    double passRate = (double)(totalStudents - failCount) / totalStudents * 100;
    
    printf("=== 基本统计 ===\n");
    printf("参考人数: %d\n", totalStudents);
    printf("平均分数: %.2f\n", averageScore);
    printf("最高分数: %.2f (%s)\n", maxScore, maxStudent);
    printf("最低分数: %.2f (%s)\n", minScore, minStudent);
    printf("及格率: %.1f%%\n", passRate);
    
    printf("\n=== 成绩分布 ===\n");
    printf("优秀(90-100): %d 人 (%.1f%%)\n", excellentCount, (double)excellentCount / totalStudents * 100);
    printf("良好(80-89):  %d 人 (%.1f%%)\n", goodCount, (double)goodCount / totalStudents * 100);
    printf("中等(70-79):  %d 人 (%.1f%%)\n", averageCount, (double)averageCount / totalStudents * 100);
    printf("及格(60-69):  %d 人 (%.1f%%)\n", passCount, (double)passCount / totalStudents * 100);
    printf("不及格(<60):  %d 人 (%.1f%%)\n", failCount, (double)failCount / totalStudents * 100);
    
    printf("\n=== 教学建议 ===\n");
    if (averageScore >= 85) {
        printf("整体成绩优秀，教学效果良好！\n");
    } else if (averageScore >= 75) {
        printf("成绩良好，可以适当增加教学难度。\n");
    } else if (averageScore >= 65) {
        printf("成绩中等，建议加强重点难点的讲解。\n");
    } else {
        printf("整体成绩偏低，建议调整教学方法，加强课后辅导。\n");
    }
    
    if (failCount > totalStudents * 0.2) {
        printf("不及格率较高，需要特别关注差生，提供额外辅导。\n");
    }
    
    if (excellentCount < totalStudents * 0.1) {
        printf("优秀率偏低，可以设置一些挑战性的任务，激发学生潜能。\n");
    }
}

// 教师导出成绩数据
void exportScoresByTeacher() {
    if (currentUserData == NULL || currentUser.role != ROLE_TEACHER) {
        printf("无法获取教师信息！\n");
        return;
    }
    
    Teacher* teacher = (Teacher*)currentUserData;
    
    char filename[MAX_PATH];
    SYSTEMTIME st;
    GetLocalTime(&st);
    sprintf(filename, "scores_%s_%04d%02d%02d.txt", 
            teacher->base.id, st.wYear, st.wMonth, st.wDay);
    
    FILE* fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("创建文件失败！\n");
        return;
    }
    
    fprintf(fp, "教师: %s (%s)\n", teacher->base.name, teacher->base.id);
    fprintf(fp, "导出时间: %04d-%02d-%02d %02d:%02d:%02d\n", 
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    fprintf(fp, "========================================\n\n");
    
    // 导出每个班级的成绩
    for (int i = 0; i < teacher->classCount; i++) {
        char* className = teacher->classNames[i];
        fprintf(fp, "班级: %s\n", className);
        fprintf(fp, "%-15s %-20s %-10s\n", "学号", "姓名", "平均成绩");
        fprintf(fp, "----------------------------------------\n");
        
        FILE* studentFp = fopen("students.bin", "rb");
        if (studentFp != NULL) {
            Student student;
            while (fread(&student, sizeof(Student), 1, studentFp) == 1) {
                if (strcmp(student.className, className) == 0) {
                    fprintf(fp, "%-15s %-20s %-10.2f\n", 
                            student.base.id, student.base.name, student.averageScore);
                }
            }
            fclose(studentFp);
        }
        
        fprintf(fp, "\n");
    }
    
    fclose(fp);
    printf("成绩数据已导出到文件：%s\n", filename);
}

// 分析班级成绩 - 新增实现
void analyzeClassScores(const char* teacherId) {
    if (currentUserData == NULL || currentUser.role != ROLE_TEACHER) {
        printf("无法获取教师信息！\n");
        return;
    }
    
    Teacher* teacher = (Teacher*)currentUserData;
    
    if (teacher->classCount == 0) {
        printf("您还没有管理的班级！\n");
        return;
    }
    
    printf("\n请选择要分析的班级：\n");
    for (int i = 0; i < teacher->classCount; i++) {
        printf("%d. %s\n", i + 1, teacher->classNames[i]);
    }
    printf("请选择: ");
    int classIndex = safe_int_input() - 1;
    
    if (classIndex < 0 || classIndex >= teacher->classCount) {
        printf("无效的选择！\n");
        return;
    }
    
    char* className = teacher->classNames[classIndex];
    
    // 分析班级整体成绩
    clean();
    printf("=== %s 班级整体分析 ===\n", className);
    
    int totalStudents = 0;
    double totalAverageScore = 0;
    double maxAverage = 0;
    double minAverage = 100;
    
    FILE* fp = fopen("students.bin", "rb");
    if (fp != NULL) {
        Student student;
        while (fread(&student, sizeof(Student), 1, fp) == 1) {
            if (strcmp(student.className, className) == 0) {
                totalStudents++;
                totalAverageScore += student.averageScore;
                
                if (student.averageScore > maxAverage) {
                    maxAverage = student.averageScore;
                }
                if (student.averageScore < minAverage) {
                    minAverage = student.averageScore;
                }
            }
        }
        fclose(fp);
    }
    
    if (totalStudents == 0) {
        printf("该班级暂无学生！\n");
        return;
    }
    
    double classAverage = totalAverageScore / totalStudents;
    
    printf("班级人数: %d\n", totalStudents);
    printf("班级平均成绩: %.2f\n", classAverage);
    printf("最高平均分: %.2f\n", maxAverage);
    printf("最低平均分: %.2f\n", minAverage);
    printf("成绩差距: %.2f\n", maxAverage - minAverage);
    
    // 分析各科目成绩
    printf("\n=== 各科目成绩分析 ===\n");
    
    // 统计所有课程
    char courses[MAX_COURSE][MAX_NAME];
    int courseCount = 0;
    
    fp = fopen("students.bin", "rb");
    if (fp != NULL) {
        Student student;
        while (fread(&student, sizeof(Student), 1, fp) == 1) {
            if (strcmp(student.className, className) == 0) {
                for (int i = 0; i < student.scoreCount; i++) {
                    int exists = 0;
                    for (int j = 0; j < courseCount; j++) {
                        if (strcmp(courses[j], student.scores[i].courseName) == 0) {
                            exists = 1;
                            break;
                        }
                    }
                    if (!exists && courseCount < MAX_COURSE) {
                        strcpy(courses[courseCount], student.scores[i].courseName);
                        courseCount++;
                    }
                }
            }
        }
        fclose(fp);
    }
    
    if (courseCount > 0) {
        for (int i = 0; i < courseCount; i++) {
            analyzeSingleCourseInClass(className, courses[i]);
        }
    } else {
        printf("暂无课程成绩记录！\n");
    }
}

// 分析班级中单科成绩
void analyzeSingleCourseInClass(const char* className, const char* courseName) {
    int studentCount = 0;
    double totalScore = 0;
    double maxScore = 0;
    double minScore = 100;
    
    FILE* fp = fopen("students.bin", "rb");
    if (fp != NULL) {
        Student student;
        while (fread(&student, sizeof(Student), 1, fp) == 1) {
            if (strcmp(student.className, className) == 0) {
                for (int i = 0; i < student.scoreCount; i++) {
                    if (strcmp(student.scores[i].courseName, courseName) == 0) {
                        studentCount++;
                        double score = student.scores[i].score;
                        totalScore += score;
                        
                        if (score > maxScore) maxScore = score;
                        if (score < minScore) minScore = score;
                        break;
                    }
                }
            }
        }
        fclose(fp);
    }
    
    if (studentCount > 0) {
        double average = totalScore / studentCount;
        printf("%s: 平均 %.2f, 最高 %.2f, 最低 %.2f (%d人)\n", 
               courseName, average, maxScore, minScore, studentCount);
    }
}

// 导出学生数据 - 新增实现
void exportStudentData(const char* teacherId) {
    if (currentUserData == NULL || currentUser.role != ROLE_TEACHER) {
        printf("无法获取教师信息！\n");
        return;
    }
    
    Teacher* teacher = (Teacher*)currentUserData;
    
    char filename[MAX_PATH];
    SYSTEMTIME st;
    GetLocalTime(&st);
    sprintf(filename, "students_%s_%04d%02d%02d.txt", 
            teacher->base.id, st.wYear, st.wMonth, st.wDay);
    
    FILE* fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("创建文件失败！\n");
        return;
    }
    
    fprintf(fp, "教师: %s (%s)\n", teacher->base.name, teacher->base.id);
    fprintf(fp, "导出时间: %04d-%02d-%02d %02d:%02d:%02d\n", 
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    fprintf(fp, "========================================\n\n");
    
    // 导出每个班级的学生信息
    for (int i = 0; i < teacher->classCount; i++) {
        char* className = teacher->classNames[i];
        fprintf(fp, "班级: %s\n", className);
        fprintf(fp, "%-15s %-20s %-10s %-10s\n", "学号", "姓名", "平均分", "排名");
        fprintf(fp, "----------------------------------------\n");
        
        FILE* studentFp = fopen("students.bin", "rb");
        if (studentFp != NULL) {
            Student student;
            while (fread(&student, sizeof(Student), 1, studentFp) == 1) {
                if (strcmp(student.className, className) == 0) {
                    fprintf(fp, "%-15s %-20s %-10.2f %-10d\n", 
                            student.base.id, student.base.name, student.averageScore, student.rank);
                }
            }
            fclose(studentFp);
        }
        
        fprintf(fp, "\n");
    }
    
    fclose(fp);
    printf("学生数据已导出到文件：%s\n", filename);
}

// 管理员用户管理 - 修复：已实现但需要优化错误处理
void adminUserManagement() {
    int choice;
    do {
        clean();
        printf("=== 用户管理 ===\n");
        printf("1. 添加学生\n");
        printf("2. 添加教师\n");
        printf("3. 添加管理员\n");
        printf("4. 删除用户\n");
        printf("5. 搜索用户\n");
        printf("6. 查看用户列表\n");
        printf("7. 返回上一级\n");
        printf("请选择: ");
        choice = safe_int_input();

        switch (choice) {
            case 1:
                addStudentUI();
                wait();
                break;
            case 2:
                addTeacherUI();
                wait();
                break;
            case 3:
                addAdminUI();
                wait();
                break;
            case 4:
                deleteUserUI();
                wait();
                break;
            case 5:
                searchUser();
                wait();
                break;
            case 6:
                listAllUsers();
                wait();
                break;
            case 7:
                printf("返回上一级...\n");
                break;
            default:
                printf("无效输入，请重新选择！\n");
                wait();
        }
    } while (choice != 7);
}

// 学生菜单
void studentMenu() {
    int choice;
    do {
        clean();
        printf("=== 学生界面 ===\n");
        printf("欢迎您，%s 同学\n", currentUser.name);
        printf("1. 查看个人信息\n");
        printf("2. 修改密码\n");
        printf("3. 查询个人成绩\n");
        printf("4. 查询班级成绩排名\n");
        printf("5. 查询成绩分析\n");
        printf("6. 退出登录\n");
        printf("请选择: ");
        choice = safe_int_input();

        switch (choice) {
            case 1:
                showStudentInfo(currentUser.id);
                wait();
                break;
            case 2:
                changeStudentPassword(currentUser.id);
                wait();
                break;
            case 3:
                queryPersonalScores(currentUser.id);
                wait();
                break;
            case 4:
                queryClassRanking(currentUser.id);
                wait();
                break;
            case 5:
                analyzeStudentScores(currentUser.id);
                wait();
                break;
            case 6:
                printf("正在退出登录...\n");
                // 清理当前用户数据
                if (currentUserData != NULL) {
                    free(currentUserData);
                    currentUserData = NULL;
                }
                memset(&currentUser, 0, sizeof(User));
                wait();
                break;
            default:
                printf("无效输入，请重新选择！\n");
                wait();
        }
    } while (choice != 6);
}

// 教师菜单
void teacherMenu() {
    int choice;
    do {
        clean();
        printf("=== 教师界面 ===\n");
        printf("欢迎您，%s 老师\n", currentUser.name);
        printf("1. 查看个人信息\n");
        printf("2. 修改密码\n");
        printf("3. 学生管理\n");
        printf("4. 班级管理\n");
        printf("5. 成绩管理\n");
        printf("6. 成绩分析\n");
        printf("7. 数据导出\n");
        printf("8. 退出登录\n");
        printf("请选择: ");
        choice = safe_int_input();

        switch (choice) {
            case 1:
                showTeacherInfo(currentUser.id);
                wait();
                break;
            case 2:
                changeTeacherPassword(currentUser.id);
                wait();
                break;
            case 3:
                teacherStudentManagement(currentUser.id);
                break;
            case 4:
                teacherClassManagement(currentUser.id);
                break;
            case 5:
                teacherScoreManagement(currentUser.id);
                break;
            case 6:
                analyzeClassScores(currentUser.id);
                wait();
                break;
            case 7:
                exportStudentData(currentUser.id);
                wait();
                break;
            case 8:
                printf("正在退出登录...\n");
                // 清理当前用户数据
                if (currentUserData != NULL) {
                    free(currentUserData);
                    currentUserData = NULL;
                }
                memset(&currentUser, 0, sizeof(User));
                wait();
                break;
            default:
                printf("无效输入，请重新选择！\n");
                wait();
        }
    } while (choice != 8);
}

// 管理员菜单
void adminMenu() {
    int choice;
    do {
        clean();
        printf("=== 管理员界面 ===\n");
        printf("欢迎您，%s 管理员\n", currentUser.name);
        printf("1. 查看个人信息\n");
        printf("2. 修改密码\n");
        printf("3. 用户管理\n");
        printf("4. 切换到教师界面\n");
        printf("5. 返回登录界面\n");
        printf("请选择: ");
        choice = safe_int_input();

        switch (choice) {
            case 1:
                printf("\n=== 个人信息 ===\n");
                printf("工号: %s\n", currentUser.id);
                printf("姓名: %s\n", currentUser.name);
                printf("部门: 教务处\n");
                wait();
                break;
            case 2: {
                char old_pwd[MAX_DATA], new_pwd[MAX_DATA], confirm_pwd[MAX_DATA];
                printf("\n=== 修改密码 ===\n");
                printf("请输入原密码: ");
                safe_input_pwd(old_pwd, sizeof(old_pwd));
                
                if (strcmp(old_pwd, currentUser.pwd) != 0) {
                    printf("原密码错误！\n");
                    wait();
                    break;
                }
                
                printf("请输入新密码: ");
                safe_input_pwd(new_pwd, sizeof(new_pwd));
                printf("请确认新密码: ");
                safe_input_pwd(confirm_pwd, sizeof(confirm_pwd));
                
                if (strcmp(new_pwd, confirm_pwd) != 0) {
                    printf("两次输入的密码不一致！\n");
                    wait();
                    break;
                }
                
                if (!validatePassword(new_pwd)) {
                    printf("密码不符合要求！密码至少6位，包含字母和数字\n");
                    wait();
                    break;
                }
                
                updateUserPwd(currentUser.id, new_pwd, ROLE_ADMIN);
                strcpy(currentUser.pwd, new_pwd);
                wait();
                break;
            }
            case 3:
                adminUserManagement();
                break;
            case 4: {
                printf("切换到教师界面...\n");
                // 记录操作日志
                printf("管理员 %s 切换到教师界面\n", currentUser.name);
                
                // 清理当前用户数据
                if (currentUserData != NULL) {
                    free(currentUserData);
                    currentUserData = NULL;
                }
                
                teacherMenu();
                break;
            }
            case 5:
                printf("返回登录界面...\n");
                // 清理当前用户数据
                if (currentUserData != NULL) {
                    free(currentUserData);
                    currentUserData = NULL;
                }
                memset(&currentUser, 0, sizeof(User));
                break;
            default:
                printf("无效输入，请重新选择！\n");
                wait();
        }
    } while (choice != 5);
}

// 添加学生界面
void addStudentUI() {
    char student_id[MAX_DATA];
    char input_pwd[MAX_DATA];
    char student_name[MAX_NAME];
    char class_name[MAX_NAME];
    int choice;

    while (1) {
        clean();
        printf("=== 添加学生信息 ===\n");
        printf("请输入学号：");
        safe_input(student_id, sizeof(student_id));
        
        if (strlen(student_id) == 0) {
            printf("学号不能为空！\n");
            wait();
            continue;
        }

        // 验证学号格式
        if (!validateId(student_id, ROLE_STUDENT)) {
            printf("学号格式不正确！学生学号应以'S'开头，后跟6位数字\n");
            wait();
            continue;
        }

        if (checkUserId(student_id, ROLE_STUDENT)) {
            do {
                clean();
                printf("学号 %s 已存在！\n", student_id);
                printf("1. 修改该学号的密码\n");
                printf("2. 重新输入学号\n");
                printf("请选择：");
                choice = safe_int_input();
                
                switch (choice) {
                    case 1:
                        printf("请输入新密码：");
                        safe_input_pwd(input_pwd, sizeof(input_pwd));
                        if (!validatePassword(input_pwd)) {
                            printf("密码不符合要求！密码至少6位，包含字母和数字\n");
                            wait();
                            return;
                        }
                        updateUserPwd(student_id, input_pwd, ROLE_STUDENT);
                        return;
                    case 2:
                        break;
                    default:
                        printf("输入无效，请重新选择！\n");
                        wait();
                }
            } while (choice != 2);
        } else {
            break;
        }
    }

    printf("请输入学生姓名：");
    safe_input(student_name, sizeof(student_name));
    
    if (strlen(student_name) == 0) {
        printf("姓名不能为空！\n");
        wait();
        return;
    }

    printf("请输入班级名称：");
    safe_input(class_name, sizeof(class_name));
    
    if (strlen(class_name) == 0) {
        printf("班级名称不能为空！\n");
        wait();
        return;
    }

    printf("请输入密码：");
    safe_input_pwd(input_pwd, sizeof(input_pwd));
    
    if (!validatePassword(input_pwd)) {
        printf("密码不符合要求！密码至少6位，包含字母和数字\n");
        wait();
        return;
    }

    // 创建完整的学生数据
    Student student;
    strncpy(student.base.id, student_id, sizeof(student.base.id) - 1);
    strncpy(student.base.pwd, input_pwd, sizeof(student.base.pwd) - 1);
    strncpy(student.base.name, student_name, sizeof(student.base.name) - 1);
    student.base.role = ROLE_STUDENT;
    strcpy(student.className, class_name);
    student.scoreCount = 0;
    student.totalScore = 0.0;
    student.averageScore = 0.0;
    student.rank = 0;
    memset(student.scores, 0, sizeof(student.scores));

    addUser(student_id, input_pwd, student_name, ROLE_STUDENT, &student);
}

// 添加教师界面
void addTeacherUI() {
    char teacher_id[MAX_DATA];
    char input_pwd[MAX_DATA];
    char teacher_name[MAX_NAME];
    int choice;

    while (1) {
        clean();
        printf("=== 添加教师信息 ===\n");
        printf("请输入工号：");
        safe_input(teacher_id, sizeof(teacher_id));
        
        if (strlen(teacher_id) == 0) {
            printf("工号不能为空！\n");
            wait();
            continue;
        }

        // 验证工号格式
        if (!validateId(teacher_id, ROLE_TEACHER)) {
            printf("工号格式不正确！教师工号应以'T'开头，后跟5位数字\n");
            wait();
            continue;
        }

        if (checkUserId(teacher_id, ROLE_TEACHER)) {
            do {
                clean();
                printf("工号 %s 已存在！\n", teacher_id);
                printf("1. 修改该工号的密码\n");
                printf("2. 重新输入工号\n");
                printf("请选择：");
                choice = safe_int_input();
                
                switch (choice) {
                    case 1:
                        printf("请输入新密码：");
                        safe_input_pwd(input_pwd, sizeof(input_pwd));
                        if (!validatePassword(input_pwd)) {
                            printf("密码不符合要求！密码至少6位，包含字母和数字\n");
                            wait();
                            return;
                        }
                        updateUserPwd(teacher_id, input_pwd, ROLE_TEACHER);
                        return;
                    case 2:
                        break;
                    default:
                        printf("输入无效，请重新选择！\n");
                        wait();
                }
            } while (choice != 2);
        } else {
            break;
        }
    }

    printf("请输入教师姓名：");
    safe_input(teacher_name, sizeof(teacher_name));
    
    if (strlen(teacher_name) == 0) {
        printf("姓名不能为空！\n");
        wait();
        return;
    }

    printf("请输入密码：");
    safe_input_pwd(input_pwd, sizeof(input_pwd));
    
    if (!validatePassword(input_pwd)) {
        printf("密码不符合要求！密码至少6位，包含字母和数字\n");
        wait();
        return;
    }

    addUser(teacher_id, input_pwd, teacher_name, ROLE_TEACHER, NULL);
}

// 添加管理员界面
void addAdminUI() {
    char admin_id[MAX_DATA];
    char input_pwd[MAX_DATA];
    char admin_name[MAX_NAME];
    int choice;

    while (1) {
        clean();
        printf("=== 添加管理员信息 ===\n");
        printf("请输入工号：");
        safe_input(admin_id, sizeof(admin_id));
        
        if (strlen(admin_id) == 0) {
            printf("工号不能为空！\n");
            wait();
            continue;
        }

        // 验证管理员工号格式
        if (!validateId(admin_id, ROLE_ADMIN)) {
            printf("管理员工号格式不正确！管理员工号应以'A'开头，后跟4位数字\n");
            wait();
            continue;
        }

        if (checkUserId(admin_id, ROLE_ADMIN)) {
            do {
                clean();
                printf("工号 %s 已存在！\n", admin_id);
                printf("1. 修改该工号的密码\n");
                printf("2. 重新输入工号\n");
                printf("请选择：");
                choice = safe_int_input();
                
                switch (choice) {
                    case 1:
                        printf("请输入新密码：");
                        safe_input_pwd(input_pwd, sizeof(input_pwd));
                        if (!validatePassword(input_pwd)) {
                            printf("密码不符合要求！密码至少6位，包含字母和数字\n");
                            wait();
                            return;
                        }
                        updateUserPwd(admin_id, input_pwd, ROLE_ADMIN);
                        return;
                    case 2:
                        break;
                    default:
                        printf("输入无效，请重新选择！\n");
                        wait();
                }
            } while (choice != 2);
        } else {
            break;
        }
    }

    printf("请输入管理员姓名：");
    safe_input(admin_name, sizeof(admin_name));
    
    if (strlen(admin_name) == 0) {
        printf("姓名不能为空！\n");
        wait();
        return;
    }

    printf("请输入密码：");
    safe_input_pwd(input_pwd, sizeof(input_pwd));
    
    if (!validatePassword(input_pwd)) {
        printf("密码不符合要求！密码至少6位，包含字母和数字\n");
        wait();
        return;
    }

    addUser(admin_id, input_pwd, admin_name, ROLE_ADMIN, NULL);
}

// 列出所有学生
void listStudents() {
    FILE* fp = fopen("students.bin", "rb");
    if (fp == NULL) {
        printf("暂无学生数据\n");
        return;
    }

    Student student;
    printf("\n=== 学生列表 ===\n");
    printf("%-15s %-20s\n", "学号", "姓名");
    printf("-------------------------------\n");

    while (fread(&student, sizeof(Student), 1, fp) == 1) {
        printf("%-15s %-20s\n", student.base.id, student.base.name);
    }

    fclose(fp);
}

// 列出所有用户
void listAllUsers() {
    int choice;
    do {
        clean();
        printf("=== 用户列表 ===\n");
        printf("1. 查看学生列表\n");
        printf("2. 查看教师列表\n");
        printf("3. 查看管理员列表\n");
        printf("4. 返回上一级\n");
        printf("请选择: ");
        choice = safe_int_input();

        switch (choice) {
            case 1:
                listUsersByRole(ROLE_STUDENT);
                wait();
                break;
            case 2:
                listUsersByRole(ROLE_TEACHER);
                wait();
                break;
            case 3:
                listUsersByRole(ROLE_ADMIN);
                wait();
                break;
            case 4:
                printf("返回上一级...\n");
                break;
            default:
                printf("无效输入，请重新选择！\n");
                wait();
        }
    } while (choice != 4);
}

// 按角色列出用户
void listUsersByRole(UserRole role) {
    char filename[20];
    char roleName[20];
    
    switch (role) {
        case ROLE_STUDENT: 
            strcpy(filename, "students.bin");
            strcpy(roleName, "学生");
            break;
        case ROLE_TEACHER: 
            strcpy(filename, "teachers.bin");
            strcpy(roleName, "教师");
            break;
        case ROLE_ADMIN: 
            strcpy(filename, "admins.bin");
            strcpy(roleName, "管理员");
            break;
        default: return;
    }

    FILE* fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("暂无%s数据\n", roleName);
        return;
    }

    User user;
    size_t size = (role == ROLE_STUDENT) ? sizeof(Student) : 
                  (role == ROLE_TEACHER) ? sizeof(Teacher) : sizeof(Admin);

    printf("\n=== %s列表 ===\n", roleName);
    printf("%-15s %-20s\n", "ID", "姓名");
    printf("-------------------------------\n");

    while (fread(&user, sizeof(User), 1, fp) == 1) {
        printf("%-15s %-20s\n", user.id, user.name);
        // 跳过额外数据
        fseek(fp, size - sizeof(User), SEEK_CUR);
    }

    fclose(fp);
}

// 学生登录
void studentLogin() {
    clean();
    char student_id[MAX_DATA];
    char input_pwd[MAX_DATA];

    printf("=== 学生登录 ===\n");
    printf("请输入学号：");
    safe_input(student_id, sizeof(student_id));
    
    if (strlen(student_id) == 0) {
        printf("学号不能为空！\n");
        wait();
        return;
    }

    printf("请输入密码：");
    safe_input_pwd(input_pwd, sizeof(input_pwd));

    if (loginUser(student_id, input_pwd, ROLE_STUDENT)) {
        clean();
        printf("登录成功！\n");
        studentMenu();
    } else {
        wait();
    }
}

// 教师登录
void teacherLogin() {
    clean();
    char teacher_id[MAX_DATA];
    char input_pwd[MAX_DATA];

    printf("=== 教师登录 ===\n");
    printf("请输入工号：");
    safe_input(teacher_id, sizeof(teacher_id));
    
    if (strlen(teacher_id) == 0) {
        printf("工号不能为空！\n");
        wait();
        return;
    }

    printf("请输入密码：");
    safe_input_pwd(input_pwd, sizeof(input_pwd));

    if (loginUser(teacher_id, input_pwd, ROLE_TEACHER)) {
        clean();
        printf("登录成功！\n");
        teacherMenu();
    } else {
        wait();
    }
}

// 管理员登录
void adminLogin() {
    clean();
    char admin_id[MAX_DATA];
    char input_pwd[MAX_DATA];

    printf("=== 管理员登录 ===\n");
    printf("请输入工号：");
    safe_input(admin_id, sizeof(admin_id));
    
    if (strlen(admin_id) == 0) {
        printf("工号不能为空！\n");
        wait();
        return;
    }

    printf("请输入密码：");
    safe_input_pwd(input_pwd, sizeof(input_pwd));

    if (loginUser(admin_id, input_pwd, ROLE_ADMIN)) {
        clean();
        printf("登录成功！\n");
        adminMenu();
    } else {
        wait();
    }
}

// 初始化默认管理员
void initAdmin() {
    // 检查是否已存在管理员
    FILE* fp = fopen("admins.bin", "rb");
    if (fp != NULL) {
        fseek(fp, 0, SEEK_END);
        long fileSize = ftell(fp);
        fclose(fp);
        
        if (fileSize > 0) {
            return; // 已有管理员数据
        }
    }

    // 创建默认管理员
    Admin defaultAdmin;
    strncpy(defaultAdmin.base.id, "admin", sizeof(defaultAdmin.base.id) - 1);
    strncpy(defaultAdmin.base.pwd, "123456", sizeof(defaultAdmin.base.pwd) - 1);
    strncpy(defaultAdmin.base.name, "系统管理员", sizeof(defaultAdmin.base.name) - 1);
    defaultAdmin.base.role = ROLE_ADMIN;
    strcpy(defaultAdmin.department, "教务处");

    fp = fopen("admins.bin", "wb");
    if (fp == NULL) {
        printf("创建默认管理员失败！\n");
        return;
    }

    fwrite(&defaultAdmin, sizeof(Admin), 1, fp);
    fclose(fp);
    
    printf("默认管理员已创建\n");
    printf("账号: admin\n");
    printf("密码: 123456\n");
    wait();
}

// 加载文件
void loadFile() {
    // 检查并创建必要的文件
    loadUserFile("students.bin");
    loadUserFile("teachers.bin");
    loadUserFile("admins.bin");
    
    // 初始化默认管理员
    initAdmin();
    
    printf("系统初始化完成\n");
}

// 加载用户文件
void loadUserFile(const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (fp == NULL) {
        // 文件不存在，创建空文件
        fp = fopen(filename, "wb");
        if (fp == NULL) {
            printf("创建文件 %s 失败！\n", filename);
            return;
        }
        fclose(fp);
    } else {
        fclose(fp);
    }
}

// 清理函数 - 修复：添加内存清理
void cleanup() {
    printf("正在关闭系统...\n");
    
    // 清理当前用户数据
    if (currentUserData != NULL) {
        free(currentUserData);
        currentUserData = NULL;
    }
    
    // 清理动态分配的数组
    if (students != NULL) {
        free(students);
        students = NULL;
    }
    if (teachers != NULL) {
        free(teachers);
        teachers = NULL;
    }
    if (admins != NULL) {
        free(admins);
        admins = NULL;
    }
    if (classes != NULL) {
        free(classes);
        classes = NULL;
    }
    
    printf("系统已安全关闭\n");
    wait();
}

// 删除用户函数 - 修复：增强错误处理
int deleteUser(const char* id, UserRole role) {
    char filename[20];
    char tempFilename[30];
    char backupFilename[30];
    
    switch (role) {
        case ROLE_STUDENT: 
            strcpy(filename, "students.bin"); 
            strcpy(tempFilename, "students_temp.bin");
            strcpy(backupFilename, "students_backup.bin");
            break;
        case ROLE_TEACHER: 
            strcpy(filename, "teachers.bin"); 
            strcpy(tempFilename, "teachers_temp.bin");
            strcpy(backupFilename, "teachers_backup.bin");
            break;
        case ROLE_ADMIN: 
            strcpy(filename, "admins.bin"); 
            strcpy(tempFilename, "admins_temp.bin");
            strcpy(backupFilename, "admins_backup.bin");
            break;
        default: return 0;
    }

    // 创建备份文件
    if (!CopyFile(filename, backupFilename, TRUE)) {
        printf("创建备份文件失败，操作取消！\n");
        return 0;
    }

    FILE* fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("文件不存在！\n");
        DeleteFile(backupFilename); // 删除备份文件
        return 0;
    }

    FILE* tempFp = fopen(tempFilename, "wb");
    if (tempFp == NULL) {
        printf("创建临时文件失败！\n");
        fclose(fp);
        DeleteFile(backupFilename); // 删除备份文件
        return 0;
    }

    User user;
    size_t size = (role == ROLE_STUDENT) ? sizeof(Student) : 
                  (role == ROLE_TEACHER) ? sizeof(Teacher) : sizeof(Admin);
    char buffer[size];
    int found = 0;

    while (fread(buffer, size, 1, fp) == 1) {
        memcpy(&user, buffer, sizeof(User));
        if (strcmp(user.id, id) != 0) {
            fwrite(buffer, size, 1, tempFp);
        } else {
            found = 1;
        }
    }

    fclose(fp);
    fclose(tempFp);

    if (!found) {
        DeleteFile(tempFilename);
        DeleteFile(backupFilename); // 删除备份文件
        printf("未找到用户 %s！\n", id);
        return 0;
    }

    // 关闭文件句柄后再进行文件操作
    if (remove(filename) != 0) {
        printf("删除原文件失败！正在恢复备份...\n");
        DeleteFile(tempFilename);
        CopyFile(backupFilename, filename, TRUE);
        DeleteFile(backupFilename);
        return 0;
    }

    if (rename(tempFilename, filename) != 0) {
        printf("重命名文件失败！正在恢复备份...\n");
        CopyFile(backupFilename, filename, TRUE);
        DeleteFile(tempFilename);
        DeleteFile(backupFilename);
        return 0;
    }

    // 删除备份文件
    DeleteFile(backupFilename);
    
    printf("用户 %s 删除成功！\n", id);
    return 1;
}

// 删除用户界面
void deleteUserUI() {
    int role;
    char id[MAX_DATA];
    char confirm[10];

    clean();
    printf("=== 删除用户 ===\n");
    printf("请选择用户类型：\n");
    printf("1. 学生\n");
    printf("2. 教师\n");
    printf("3. 管理员\n");
    printf("请选择: ");
    role = safe_int_input();

    if (role < 1 || role > 3) {
        printf("无效的用户类型！\n");
        return;
    }

    printf("请输入要删除的用户ID：");
    safe_input(id, sizeof(id));

    if (strlen(id) == 0) {
        printf("ID不能为空！\n");
        return;
    }

    // 验证ID格式
    if (!validateId(id, role)) {
        printf("ID格式不正确！\n");
        return;
    }

    // 检查用户是否存在
    if (!checkUserId(id, role)) {
        printf("用户 %s 不存在！\n", id);
        return;
    }

    // 特殊检查：不能删除默认管理员
    if (role == ROLE_ADMIN && strcmp(id, "admin") == 0) {
        printf("不能删除默认管理员！\n");
        return;
    }

    // 二次确认
    printf("确定要删除用户 %s 吗？(yes/no): ", id);
    safe_input(confirm, sizeof(confirm));

    if (strcmp(confirm, "yes") == 0 || strcmp(confirm, "YES") == 0) {
        if (deleteUser(id, role)) {
            printf("删除成功！\n");
        } else {
            printf("删除失败！\n");
        }
    } else {
        printf("已取消删除操作！\n");
    }
}

// 搜索用户函数
void searchUser() {
    int role;
    char id[MAX_DATA];

    clean();
    printf("=== 搜索用户 ===\n");
    printf("请选择用户类型：\n");
    printf("1. 学生\n");
    printf("2. 教师\n");
    printf("3. 管理员\n");
    printf("请选择: ");
    role = safe_int_input();

    if (role < 1 || role > 3) {
        printf("无效的用户类型！\n");
        return;
    }

    printf("请输入要搜索的用户ID：");
    safe_input(id, sizeof(id));

    if (strlen(id) == 0) {
        printf("ID不能为空！\n");
        return;
    }

    if (checkUserId(id, role)) {
        printf("找到用户：%s\n", id);
        
        // 显示用户详细信息
        char filename[20];
        switch (role) {
            case ROLE_STUDENT: strcpy(filename, "students.bin"); break;
            case ROLE_TEACHER: strcpy(filename, "teachers.bin"); break;
            case ROLE_ADMIN: strcpy(filename, "admins.bin"); break;
        }

        FILE* fp = fopen(filename, "rb");
        if (fp != NULL) {
            User user;
            size_t size = (role == ROLE_STUDENT) ? sizeof(Student) : 
                          (role == ROLE_TEACHER) ? sizeof(Teacher) : sizeof(Admin);

            while (fread(&user, sizeof(User), 1, fp) == 1) {
                if (strcmp(user.id, id) == 0) {
                    printf("姓名: %s\n", user.name);
                    printf("角色: %s\n", role == ROLE_STUDENT ? "学生" : role == ROLE_TEACHER ? "教师" : "管理员");
                    break;
                }
                fseek(fp, size - sizeof(User), SEEK_CUR);
            }
            fclose(fp);
        }
    } else {
        printf("未找到用户：%s\n", id);
    }
}

// 数据备份函数 - 修复：增强错误处理
void backupData() {
    char backupDir[MAX_PATH];
    char sourceFile[MAX_PATH];
    char destFile[MAX_PATH];
    SYSTEMTIME st;
    
    GetLocalTime(&st);
    sprintf(backupDir, "backup_%04d%02d%02d_%02d%02d%02d", 
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

    // 创建备份目录
    if (CreateDirectory(backupDir, NULL) || GetLastError() == ERROR_ALREADY_EXISTS) {
        printf("正在备份数据到目录：%s\n", backupDir);
        
        // 备份各个数据文件
        const char* files[] = {"students.bin", "teachers.bin", "admins.bin"};
        int successCount = 0;
        
        for (int i = 0; i < sizeof(files)/sizeof(files[0]); i++) {
            sprintf(sourceFile, "%s", files[i]);
            sprintf(destFile, "%s\\%s", backupDir, files[i]);
            
            if (CopyFile(sourceFile, destFile, FALSE)) {
                printf("✓ %s 备份成功\n", files[i]);
                successCount++;
            } else {
                printf("✗ %s 备份失败，错误码：%d\n", files[i], GetLastError());
            }
        }
        
        if (successCount == sizeof(files)/sizeof(files[0])) {
            printf("数据备份成功！备份目录：%s\n", backupDir);
        } else {
            printf("部分文件备份失败，请检查错误信息\n");
        }
    } else {
        printf("创建备份目录失败，错误码：%d\n", GetLastError());
    }
}

// 数据恢复函数 - 修复：增强错误处理并修复switch-case编译错误
void restoreData() {
    char backupDir[MAX_PATH];
    char sourceFile[MAX_PATH];
    char targetFile[MAX_PATH];
    int choice;

    clean();
    printf("=== 数据恢复 ===\n");
    printf("请输入备份目录名称：");
    safe_input(backupDir, sizeof(backupDir));

    // 检查备份目录是否存在
    if (GetFileAttributes(backupDir) == INVALID_FILE_ATTRIBUTES) {
        printf("备份目录不存在！\n");
        return;
    }

    // 显示恢复选项
    printf("\n请选择要恢复的数据：\n");
    printf("1. 恢复所有数据\n");
    printf("2. 仅恢复学生数据\n");
    printf("3. 仅恢复教师数据\n");
    printf("4. 仅恢复管理员数据\n");
    printf("请选择: ");
    choice = safe_int_input();

    // 二次确认
    printf("\n警告：恢复操作将覆盖当前数据！确定要继续吗？(yes/no): ");
    char confirm[10];
    safe_input(confirm, sizeof(confirm));
    
    if (strcmp(confirm, "yes") != 0 && strcmp(confirm, "YES") != 0) {
        printf("已取消恢复操作！\n");
        return;
    }

    int successCount = 0;
    
    switch (choice) {
        case 1: // 恢复所有数据
            ; // 修复：添加空语句解决case标签后直接跟变量声明的问题
            const char* allFiles[] = {"students.bin", "teachers.bin", "admins.bin"};
            for (int i = 0; i < sizeof(allFiles)/sizeof(allFiles[0]); i++) {
                sprintf(sourceFile, "%s\\%s", backupDir, allFiles[i]);
                sprintf(targetFile, "%s", allFiles[i]);
                
                if (CopyFile(sourceFile, targetFile, FALSE)) {
                    printf("✓ %s 恢复成功\n", allFiles[i]);
                    successCount++;
                } else {
                    printf("✗ %s 恢复失败，错误码：%d\n", allFiles[i], GetLastError());
                }
            }
            break;
            
        case 2: // 仅恢复学生数据
            sprintf(sourceFile, "%s\\students.bin", backupDir);
            sprintf(targetFile, "students.bin");
            if (CopyFile(sourceFile, targetFile, FALSE)) {
                printf("✓ 学生数据恢复成功\n");
                successCount++;
            } else {
                printf("✗ 学生数据恢复失败，错误码：%d\n", GetLastError());
            }
            break;
            
        case 3: // 仅恢复教师数据
            sprintf(sourceFile, "%s\\teachers.bin", backupDir);
            sprintf(targetFile, "teachers.bin");
            if (CopyFile(sourceFile, targetFile, FALSE)) {
                printf("✓ 教师数据恢复成功\n");
                successCount++;
            } else {
                printf("✗ 教师数据恢复失败，错误码：%d\n", GetLastError());
            }
            break;
            
        case 4: // 仅恢复管理员数据
            sprintf(sourceFile, "%s\\admins.bin", backupDir);
            sprintf(targetFile, "admins.bin");
            if (CopyFile(sourceFile, targetFile, FALSE)) {
                printf("✓ 管理员数据恢复成功\n");
                successCount++;
            } else {
                printf("✗ 管理员数据恢复失败，错误码：%d\n", GetLastError());
            }
            break;
            
        default:
            printf("无效的选择！\n");
            return;
    }
    
    if (successCount > 0) {
        printf("\n恢复完成！请重新启动系统以加载恢复的数据。\n");
    } else {
        printf("\n恢复失败，请检查错误信息。\n");
    }
}

// ID验证函数
int validateId(const char* id, UserRole role) {
    int len = strlen(id);
    
    switch (role) {
        case ROLE_STUDENT:
            // 学生学号格式：S开头，后跟6位数字
            if (len != 7 || id[0] != 'S') return 0;
            for (int i = 1; i < len; i++) {
                if (!isdigit(id[i])) return 0;
            }
            return 1;
            
        case ROLE_TEACHER:
            // 教师工号格式：T开头，后跟5位数字
            if (len != 6 || id[0] != 'T') return 0;
            for (int i = 1; i < len; i++) {
                if (!isdigit(id[i])) return 0;
            }
            return 1;
            
        case ROLE_ADMIN:
            // 管理员工号格式：A开头，后跟4位数字，或者是默认管理员admin
            if (strcmp(id, "admin") == 0) return 1;
            if (len != 5 || id[0] != 'A') return 0;
            for (int i = 1; i < len; i++) {
                if (!isdigit(id[i])) return 0;
            }
            return 1;
            
        default:
            return 0;
    }
}

// 密码验证函数
int validatePassword(const char* pwd) {
    int len = strlen(pwd);
    int hasLetter = 0, hasDigit = 0;
    
    // 密码长度至少6位
    if (len < 6) return 0;
    
    // 检查是否包含字母和数字
    for (int i = 0; i < len; i++) {
        if (isalpha(pwd[i])) hasLetter = 1;
        if (isdigit(pwd[i])) hasDigit = 1;
        if (hasLetter && hasDigit) return 1;
    }
    
    return 0;
}

// 帮助信息函数
void showHelp() {
    clean();
    printf("=== 系统帮助 ===\n\n");
    printf("学生管理系统使用说明：\n\n");
    
    printf("1. 用户登录：\n");
    printf("   - 学生登录：使用学号（S开头+6位数字）和密码登录\n");
    printf("   - 教师登录：使用工号（T开头+5位数字）和密码登录\n");
    printf("   - 管理员登录：使用工号（A开头+4位数字）或默认账号admin登录\n\n");
    
    printf("2. 功能说明：\n");
    printf("   - 学生：查看个人信息、修改密码、查看成绩\n");
    printf("   - 教师：查看个人信息、修改密码、管理学生、管理成绩\n");
    printf("   - 管理员：所有功能，包括用户管理、数据备份恢复\n\n");
    
    printf("3. 数据备份：\n");
    printf("   - 系统会在当前目录创建备份文件夹，包含所有数据文件\n");
    printf("   - 备份文件名格式：backup_年月日_时分秒\n\n");
    
    printf("4. 数据恢复：\n");
    printf("   - 选择备份目录进行数据恢复\n");
    printf("   - 可以选择恢复部分或全部数据\n\n");
    
    printf("5. 注意事项：\n");
    printf("   - 密码至少6位，必须包含字母和数字\n");
    printf("   - 不能删除默认管理员账号\n");
    printf("   - 定期备份数据以防止数据丢失\n\n");
    
    printf("按任意键返回...\n");
    getch();
}

// 登录界面
void loginScreen() {
    int role;
    do {
        clean();
        printf("=== 登录界面 ===\n");
        printf("请选择用户类型：\n");
        printf("1. 学生\n");
        printf("2. 教师\n");
        printf("3. 管理员\n");
        printf("4. 返回主菜单\n");
        printf("请选择: ");
        role = safe_int_input();

        switch (role) {
            case 1:
                studentLogin();
                break;
            case 2:
                teacherLogin();
                break;
            case 3:
                adminLogin();
                break;
            case 4:
                printf("返回主菜单...\n");
                break;
            default:
                printf("无效输入，请重新选择！\n");
                wait();
        }
    } while (role != 4);
}

// 主菜单
void mainMenu() {
    int choice;
    do {
        clean();
        printf("=== 学生管理系统 ===\n");
        printf("1. 用户登录\n");
        printf("2. 数据备份\n");
        printf("3. 数据恢复\n");
        printf("4. 帮助信息\n");
        printf("5. 退出系统\n");
        printf("请选择: ");
        choice = safe_int_input();

        switch (choice) {
            case 1:
                loginScreen();
                break;
            case 2:
                backupData();
                wait();
                break;
            case 3:
                restoreData();
                wait();
                break;
            case 4:
                showHelp();
                break;
            case 5:
                printf("感谢使用学生管理系统！\n");
                cleanup();
                break;
            default:
                printf("无效输入，请重新选择！\n");
                wait();
        }
    } while (choice != 5);
}

// 主函数
int main() {
    // 设置控制台编码为UTF-8（仅Windows）
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    
    // 初始化系统
    loadFile();
    
    // 显示主菜单
    mainMenu();
    
    return 0;
}
