#define zone_A_rows 10
#define zone_B_rows 20
#define total_rows (zone_A_rows + zone_B_rows)
#define seats_per_row 10
const int n_cash = 2;
const int n_tel = 3;
const int zone_A_cost = 30;
const int zone_B_cost = 20;
const int n_seatlow = 1;
const int n_seathigh = 5;
const int t_reslow = 1;
const int t_reshigh = 5;
const float prob_zone_A = 0.3;
const int t_seatlow = 5;
const int t_seathigh = 13;
const int t_cashlow = 4;
const int t_cashhigh = 8;
const float p_payment_success = 0.9;

void* connect_with_tel(void* args);
int* find_row(int zone, int total_tickets);
void* make_payment(void* args, int zone_selection, int info[2], int total_tickets);
bool bernoulli_distr(float p, void* args);
double calculate_average_time(struct timespec start[], struct timespec end[], int n);