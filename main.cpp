#include<iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cmath>
#include <random>
#include <map>

//const from data_analyze.ipynb
const double intercept_away = -0.234020;
const double intercept_draw = -0.061628;
const double intercept_home = 0.295648;
const double coef_away = -0.006910;
const double coef_draw = -0.000218;
const double coef_home = 0.007128;

struct Team {
    std::string name;
    std::string group;
    double elo;
    
    // Group Stage Tracking
    int points = 0;
    
    // Monte Carlo Tracking (How many times they achieve these milestones)
    int groupStagesCleared = 0;
    int quarterfinalsReached = 0;
    int championshipsWon = 0;
};

int simulateMatch(double eloHome, double eloAway, bool isKnockout, std::mt19937& gen) {
    // Calculate the difference
    double elo_diff = eloHome - eloAway;

    // Calculate raw scores (logits) using your Python model's formula
    double z_away = intercept_away + (coef_away * elo_diff);
    double z_draw = intercept_draw + (coef_draw * elo_diff);
    double z_home = intercept_home + (coef_home * elo_diff);

    // Apply the Softmax function to get exact probabilities (0.0 to 1.0)
    double exp_away = std::exp(z_away);
    double exp_draw = std::exp(z_draw);
    double exp_home = std::exp(z_home);
    double sum_exp = exp_away + exp_draw + exp_home;

    double prob_away = exp_away / sum_exp;
    double prob_draw = exp_draw / sum_exp;
    double prob_home = exp_home / sum_exp;

    // In knockout rounds, a draw is impossible. 
    // We recalculate the probabilities ignoring the draw chance.
    if (isKnockout) {
        sum_exp = exp_away + exp_home;
        prob_away = exp_away / sum_exp;
        prob_draw = 0.0; // Draw is eliminated
        prob_home = exp_home / sum_exp;
    }

    // Generate a random number between 0.0 and 1.0
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    double roll = dist(gen);

    // Determine the winner based on the probability buckets
    if (roll < prob_away) return 0; // Away Win
    if (roll < prob_away + prob_draw) return 1; // Draw
    return 2; // Home Win
}

std::vector<Team> loadTeams(const std::string& filepath) {
    std::vector<Team> teams;
    std::ifstream file(filepath);
    std::string line;
    
    // Skip the header row
    std::getline(file, line); 
    
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        Team t;
        std::string eloStr;
        
        std::getline(ss, t.name, ',');
        std::getline(ss, t.group, ',');
        std::getline(ss, eloStr, ',');
        
        t.elo = std::stod(eloStr);
        teams.push_back(t);
    }
    return teams;
}

std::string runTournament(std::vector<Team> all_teams, std::mt19937& gen) {
    // A. Group Stage Setup
    std::map<std::string, std::vector<Team>> groups;
    for (auto& t : all_teams) {
        t.points = 0; // Reset points for this iteration
        groups[t.group].push_back(t);
    }

    std::vector<Team> advancers;
    std::vector<Team> third_places;

    // B. Play Group Matches
    for (auto& [g_name, g_teams] : groups) {
        for (size_t i = 0; i < g_teams.size(); ++i) {
            for (size_t j = i + 1; j < g_teams.size(); ++j) {
                int res = simulateMatch(g_teams[i].elo, g_teams[j].elo, false, gen);
                if (res == 2) g_teams[i].points += 3;
                else if (res == 0) g_teams[j].points += 3;
                else { g_teams[i].points += 1; g_teams[j].points += 1; }
            }
        }
        
        // Sort group by Points, then Elo (tiebreaker)
        std::sort(g_teams.begin(), g_teams.end(), [](const Team& a, const Team& b) {
            if (a.points == b.points) return a.elo > b.elo;
            return a.points > b.points;
        });

        // Top 2 advance automatically
        advancers.push_back(g_teams[0]);
        advancers.push_back(g_teams[1]);
        // 3rd place goes to the waiting pool
        third_places.push_back(g_teams[2]);
    }

    // C. Get Best 8 Third-Place Teams
    std::sort(third_places.begin(), third_places.end(), [](const Team& a, const Team& b) {
        if (a.points == b.points) return a.elo > b.elo;
        return a.points > b.points;
    });
    for (int i = 0; i < 8; ++i) {
        advancers.push_back(third_places[i]);
    }

    // D. Seed the Knockout Bracket (1 plays 32, 2 plays 31...)
    std::sort(advancers.begin(), advancers.end(), [](const Team& a, const Team& b) {
        if (a.points == b.points) return a.elo > b.elo;
        return a.points > b.points;
    });

    std::vector<Team> current_round;
    for (int i = 0; i < 16; ++i) {
        current_round.push_back(advancers[i]);
        current_round.push_back(advancers[31 - i]);
    }

    // E. Play Knockout Bracket
    while (current_round.size() > 1) {
        std::vector<Team> next_round;
        for (size_t i = 0; i < current_round.size(); i += 2) {
            int res = simulateMatch(current_round[i].elo, current_round[i+1].elo, true, gen);
            if (res == 2) next_round.push_back(current_round[i]);
            else next_round.push_back(current_round[i+1]);
        }
        current_round = next_round;
    }

    return current_round[0].name;
}

int main() {
    std::random_device rd;
    std::mt19937 gen(rd());

    std::vector<Team> teams = loadTeams("wc_2026_teams.csv");
    if (teams.empty()) {
        std::cerr << "Error: Could not load teams. Check CSV path." << std::endl;
        return 1;
    }

    int num_simulations = 10000000;
    std::map<std::string, int> championships;

    std::cout << "Simulating " << num_simulations << " World Cups... Please wait.\n";

    for (int i = 0; i < num_simulations; ++i) {
        std::string champion = runTournament(teams, gen);
        championships[champion]++;
    }

    // Sort and display results
    std::vector<std::pair<std::string, int>> results(championships.begin(), championships.end());
    std::sort(results.begin(), results.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });

    std::cout << "\n🏆 2026 WORLD CUP WIN PROBABILITIES 🏆\n";
    std::cout << "----------------------------------------\n";
    for (const auto& result : results) {
        double prob = (static_cast<double>(result.second) / num_simulations) * 100.0;
        if (prob >= 0.1) { // Only show teams with at least 0.1% chance
            std::cout << std::left << std::setw(15) << result.first 
                      << std::fixed << std::setprecision(2) << prob << "%" 
                      << "  (" << result.second << " wins)\n";
        }
    }

    return 0;
}