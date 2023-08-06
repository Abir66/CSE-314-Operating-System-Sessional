#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <queue>
#include <vector>
#include "helper.h"

using namespace std;

#define NOT_ARRIVED 0
#define WAITING_FOR_PRINTER 1
#define PRINTING 2
#define PRINTING_FINISHED 3
#define NUMBER_OF_PRINTERS 4
#define NUMBER_OF_BINDERS 2
#define ARRIVAL_TIME_MEAN 5
#define FREE 0
#define BUSY 1

int no_of_students, no_of_groups, students_per_group;
int print_time, bind_time, submit_time;
int printer_states[NUMBER_OF_PRINTERS + 1];
Semaphore printer_mutexes[NUMBER_OF_PRINTERS + 1];
Semaphore bind_station(2);
Semaphore bind_mutex;

struct Student
{
    int id;
    int group_id;
    int printer_id;
    int print_state;
};

vector<Student> students;
vector<pthread_t> threads;
vector<Semaphore> printer_s;

bool test_printer(int student_id, int printer_id)
{
    if (students[student_id].print_state == WAITING_FOR_PRINTER && printer_states[printer_id] == FREE)
    {
        students[student_id].print_state = PRINTING;
        printer_states[printer_id] = BUSY;
        printer_s[student_id].up();
        return true;
    }
    return false;
}

void get_printer(int student_id)
{
    int printer_id = students[student_id].printer_id;
    printer_mutexes[printer_id].down();
    students[student_id].print_state = WAITING_FOR_PRINTER;
    auto res = test_printer(student_id, printer_id);
    printer_mutexes[printer_id].up();
    printer_s[student_id].down();
}

void give_up_printer(int student_id)
{
    int printer_id = students[student_id].printer_id;

    printer_mutexes[printer_id].down();
    printer_states[printer_id] = FREE;
    students[student_id].print_state = PRINTING_FINISHED;

    int group_id = students[student_id].group_id;
    int group_start = (group_id - 1) * students_per_group + 1;
    bool found = false;
    for (int i = group_start; i < group_start + students_per_group; i = i + NUMBER_OF_PRINTERS)
    {
        if (i == student_id || students[i].printer_id != printer_id) continue;
        found = test_printer(i, printer_id);
        if (found) break;
    }

    if (!found)
    {
        for (int i = 1; i <= no_of_students; i++)
        {
            if (students[i].printer_id != printer_id || i == student_id || students[i].group_id == group_id) continue;
            found = test_printer(i, printer_id);
            if (found) break;
        }
    }

    printer_mutexes[printer_id].up();
}

void do_print(int student_id)
{
    int printer_id = students[student_id].printer_id;
    sleep(get_random_number(ARRIVAL_TIME_MEAN));
    SYNCHRONIZED_PRINT("Student " << student_id << " has arrived at printing station : " << printer_id);

    // get the assigned printer
    get_printer(student_id);
    SYNCHRONIZED_PRINT("Student " << student_id << " has started printing at printing station : " << printer_id);

    // print
    sleep(print_time);
    SYNCHRONIZED_PRINT("Student " << student_id << " has finished printing at printing station : " << printer_id);

    // give up the printer
    give_up_printer(student_id);
}

void bind(int student_id){
    int group_id = students[student_id].group_id;
    bind_station.down();
    SYNCHRONIZED_PRINT("Group " << group_id << " has started binding");
    sleep(bind_time);
    SYNCHRONIZED_PRINT("Group " << group_id << " has finished binding");
    bind_station.up();
}

void *func(void *arg)
{
    Student *s = (Student *)arg;
    
    // print the assignment
    do_print(s->id);

    // if not the group leader, exit
    if(s->id % students_per_group != 0) pthread_exit(NULL);

    // if group leader, wait for others to finish
    int group_id = s->group_id;
    int group_start = (group_id - 1) * students_per_group + 1;
    for (int i = group_start; i < group_start + students_per_group - 1; i++) pthread_join(threads[i - 1], NULL);
    SYNCHRONIZED_PRINT("Group " << group_id << " has finished printing");

    // bind the assignment
    bind(s->id);

    
}

int main()
{

    freopen("input.txt", "r", stdin);
    // freopen("output.txt", "w", stdout);

    cin >> no_of_students >> no_of_groups;
    cin >> print_time >> bind_time >> submit_time;
    students_per_group = no_of_students / no_of_groups;

    Student s;
    students.push_back(s);

    for (int id = 1, group_id = 1; id <= no_of_students; id++)
    {
        Student student;
        student.id = id;
        student.group_id = group_id;
        student.printer_id = id % NUMBER_OF_PRINTERS + 1;
        student.print_state = NOT_ARRIVED;
        students.push_back(student);
        if (id % students_per_group == 0) group_id++;
    }

    for (int i = 1; i <= NUMBER_OF_PRINTERS; i++) printer_states[i] = FREE;

    threads.resize(no_of_students);
    printer_s.resize(no_of_students + 1);
    for (int i = 0; i < printer_s.size(); i++) printer_s[i].down();

    begin_time = time(0);

    for (int i = 1; i <= no_of_students; i++)
    {
        Student *s = &students[i];
        pthread_create(&threads[i - 1], NULL, func, (void *)s);
    }

    pthread_exit(NULL);
    return 0;
}
