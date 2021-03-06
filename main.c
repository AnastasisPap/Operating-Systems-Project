#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include "main.h"

#define BILLION 1000000000000L;
struct thread_args
{
    struct timespec* end_service_time;
    struct timespec* end_wait_time_phone;
    struct timespec* start_wait_time_cash;
    struct timespec* end_wait_time_cash;
    int id;
};

// Initialize variables
int bank_account_balance, available_cashiers, available_phones;
pthread_mutex_t bank_acc_lock, cashier_lock, phones_lock;
pthread_cond_t cashier_cond, phones_cond;
bool theatre_matrix[total_rows][seats_per_row];
pthread_mutex_t matrix_lock;
int seed_in;

int successful, unsuccessful_seats, unsuccessful_pay, mean_wait_time, mean_service_time = 0;
pthread_mutex_t successful_m, unsuccessful_seats_m, unsuccessful_pay_m, mean_wait_time_m, mean_service_time_m;

int main(int argc, char** argv)
{
    int rc;
    // Convert the first CLI argument to integer
    int total_customers = atoi(argv[1]);
    // Convert the second CLI argument to float 
    seed_in = atoi(argv[2]);
    // Reserve memory for the threads
    pthread_t threads[total_customers]; 

    // Initialize Mutexes
    if (pthread_mutex_init(&bank_acc_lock, NULL) != 0) printf("Error initializing\n");
    if (pthread_mutex_init(&cashier_lock, NULL) != 0) printf("Error initializing\n");
    if (pthread_mutex_init(&phones_lock, NULL) != 0) printf("Error initializing\n");
    if (pthread_mutex_init(&matrix_lock, NULL) != 0)printf("Error initializing\n");
    if (pthread_mutex_init(&successful_m, NULL) != 0)printf("Error initializing\n");
    if (pthread_mutex_init(&unsuccessful_pay_m, NULL) != 0)printf("Error initializing\n");
    if (pthread_mutex_init(&unsuccessful_seats_m, NULL) != 0)printf("Error initializing\n");
    if (pthread_mutex_init(&mean_wait_time_m, NULL) != 0)printf("Error initializing\n");
    if (pthread_mutex_init(&mean_service_time_m, NULL) != 0)printf("Error initializing\n");
    if (pthread_cond_init(&cashier_cond, NULL) != 0)printf("Error initializing\n");
    if (pthread_cond_init(&phones_cond, NULL) != 0)printf("Error initializing\n");
    available_cashiers = n_cash;
    available_phones = n_tel;

    // theatre_matrix[i][j] = false -> that seat is reserved / true -> seat not reserved
    int i, j;
    for (i = 0; i < zone_A_rows + zone_B_rows; i++) {
        for (j = 0; j < seats_per_row; j++)
            theatre_matrix[i][j] = true;
    }


    struct timespec start_service_time[total_customers], end_service_time[total_customers];
    struct timespec start_wait_time_phone[total_customers], end_wait_time_phone[total_customers];
    struct timespec start_wait_time_cash[total_customers], end_wait_time_cash[total_customers];
    struct timespec start_program, end_program;
    clock_gettime(CLOCK_REALTIME, &start_program);

    i = 0;
    for (i = 0; i < total_customers; i++)
    {
        clock_gettime(CLOCK_REALTIME, &start_service_time[i]);
        clock_gettime(CLOCK_REALTIME, &start_wait_time_phone[i]);

        int wait = 0;
        // DON'T FORGET TO FREE FROM THE MEMORY WHEN FINISHED
        int id = i + 1;
        struct thread_args* args = malloc(sizeof(struct thread_args));
        (*args).id = id;
        (*args).end_service_time = &end_service_time[i];
        (*args).end_wait_time_phone = &end_wait_time_phone[i];
        (*args).end_wait_time_cash = &end_wait_time_cash[i];
        (*args).start_wait_time_cash = &start_wait_time_cash[i];
        // create the thread
        if (pthread_create(&threads[i], NULL, &connect_with_tel, args) != 0) printf("Error creating thread");
        // Wait for a random time in a given interval
        unsigned int* seed = malloc(sizeof(unsigned int));
        *seed = i + seed_in;
        wait = (rand_r(seed) % (t_reshigh - t_reslow + 1)) + t_reslow;
        free(seed);
        sleep(wait);
    }

    // Join the threads
    for (i = 0; i < total_customers; i++)
        if (pthread_join(threads[i], NULL) != 0) printf("Error joining thread");

    // Print the theatre layout
    printf("Theatre plan (0 = seat not reserved, 1 = seat reserved):\n\n");
    for (i = 0; i < zone_A_rows + zone_B_rows; i++) {
        printf("| ");
        for (int j = 0; j < seats_per_row; j++)
            printf("%d | ", theatre_matrix[i][j]);
        printf("\n");
    }

    // Print the requested ouputs
    printf("\n");
    printf("Total earnings: %d\n", bank_account_balance);
    int total_transactions = successful + unsuccessful_pay + unsuccessful_seats;
    printf("Percentage of successful transactions: %lf\n", (double) successful / (double) total_transactions);
    printf("Percentage of unsuccessful seat reservations: %lf\n", (double) unsuccessful_seats / (double) total_transactions);
    printf("Percentage of unsuccessful payments: %lf\n", (double) unsuccessful_pay / (double) total_transactions);
    double average_wait_time, average_service_time;
    average_wait_time = calculate_average_time(start_wait_time_phone, end_wait_time_phone, total_customers) + calculate_average_time(start_wait_time_cash, end_wait_time_cash, total_customers);
    average_service_time = calculate_average_time(start_service_time, end_service_time, total_customers);
    printf("Average wait time: %lf\n", average_wait_time);
    printf("Average service time; %lf\n", average_service_time);
    clock_gettime(CLOCK_REALTIME, &end_program);
    double total_time = (end_program.tv_sec - start_program.tv_sec) + (end_program.tv_nsec - start_program.tv_nsec) / BILLION;
    printf("Program execution total time: %lf\n", total_time);
    
    // Delete the mutexes and condition variables
    if (pthread_mutex_destroy(&bank_acc_lock) != 0) printf("Error destroying\n");
    if (pthread_mutex_destroy(&cashier_lock) != 0) printf("Error destroying\n");
    if (pthread_mutex_destroy(&phones_lock) != 0) printf("Error destroying\n");
    if (pthread_mutex_destroy(&matrix_lock) != 0) printf("Error destroying\n");
    if (pthread_mutex_destroy(&successful_m) != 0) printf("Error destroying\n");
    if (pthread_mutex_destroy(&unsuccessful_pay_m) != 0) printf("Error destroying\n");
    if (pthread_mutex_destroy(&unsuccessful_seats_m) != 0) printf("Error destroying\n");
    if (pthread_mutex_destroy(&mean_wait_time_m) != 0) printf("Error destroying\n");
    if (pthread_mutex_destroy(&mean_service_time_m) != 0) printf("Error destroying\n");
    if (pthread_cond_destroy(&cashier_cond) != 0) printf("Error destroying\n");
    if (pthread_cond_destroy(&phones_cond) != 0) printf("Error destroying\n");

    return 0;
}

double calculate_average_time(struct timespec start[], struct timespec end[], int n)
{
    double total_time = 0;

    for (int i = 0; i < n; i++)
        total_time += (end[i].tv_sec - start[i].tv_sec) + (end[i].tv_nsec - start[i].tv_nsec) / BILLION;

    return total_time / n;
}

void* connect_with_tel(void* in)
{
    struct thread_args* args = (struct thread_args*)in;
    int id = args->id;
    struct timespec* end_wait_time_phone = args->end_wait_time_phone;

    // Try to get a mutex (increase and decrease available phones)
    if (pthread_mutex_lock(&phones_lock) != 0) printf("Error getting mutex lock\n");
    // while there are no available telephones, wait for a condition signal to continue
    // Use while to avoid spurious wakeups
    while (available_phones == 0)
        if (pthread_cond_wait(&phones_cond, &phones_lock) != 0) printf("Error waiting for condition var\n");

    // Decrement the available phones
    available_phones--;
    // stop the time (connected with phone)
    clock_gettime(CLOCK_REALTIME, end_wait_time_phone);
    // zone_sel = 0 -> Zone A, zone_sel = 1 -> Zone B
    unsigned int* seed = malloc(sizeof(unsigned int));
    *seed = seed_in + id;
    int zone_selection = bernoulli_distr(1 - prob_zone_A, seed);
    // Select a random amount of total tickets
    int total_tickets = (rand_r(seed) % (n_seathigh - n_seatlow + 1)) + n_seatlow;

    // info[] = {row of that zone, seat starting from the left of that row}, {-1, -1} = no available
    int info[] = {-1, -1};
    int row = (zone_selection == 0 ? 0 : zone_A_rows);
    int end_row = (zone_selection == 0 ? zone_A_rows : zone_A_rows + zone_B_rows);

    // The telephone agents require a random amount of time to find the seats
    int search_time = (rand_r(seed) % (t_seathigh - t_seatlow + 1)) + t_seatlow;
    free(seed);
    // Unlock the mutex while "searching"
    if (pthread_mutex_unlock(&phones_lock) != 0) printf("Error unlocking\n");
    sleep(search_time);
    if (pthread_mutex_lock(&phones_lock) != 0) printf("Error getting lock\n");
    // Sliding window algorithm for optimization
    for (int i = row; i < end_row; i++) 
    {
        int sum = 0;
        for (int j = 0; j < seats_per_row; j++)
        {
            // If the window hasn't isn't as big as the total tickets
            if (j < total_tickets) sum += theatre_matrix[i][j];
            else {
                // If we found n consecutive seats in a row
                if (sum == total_tickets) {
                    info[0] = i;
                    info[1] = j - total_tickets;
                    break;
                } 
                // Move the window
                else sum += theatre_matrix[i][j] - theatre_matrix[i][j - total_tickets];
            }
        }

        if (info[0] != -1) break;
        // If a window is found in the end of that row
        if (sum == total_tickets) {
            info[0] = i;
            info[1] = seats_per_row - total_tickets;
            break;
        }
    }
    
    // SUCCESS if info[0] > -1
    if (info[0] > -1) {
        // Lock matrix mutex to update its values
        if (pthread_mutex_lock(&matrix_lock) != 0) printf("error getting lock\n");
        // Reserve the seats
        // False = reserved seats
        for (int i = info[1]; i < total_tickets + info[1]; i++) {
            theatre_matrix[info[0]][i] = false;
        }
        if (pthread_mutex_unlock(&matrix_lock) != 0) printf("error unlocking\n");
        // Release the resources, mutexes, and condition variables
        available_phones++;
        if (pthread_cond_signal(&phones_cond) != 0) printf("Error signaling\n");
        if (pthread_mutex_unlock(&phones_lock) != 0) printf("Error unlocking\n");
        // Move to payment
        make_payment(args, zone_selection, info, total_tickets);
    // There are no consecutive seats in a row in that zone
    } else {
        // lock mutex to update statistics variable
        if (pthread_mutex_lock(&unsuccessful_seats_m) != 0) printf("error getting lock\n");
        unsuccessful_seats++;
        if (pthread_mutex_unlock(&unsuccessful_seats_m) != 0) printf("error unlocking\n");
        char zone = (zone_selection == 0 ? 'A' : 'B');
        printf("id(%d): Reservation failed, can't find %d consecutive seats in zone %c\n", id, total_tickets, zone);
        // Release the resources
        available_phones++;
        if (pthread_cond_signal(&phones_cond) != 0) printf("Error signaling\n");
        if (pthread_mutex_unlock(&phones_lock) != 0) printf("error unlocking\n");
        // Exit thread
        struct timespec* end_service_time = args->end_service_time;
        struct timespec* start_wait_time_cash = args->start_wait_time_cash;
        struct timespec* end_wait_time_cash = args->end_wait_time_cash;

        clock_gettime(CLOCK_REALTIME, end_service_time);
        clock_gettime(CLOCK_REALTIME, start_wait_time_cash);
        clock_gettime(CLOCK_REALTIME, end_wait_time_cash);
        // Free the memory of the id variable that was allocated in the main() before calling connect_with_tel
        free(in);
        pthread_exit(NULL);
    }

    return NULL;
}

void* make_payment(void* in, int zone_selection, int info[2], int total_tickets)
{
    struct thread_args* args = (struct thread_args*)in;
    int id = args->id;
    struct timespec* start_wait_time_cash = args->start_wait_time_cash;
    struct timespec* end_wait_time_cash = args->end_wait_time_cash;
    clock_gettime(CLOCK_REALTIME, start_wait_time_cash);
    // try to get a lock (increment/decrement cashiers)
    if (pthread_mutex_lock(&cashier_lock) != 0) printf("error getting lock\n");
    // while there are no available cashiers wait for a signal
    while (available_cashiers == 0)
        if (pthread_cond_wait(&cashier_cond, &cashier_lock) != 0) printf("error waiting\n");
    
    clock_gettime(CLOCK_REALTIME, end_wait_time_cash);
    // Reserve a cashier
    available_cashiers--;
    unsigned int* seed = malloc(sizeof(unsigned int));
    *seed = seed_in + id;
    // The cashier requires a random amount of time to try the payment
    int wait_time = (rand_r(seed) % (t_cashhigh - t_cashlow + 1)) + t_cashlow;
    // Unlock the mutex while waiting
    if (pthread_mutex_unlock(&cashier_lock) != 0) printf("error unlocking\n");
    sleep(wait_time);
    if (pthread_mutex_lock(&cashier_lock) != 0) printf("error getting lock\n");

    // payment_success = 1 => payment is successful, otherwise failed
    int payment_success = bernoulli_distr(p_payment_success, seed);
    free(seed);
    if (payment_success == 1) {
        // reserve lock to update the statistics variable
        if (pthread_mutex_lock(&successful_m) != 0) printf("error getting lock \n");
        successful++;
        if (pthread_mutex_unlock(&successful_m) != 0) printf("error unlocking\n");
        // calculate the total cost
        int payment_amount = (zone_selection == 0 ? zone_A_cost : zone_B_cost) * total_tickets;
        // acquire lock to update bank account balance
        if (pthread_mutex_lock(&bank_acc_lock) != 0) printf("error getting lock\n");
        bank_account_balance += payment_amount;
        if (pthread_mutex_unlock(&bank_acc_lock) != 0) printf("error unlocking\n");
        char zone = (zone_selection == 0 ? 'A' : 'B');
        // release the resources
        available_cashiers++;
        printf("id(%d): Reservation successful! Info about your reservation:\n Zone: %c\n Row: %d\n Seats: ", id, zone, info[0]);
        for (int i = info[1]; i < total_tickets + info[1]; i++)
            printf("%d, ", i);
        printf("\n Total cost: %d euros.\n", payment_amount);
    // Payment failed
    } else {
        // acquire lock for the statistics variable
        if (pthread_mutex_lock(&unsuccessful_pay_m) != 0) printf("error getting lock\n");
        unsuccessful_pay++;
        if (pthread_mutex_unlock(&unsuccessful_pay_m) != 0) printf("error unlocking\n");
        printf("id(%d): Reservation failed because of unsuccessful payment.\n", id);
        // acquire lock to release the reserved seats from the theatre layout
        if (pthread_mutex_lock(&matrix_lock) != 0) printf("error getting lock\n");
        for (int i = info[1]; i < total_tickets + info[1]; i++)
            theatre_matrix[info[0]][i] = true;
        if (pthread_mutex_unlock(&matrix_lock) != 0) printf("error unlocking\n");
        // release resources
        available_cashiers++;
    }

    // release the lock and signal
    if (pthread_cond_signal(&cashier_cond) != 0) printf("error signaling\n");
    if (pthread_mutex_unlock(&cashier_lock) != 0) printf("error unlocking\n");

    struct timespec* end_service_time = args->end_service_time;
    clock_gettime(CLOCK_REALTIME, end_service_time);
    // free the memory of the variable id that was allocated in the main() function
    free(in);
    // exit the thread
    pthread_exit(NULL);
}

// Returns 1 with a probability of p and 0 with a probability of 1 - p
bool bernoulli_distr(float p, void* args)
{
    unsigned int* random_res = (unsigned int*) args;
    // Uni = random Uniform[0, 1] (we divide with RAND_MAX)
    double uni = (double) rand_r(random_res) / RAND_MAX;

    if (uni < p) return true;
    return false;
}
