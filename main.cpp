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

int main() {
    // Initialize our high-performance random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // Quick test to ensure the math works
    std::cout << "Testing Match Engine (Spain vs. Uruguay)..." << std::endl;
    int result = simulateMatch(1650.0, 1550.0, false, gen);
    
    if (result == 2) std::cout << "Result: Home Win!" << std::endl;
    else if (result == 1) std::cout << "Result: Draw!" << std::endl;
    else std::cout << "Result: Away Win!" << std::endl;

    return 0;
}