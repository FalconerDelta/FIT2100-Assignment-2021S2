// Name: Conrad Lee
// Student ID: 29255554
// Created At: 2021/10/1
// Last Modified At: 2021/10/6
// Description: Implementation of Preemptive Earliest Deadline First Algorithm

#include <stdio.h>
#include <string.h>

#define MAX_PROC 100
#define NAME_LEN 11
#define OUTPUT_FILE "results-3.txt"

/* Special enumerated data type for process state */
typedef enum {
    READY, RUNNING, EXIT
} process_state_t;

/* C data structure used as process control block. The scheduler
 * should create one instance per running process in the system.
 */
typedef struct {
    char process_name[11]; // A string that identifies the process

    /* Times are measured in seconds. */
    int entryTime; // The time process entered system
    int serviceTime; // The total CPU time required by the process
    int remainingTime; // Remaining service time until completion
    int deadline; // Absolute deadline of the process
    int executeTime; // The time process start execution for the first time

    process_state_t state; // current process state (e.g. READY)
} pcb_t;

/**
 * EDF (Earliest Deadline First) scheduler struct
 */
typedef struct {
    pcb_t procs[MAX_PROC]; // process array
    int proc_num; // number of READY or RUNNING processes
    int current_time; // time from simulation in seconds
    int running_proc; // running process's index in procs
} scheduler_t;

/**
 * Remove process with given index from process array
 * @param scheduler pointer to scheduler_t
 * @param index index of process to remove
 */
void remove_process(scheduler_t *scheduler, int index) {
    if (index < 0 || index >= scheduler->proc_num) return;
    for (int i = index; i < scheduler->proc_num - 1; i++) {
        scheduler->procs[i] = scheduler->procs[i + 1];
    }
    scheduler->proc_num--;
}

/**
 * Log simulation spec of process with given index to results
 * @param scheduler pointer to scheduler_t
 * @param fp  pointer to results FILE
 * @param index index of process to log
 */
void log_spec(scheduler_t *scheduler, FILE *fp, int index) {
    pcb_t *proc = &scheduler->procs[index];
    int turnaround_time = scheduler->current_time - proc->entryTime; // Time interval from entry to completion
    int wait_time = proc->executeTime - proc->entryTime; // Time interval from entry to execution
    int deadline_met = turnaround_time <= proc->deadline - proc->entryTime; // If this process met its deadline
    fprintf(fp, "%s %d %d %d\n", proc->process_name, wait_time, turnaround_time, deadline_met);
}


/**
 * Switch two processes, if i is -1, just run process j, if j is -1, just terminate process i,
 * if both i and j are not -1, then stop process i and run process j
 * @param scheduler pointer to scheduler_t
 * @param fp  pointer to results FILE
 * @param i index of process to stop
 * @param j index of process to run
 */
void switch_process(scheduler_t *scheduler, FILE *fp, int i, int j) {
    if (i == -1 && j == -1) return;
    if (j == -1) {
        // just terminate process i
        pcb_t *proc = &scheduler->procs[i];
        proc->state = EXIT;
        log_spec(scheduler, fp, i);
        printf("Time %d: %s has finished execution.\n", scheduler->current_time, proc->process_name);
        scheduler->running_proc = -1;
        remove_process(scheduler, i);
    } else {
        if (i != -1) {
            // stop process i
            scheduler->procs[i].state = READY;
            scheduler->running_proc = -1;
        }
        // run process j
        pcb_t *proc = &scheduler->procs[j];
        // set executeTime when the process executes for the first time
        if (proc->executeTime == -1) {
            proc->executeTime = scheduler->current_time;
        }
        scheduler->running_proc = j;
        proc->state = RUNNING;
        printf("Time %d: %s is in the running state.\n", scheduler->current_time, proc->process_name);
    }
}

/**
 * Accepting arriving processes by reading process data line by line
 * @param scheduler pointer to scheduler_t
 * @param fp pointer to FILE with process data
 * @return return EOF(-1) is process data is end
 */
int accept_arriving_processes(scheduler_t *scheduler, FILE *fp) {
    int flag = 0;
    char proc_name[NAME_LEN];
    int arrive_time;
    int service_time;
    int deadline;
    long position = ftell(fp); // record position at fp before start fscanf
    while ((flag = fscanf(fp, "%s %d %d %d", proc_name, &arrive_time, &service_time, &deadline)) != EOF) {
        if (arrive_time == scheduler->current_time) {
            pcb_t *proc = &scheduler->procs[scheduler->proc_num];
            strcpy(proc->process_name, proc_name);
            proc->entryTime = arrive_time;
            proc->serviceTime = service_time;
            proc->remainingTime = service_time;
            proc->deadline = deadline + arrive_time;
            proc->state = READY;
            proc->executeTime = -1;
            scheduler->proc_num++;
            printf("Time %d: %s has entered the system.\n", scheduler->current_time, proc_name);
            position = ftell(fp); // record position at fp after each fscanf
        } else {
            // go back to last line if arrive time of process is greater than current time
            fseek(fp, position, SEEK_SET);
            break;
        }
    }
    return flag;
}

/**
 * Get index of READY process with earliest deadline and its deadline
 * @param scheduler pointer to scheduler_t
 * @param index [return value] pointer to index of process with earliest deadline,
 * -1 if there's no process with earlier deadline time than currently running process
 * @param earliest_deadline [return value] pointer to earliest deadline
 */
void get_queue_head(scheduler_t *scheduler, int *index, int *earliest_deadline) {
    *index = -1;
    if (scheduler->proc_num == 0) return;
    // if there's running process, set earliest_deadline as its deadline
    if (scheduler->running_proc != -1) {
        *earliest_deadline = scheduler->procs[scheduler->running_proc].deadline;
    } else {
        *earliest_deadline = scheduler->procs[0].deadline + 1;
    }
    // find earliest deadline among READY processes
    for (int i = 0; i < scheduler->proc_num; i++) {
        pcb_t proc = scheduler->procs[i];
        if (proc.state == READY && proc.deadline < *earliest_deadline) {
            *earliest_deadline = proc.deadline;
            *index = i;
        }
    }
}


/**
 * Start scheduling simulation
 * @param scheduler pointer to scheduler_t
 * @param fp pointer to FILE with process data
 */
void run(scheduler_t *scheduler, FILE *fp) {
    FILE *results_fp = fopen(OUTPUT_FILE, "w");
    while (1) {
        // if there's process running
        if (scheduler->running_proc != -1) {
            pcb_t *proc = &scheduler->procs[scheduler->running_proc];
            proc->remainingTime--;
            // the process completes
            if (proc->remainingTime == 0) {
                switch_process(scheduler, results_fp, scheduler->running_proc, -1);
            }
        }
        int eof_flag = accept_arriving_processes(scheduler, fp);

        int earliest_deadline_index = -1;
        int earliest_deadline;
        get_queue_head(scheduler, &earliest_deadline_index, &earliest_deadline);

        // Preemptive
        if (scheduler->running_proc != -1 && earliest_deadline_index != -1) {
            switch_process(scheduler, results_fp, scheduler->running_proc, earliest_deadline_index);
        }
            // if there's no running process and there are several processes, then run a process
        else if (scheduler->running_proc == -1 && scheduler->proc_num > 0) {
            switch_process(scheduler, results_fp, -1,
                           earliest_deadline_index); // run process with shortest remaining time
        }

        // if there's no processes and process data is end, end simulation
        if (eof_flag == EOF && scheduler->proc_num == 0) {
            break;
        }
        scheduler->current_time++;
    }
    fclose(results_fp);
}

int main(int argc, char **argv) {
    FILE *fp;
    // if no argument is specified, then read from default file
    if (argc < 2) {
        fp = fopen("processes.txt", "r");
    } else {
        fp = fopen(argv[1], "r");
    }
    if (!fp) {
        printf("error opening file");
        return -1;
    }
    scheduler_t scheduler = {.proc_num=0, .current_time=0, .running_proc=-1};
    run(&scheduler, fp);
    fclose(fp);
}
