#include <string>
#include <algorithm>
#include <iostream>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <climits>
#include <regex>

using namespace std;

const int MX_TIME = 950;
const long long INF = LLONG_MAX;

const int LOWER_TIME_LIMIT = 355;
const int UPPER_TIME_LIMIT = 1281;

const int LAST_HOUR = 21;
const int MINUTES_IN_HOUR = 60;
const int PENNYS_IN_PRICE_INTEGRAL = 100;

const regex assistant_regex("[0-9]+|[a-zA-Z_^]+");
const regex question_regex(
        "[?] [a-zA-Z_^]+(?: (?:(?:0*[1-9][0-9]{0,17})|(?:0+)) [a-zA-Z_^]+)+");
const regex tram_regex(
        "(?:(?:0*[1-9][0-9]{0,17})|(?:0+))(?: [1-9][0-9]?[:][0-9]{2} [a-zA-Z_^]+)+");
const regex ticket_regex(
        "([a-zA-Z ]+) (0|[1-9][0-9]{0,17})[.]([0-9]{2}) ([1-9][0-9]{0,17})");

//  checks if the tram number was already added
bool is_tram_present(const long long tram_number,
                     unordered_map<long long, unordered_map<string, int> > &schedule_for_trams) {
    if (schedule_for_trams.count(tram_number) == 0)
        return false;
    return true;
}

//  checks if the tram stops at given stop
bool check_if_tram_at_stop(const long long tram_number, string &stop,
                           unordered_map<long long, unordered_map<string, int> > &schedule_for_trams) {
    if (!is_tram_present(tram_number, schedule_for_trams))
        return false;

    return (bool) schedule_for_trams[tram_number].count(stop);
}

//  adds tram trace to schedule
//  returns false if there is a loop in given tram trace
bool add_tram(const long long tram_number, vector<int> &schedule_time,
              vector<string> &tram_stops,
              unordered_map<long long, unordered_map<string, int> > &schedule_for_trams) {

    bool stop_repeated = false;
    for (size_t i = 0; i < tram_stops.size(); ++i) {
        auto stop_name = tram_stops[i];
        auto stop_time = schedule_time[i];

        if (check_if_tram_at_stop(tram_number, stop_name, schedule_for_trams))
            stop_repeated = true;

        schedule_for_trams[tram_number][stop_name] = stop_time;
    }

    if (stop_repeated) {
        for (size_t i = 0; i < tram_stops.size(); ++i) {
            auto stop_name = tram_stops[i];
            schedule_for_trams[tram_number].erase(stop_name);
            if (schedule_for_trams[tram_number].empty())
                schedule_for_trams.erase(tram_number);
        }

        return false;
    }

    return true;
}

//  adds new available ticket and updates optimal cost for given time
bool add_ticket(string &name, const long long price, long long minutes,
                vector <string> &ticket_name,
                vector<long long> &ticket_price, vector<long long> &ticket_time,
                vector<vector<long long> > &cost,
                vector <vector <vector <unsigned long long> > > &proposed_tickets,
                unordered_set <string> &present_ticket,
                vector <vector<unsigned long long> > &final_tickets) {

    if (present_ticket.find(name) != present_ticket.end())
        return false;

    //  time exceeds maximum minutes for a day
    if (minutes > MX_TIME)
        minutes = MX_TIME;

    unsigned long long ticket_id = ticket_name.size();

    present_ticket.insert(name);
    ticket_name.push_back(name);
    ticket_price.push_back(price);
    ticket_time.push_back(minutes);

    for (int layer = 1; layer <= 3; ++layer) {
        for (long long i = MX_TIME; i >= minutes; --i) {
            //  knapsack approach, use previously counted cost
            if (cost[layer-1][i - minutes] != INF) {
                if (cost[layer][i] > cost[layer-1][i - minutes] + price) {
                    cost[layer][i] = cost[layer-1][i - minutes] + price;
                    proposed_tickets[layer][i] = proposed_tickets[layer-1][i - minutes];
                    proposed_tickets[layer][i].push_back(ticket_id);
                }
            }
        }
    }

    long long minimum_cost = cost[3][MX_TIME];
    vector<unsigned long long> best_tickets = proposed_tickets[3][MX_TIME];

    for (long long i = MX_TIME; i >= 1; --i) {
        for (int layer = 1; layer <= 3; ++layer) {
            if (cost[layer][i] > minimum_cost) {
                final_tickets[i] = best_tickets;
            } else {
                final_tickets[i] = proposed_tickets[layer][i];
                if (minimum_cost > cost[layer][i]) {
                    minimum_cost = cost[layer][i];
                    best_tickets = proposed_tickets[layer][i];
                }
            }
        }
    }

    return true;
}

//  prints names of best maximum 3 tickets
//  returns true if nothing have to be printed on stderr
bool ask_for_tickets(vector <string> &stops, vector<long long> &trams_numbers,
                     unordered_map<long long, unordered_map<string, int> > &schedule_for_trams,
                     long long &number_of_tickets,
                     vector <string> &ticket_name,
                     vector <vector<unsigned long long> > &final_tickets) {
    long long n = trams_numbers.size();
    trams_numbers.push_back(trams_numbers[n - 1]);
    auto previous_tram = trams_numbers[0];
    auto first_stop_name = stops[0];
    auto previous_departure_time = schedule_for_trams[trams_numbers[0]][first_stop_name];
    string stop_to_wait = "";

    for (long long i = 0; i <= n; ++i) {
        auto stop_name = stops[i];
        auto arrival_time = schedule_for_trams[previous_tram][stop_name];
        auto departure_time = schedule_for_trams[trams_numbers[i]][stop_name];

        if (arrival_time < previous_departure_time)
            return false;

        //  we are too late for the tram
        if (arrival_time > departure_time)
            return false;

        //  we have to wait for the tram
        if ((arrival_time < departure_time) && stop_to_wait.empty())
            stop_to_wait = stop_name;

        previous_tram = trams_numbers[i];
        previous_departure_time = departure_time;
    }

    if (!stop_to_wait.empty()) {
        cout << ":-( " << stop_to_wait << endl;
        return true;
    }

    auto last_stop_name = stops[n];
    auto first_time = schedule_for_trams[trams_numbers[0]][first_stop_name];
    auto last_time = schedule_for_trams[trams_numbers[n]][last_stop_name];
    auto summary_time = last_time - first_time + 1;

    //  we can't buy tickets
    if (final_tickets[summary_time].size() == 0) {
        cout << ":-|" << endl;
        return true;
    }

    number_of_tickets += final_tickets[summary_time].size();
    bool semicolon = false;
    cout << "!";
    for (auto ticket: final_tickets[summary_time]) {
        if (semicolon)
            cout << ";";
        else
            semicolon = true;
        cout << " " << ticket_name[ticket];
    }
    cout << endl;

    return true;
}

//  prints information about error on standard output stream for errors
void print_error(string line, int line_number) {
    cerr << "Error in line " << line_number << ": " << line << endl;
}

//  counts number of whitespaces in given string
int number_of_whitespaces(string line) {
    int counter = 0;

    for (size_t i = 0; i < line.size(); i++) {
        if (line[i] == ' ') {
            counter++;
        }
    }

    return counter;
}

//  converts time to minutes
int time_to_minutes(int hours, int minutes) {
    return hours * MINUTES_IN_HOUR + minutes;
}

//  converts price to pennys
long long price_to_pennys(long price_integral, int price_fractional) {
    return price_integral * PENNYS_IN_PRICE_INTEGRAL + price_fractional;
}

//  extracts data from question about ticket command and checks for an error
//  calls ask_for_tickets function if there was no error before
//  if an error occurs, calls print_error function
void question_about_ticket_command(string line, int line_number,
                                   long long &number_of_tickets,
                                   unordered_map<long long, unordered_map<string, int> > &schedule_for_trams,
                                   vector <string> &ticket_name,
                                   vector <vector<unsigned long long> > final_tickets) {
    bool error = false;
    sregex_iterator i = sregex_iterator(line.begin(),
                                        line.end(), assistant_regex);
    int number_of_stops = (number_of_whitespaces(line) + 1) / 2;
    vector <string> stops = vector<string>(number_of_stops);
    vector<long long> trams_numbers = vector<long long>(number_of_stops - 1);
    int j = 0;
    stops.at(j) = (*i).str();
    i++;
    while ((i != sregex_iterator()) && !error) {
        trams_numbers.at(j) = stoll((*i).str());
        //  checks if given tram number is unique
        if (is_tram_present(trams_numbers.at(j), schedule_for_trams)) {
            i++;
            stops.at(j + 1) = (*i).str();
            //  checks if tram passes through given stops
            if (!check_if_tram_at_stop(trams_numbers.at(j), stops.at(j),
                                       schedule_for_trams)
                || !check_if_tram_at_stop(trams_numbers.at(j), stops.at(j + 1),
                                          schedule_for_trams)) {
                error = true;
            }
            i++;
            j++;
        } else {
            error = true;
        }
    }

    if (!error && !ask_for_tickets(stops, trams_numbers,
                                   schedule_for_trams, number_of_tickets,
                                   ticket_name, final_tickets)) {
        error = true;
    }

    if (error) {
        print_error(line, line_number);
    }
}

//  extracts data from add tram command and checks for an error
//  calls add_tram function if there was no error before
//  if an error occurs, calls print_error function
void add_tram_command(string line, int line_number,
                      unordered_map<long long, unordered_map<string, int> > &schedule_for_trams) {
    bool error = false;
    int number_of_stops = 0;
    vector <string> stops;
    vector<int> times;
    sregex_iterator i = sregex_iterator(line.begin(),
                                        line.end(), assistant_regex);
    long long tram_number = stoll((*i).str());
    i++;
    if (!is_tram_present(tram_number, schedule_for_trams)) {
        number_of_stops = number_of_whitespaces(line) / 2;
        stops = vector <string>(number_of_stops);
        times = vector <int>(number_of_stops);
        int j = 0, current_time = 0, previous_time = 0, hours = 0, minutes = 0;
        while ((i != sregex_iterator()) && !error) {
            hours = stoi((*i).str());
            i++;
            minutes = stoi((*i).str());
            //  time is like 22:74 etc.
            if (hours > LAST_HOUR || minutes >= MINUTES_IN_HOUR) {
                error = true;
            }
            else {
                current_time = time_to_minutes(hours, minutes);
                // check if time is between 5:55 and 21:21
                if ((current_time >= LOWER_TIME_LIMIT)
                    && (current_time <= UPPER_TIME_LIMIT)) {
                    i++;
                    times.at(j) = current_time;
                    stops.at(j) = (*i).str();
                    i++;
                    j++;
                    //  checks if given times are ascending
                    if (current_time <= previous_time) {
                        error = true;
                    }
                    previous_time = current_time;
                } else {
                    error = true;
                }
            }
        }
    } else {
        error = true;
    }

    if (!error && !add_tram(tram_number, times, stops, schedule_for_trams)) {
        error = true;
    }

    if (error) {
        print_error(line, line_number);
    }
}

//  extracts data from add ticket command
void add_ticket_command(smatch result, string &ticket_name, long long &price,
                        long long &validity) {
    smatch::iterator i = result.begin();
    long long price_integral;
    i++;
    ticket_name = *i;
    i++;
    price_integral = stoll((*i).str());
    i++;
    price = price_to_pennys(price_integral, stoi((*i).str()));
    i++;
    validity = stoll(*i);
}

int main() {
    smatch result;
    string line, name = "";
    int line_number = 0;
    long long number_of_tickets = 0, price = -1, validity = -1;
    //  container mapping tram number to map with stop name as key
    unordered_map<long long, unordered_map<string, int> > schedule_for_trams;
    vector <string> ticket_name;
    //  containers for tickets' data
    vector <long long> ticket_price, ticket_time;
    vector <vector <unsigned long long> > final_tickets(MX_TIME + 3);
    //  container remembering tickets' names that already occurred
    unordered_set <string> present_ticket;

    vector <vector <vector <unsigned long long> > > proposed_tickets(4);
    for (int i = 0; i < 4; ++i) {
        proposed_tickets[i].resize(MX_TIME + 3);
    }

    //  container holding cost for given time
    vector <vector <long long> > cost(4);
    for (int i = 0; i < 4; ++i) {
        cost[i].resize(MX_TIME + 3);
        for(unsigned long long j = 0; j < proposed_tickets[i].size(); ++j)
            cost[i][j] = INF;
    }

    cost[0][0] = 0;

    while (!getline(cin, line).eof()) {
        line_number++;
        if (!line.empty()) {
            if (regex_match(line, question_regex)) {
                question_about_ticket_command(line, line_number,
                                              number_of_tickets,
                                              schedule_for_trams,
                                              ticket_name, final_tickets);
            } else if (regex_match(line, tram_regex)) {
                add_tram_command(line, line_number, schedule_for_trams);
            } else if (regex_match(line, result, ticket_regex)) {
                add_ticket_command(result, name, price, validity);
                if (!add_ticket(name, price, validity, ticket_name,
                                ticket_price,
                                ticket_time, cost, proposed_tickets,
                                present_ticket, final_tickets)) {
                    print_error(line, line_number);
                }
            } else {
                print_error(line, line_number);
            }
        }
    }

    cout << number_of_tickets << endl;
}
