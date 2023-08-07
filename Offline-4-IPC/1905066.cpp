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
#define NUMBER_OF_STAFF 2
#define STAFF_READ_INTERVAL 4
#define STUEDENT_ARRIVAL_TIME_MEAN 5
#define STAFF_ARRIVAL_TIME_MEAN 5
#define FREE 0
#define BUSY 1

int number_of_students, number_of_groups, students_per_group;
int print_time, bind_time, submit_time;
int printer_states[NUMBER_OF_PRINTERS + 1];
Semaphore printer_mutexes[NUMBER_OF_PRINTERS + 1];
Semaphore bind_station(2);
Semaphore bind_mutex;
Semaphore submission_access_mutex;
Semaphore submission_read_mutex;
int submission_count = 0;
int reader_count = 0;

struct Student
{
    int id;
    int group_id;
    int printer_id;
    int print_state;

    Student(){}
    Student(int id){
        this->id = id;
        group_id = (id - 1) / students_per_group + 1;
        printer_id = id % NUMBER_OF_PRINTERS + 1;
        print_state = NOT_ARRIVED;
    }
};

vector<Student> students;
vector<pthread_t> threads;
vector<pthread_t> staff_threads;
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
        for (int i = 1; i <= number_of_students; i++)
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
    sleep(get_random_number(STUEDENT_ARRIVAL_TIME_MEAN));
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

void submit_entry(int group_id){
    submission_access_mutex.down();
    SYNCHRONIZED_PRINT("Group " << group_id<<" has stated filling submission entry");
    sleep(submit_time);
    submission_count++;
    SYNCHRONIZED_PRINT("Group "<<group_id<<" has submitted the report.");
    submission_access_mutex.up();
}

int read_submission_entry(int staff_id){
    int data;
    submission_read_mutex.down();
    reader_count++;
    if(reader_count == 1) submission_access_mutex.down();
    submission_read_mutex.up();
    data = submission_count;
    SYNCHRONIZED_PRINT("Staff "<< staff_id<< " has started reading the entry book. No. of submission = "<<data);
    submission_read_mutex.down();
    reader_count--;
    if(reader_count == 0) submission_access_mutex.up();
    submission_read_mutex.up();
    if(data == number_of_groups) pthread_exit(NULL);
}

void *student_routine(void *arg)
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

    // submit the report
    submit_entry(group_id);

    pthread_exit(NULL);    
}

void *staff_routine(void *arg){
    int staff_id = (long) arg;
    sleep(get_random_number(STAFF_ARRIVAL_TIME_MEAN));
    while(true){
        sleep(STAFF_READ_INTERVAL);
        read_submission_entry(staff_id);
    }
}


int main()
{

    freopen("input.txt", "r", stdin);
    freopen("output.txt", "w", stdout);

    cin >> number_of_students >> students_per_group;
    cin >> print_time >> bind_time >> submit_time;
    number_of_groups = number_of_students / students_per_group;

    students.resize(number_of_students + 1);
    threads.resize(number_of_students);
    staff_threads.resize(NUMBER_OF_STAFF);
    printer_s.resize(number_of_students + 1);
    
    for (int i = 1; i <= NUMBER_OF_PRINTERS; i++) printer_states[i] = FREE;
    for (int i = 0; i < printer_s.size(); i++) printer_s[i].down();
    for(int id = 1; id<=number_of_students; id++) students[id] = Student(id);

    begin_time = time(0);

    // student threads
    for (int i = 1; i <= number_of_students; i++)
    {
        Student *s = &students[i];
        pthread_create(&threads[i - 1], NULL, student_routine, (void *)s);
    }

    // staff threads
    for(int i=0; i<NUMBER_OF_STAFF; i++) {
        long staff_id = i+1;
        pthread_create(&staff_threads[i], NULL, staff_routine, (void*)staff_id);
    }

    pthread_exit(NULL);
    return 0;
}
