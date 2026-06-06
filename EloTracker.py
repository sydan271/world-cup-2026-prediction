import pandas as pd
import numpy as np
import random

class EloTracker:
    def __init__(self, base_rating=1500, k_factor=30):
        self.ratings = {}
        self.base_rating = base_rating
        self.k_factor = k_factor

    def get_rating(self, team):
        return self.ratings.get(team, self.base_rating)
    
    def expected_score(self, rating_a, rating_b):
        return 1 / (1 + 10 ** ((rating_b - rating_a) / 400))
    
    def update_ratings(self, team_a, team_b, score_a):
        rating_a = self.get_rating(team_a)
        rating_b = self.get_rating(team_b)

        expected_a = self.expected_score(rating_a, rating_b)
        #expected_b = self.expected_score(rating_b, rating_a)
        expected_b = 1 - expected_a 
        score_b = 1 - score_a

        new_rating_a = rating_a + self.k_factor * (score_a - expected_a)
        new_rating_b = rating_b + self.k_factor * (score_b - expected_b)

        #update 
        self.ratings[team_a] = new_rating_a
        self.ratings[team_b] = new_rating_b

        return new_rating_a, new_rating_b


