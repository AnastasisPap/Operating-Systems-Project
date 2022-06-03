#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "vars.h"

// Initialize variables
int bank_account_balance, available_cashiers, available_phones;
pthread_mutex_t bank_acc_lock, cashier_lock, phones_lock;
pthread_cond_t cashier_cond, phones_cond;
bool theatre_matrix[zone_A_rows + zone_B_rows][seats_per_row];
pthread_mutex_t matrix_lock;

int successful, unsuccessful_seats, unsuccessful_pay = 0;
pthread_mutex_t successful_m, unsuccessful_seats_m, unsuccessful_pay_m;

int main(int argc, char** argv)
{
    int rc;
    // Convert the first CLI argument to integer
    int total_customers = atoi(argv[1]);
    // Convert the second CLI argument to float 
    float seed_in = atof(argv[2]);
    // Reserve memory for the threads
    pthread_t threads[total_customers]; 
    // Random seed
    srand(time(NULL));

    // Initialize Mutexes
    pthread_mutex_init(&bank_acc_lock, NULL);
    pthread_mutex_init(&cashier_lock, NULL);
    pthread_mutex_init(&phones_lock, NULL);
    pthread_mutex_init(&matrix_lock, NULL);
    pthread_mutex_init(&successful_m, NULL);
    pthread_mutex_init(&unsuccessful_pay_m, NULL);
    pthread_mutex_init(&unsuccessful_seats_m, NULL);
    pthread_cond_init(&cashier_cond, NULL);
    pthread_cond_init(&phones_cond, NULL);
    available_cashiers = n_cash;
    available_phones = n_tel;

    // theatre_matrix[i][j] = false -> that seat is reserved / true -> seat not reserved
    int i, j;
    for (i = 0; i < zone_A_rows + zone_B_rows; i++) {
        for (j = 0; j < seats_per_row; j++)
            theatre_matrix[i][j] = true;
    }


    i = 0;
    for (i = 0; i < total_customers; i++)
    {
        int wait = 0;
        // If more than one customer has arrived
        if (i > 0)
        {
            // Wait for a random time in a given interval
            wait = (rand() % (t_reshigh - t_reslow + 1)) + t_reslow;
            sleep(wait);
        }
        // DON'T FORGET TO FREE FROM THE MEMORY WHEN FINISHED
        int* id = malloc(sizeof(int));
        *id = i + 1;
        // create the thread
        rc = pthread_create(&threads[i], NULL, &connect_with_tel, id);   
    }

    // Join the threads
    for (i = 0; i < total_customers; i++)
        pthread_join(threads[i], NULL);

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
    
    // Delete the mutexes and condition variables
    pthread_mutex_destroy(&bank_acc_lock);
    pthread_mutex_destroy(&cashier_lock);
    pthread_mutex_destroy(&phones_lock);
    pthread_mutex_destroy(&matrix_lock);
    pthread_mutex_destroy(&successful_m);
    pthread_mutex_destroy(&unsuccessful_pay_m);
    pthread_mutex_destroy(&unsuccessful_seats_m);
    pthread_cond_destroy(&cashier_cond);
    pthread_cond_destroy(&phones_cond);

    return 0;
}

void* connect_with_tel(void* args)
{
    int id = *(int*) args;

    // Try to get a mutex (increase and decrease available phones)
    pthread_mutex_lock(&phones_lock);
    // while there are no available telephones, wait for a condition signal to continue
    // Use while to avoid spurious wakeups
    while (available_phones == 0)
        pthread_cond_wait(&phones_cond, &phones_lock);

    // Decrement the available phones
    available_phones--;
    // zone_sel = 0 -> Zone A, zone_sel = 1 -> Zone B
    int zone_selection = bernoulli_distr(1 - prob_zone_A);
    // Select a random amount of total tickets
    int total_tickets = (rand() % (n_seathigh - n_seatlow + 1)) + n_seatlow;

    // info[] = {row of that zone, seat starting from the left of that row}, {-1, -1} = no available
    int info[] = {-1, -1};
    int row = (zone_selection == 0 ? 0 : zone_A_rows);
    int end_row = (zone_selection == 0 ? zone_A_rows : zone_A_rows + zone_B_rows);

    // The telephone agents require a random amount of time to find the seats
    int search_time = (rand() % (t_seathigh - t_seatlow + 1)) + t_seatlow;
    // Unlock the mutex while "searching"
    pthread_mutex_unlock(&phones_lock);
    sleep(search_time);
    pthread_mutex_lock(&phones_lock);
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
        pthread_mutex_lock(&matrix_lock);
        // False = reserved seats
        for (int i = info[1]; i < total_tickets + info[1]; i++) {
            theatre_matrix[info[0]][i] = false;
        }
        pthread_mutex_unlock(&matrix_lock);
        // Release the resources, mutexes, and condition variables
        available_phones++;
        pthread_cond_signal(&phones_cond);
        pthread_mutex_unlock(&phones_lock);
        // Move to payment
        make_payment(args, zone_selection, info, total_tickets);
    // There are no consecutive seats in a row in that zone
    } else {
        // lock mutex to update statistics variable
        pthread_mutex_lock(&unsuccessful_seats_m);
        unsuccessful_seats++;
        pthread_mutex_unlock(&unsuccessful_seats_m);
        char zone = (zone_selection == 0 ? 'A' : 'B');
        printf("id(%d): Reservation failed, can't find %d consecutive seats in zone %c\n", id, total_tickets, zone);
        // Free the memory of the id variable that was allocated in the main() before calling connect_with_tel
        free(args);
        // Release the resources
        available_phones++;
        pthread_cond_signal(&phones_cond);
        pthread_mutex_unlock(&phones_lock);
        // Exit thread
        pthread_exit(NULL);
    }

    return NULL;
}

void* make_payment(void* args, int zone_selection, int info[2], int total_tickets)
{
    int id = *(int*) args;
    // try to get a lock (increment/decrement cashiers)
    pthread_mutex_lock(&cashier_lock);
    // while there are no available cashiers wait for a signal
    while (available_cashiers == 0)
        pthread_cond_wait(&cashier_cond, &cashier_lock);
    
    // Reserve a cashier
    available_cashiers--;
    // The cashier requires a random amount of time to try the payment
    int wait_time = (rand() % (t_cashhigh - t_cashlow + 1)) + t_cashlow;
    // Unlock the mutex while waiting
    pthread_mutex_unlock(&cashier_lock);
    sleep(wait_time);
    pthread_mutex_lock(&cashier_lock);

    // payment_success = 1 => payment is successful, otherwise failed
    int payment_success = bernoulli_distr(p_payment_success);
    if (payment_success == 1) {
        // reserve lock to update the statistics variable
        pthread_mutex_lock(&successful_m);
        successful++;
        pthread_mutex_unlock(&successful_m);
        // calculate the total cost
        int payment_amount = (zone_selection == 0 ? zone_A_cost : zone_B_cost) * total_tickets;
        // acquire lock to update bank account balance
        pthread_mutex_lock(&bank_acc_lock);
        bank_account_balance += payment_amount;
        pthread_mutex_unlock(&bank_acc_lock);
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
        pthread_mutex_lock(&unsuccessful_pay_m);
        unsuccessful_pay++;
        pthread_mutex_unlock(&unsuccessful_pay_m);
        printf("id(%d): Reservation failed because of unsuccessful payment.\n", id);
        // acquire lock to release the reserved seats from the theatre layout
        pthread_mutex_lock(&matrix_lock);
        for (int i = info[1]; i < total_tickets + info[1]; i++)
            theatre_matrix[info[0]][i] = true;
        pthread_mutex_unlock(&matrix_lock);
        // release resources
        available_cashiers++;
    }

    // release the lock and signal
    pthread_cond_signal(&cashier_cond);
    pthread_mutex_unlock(&cashier_lock);
    // free the memory of the variable id that was allocated in the main() function
    free(args);
    // exit the thread
    pthread_exit(NULL);
}

// Returns 1 with a probability of p and 0 with a probability of 1 - p
bool bernoulli_distr(float p)
{
    // Uni = random Uniform[0, 1] (we divide with RAND_MAX)
    double uni = (double) rand() / RAND_MAX;

    if (uni < p) return true;
    return false;
}