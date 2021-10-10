from vaderSentiment.vaderSentiment import SentimentIntensityAnalyzer
import tweepy
import re
import os
import numpy as np
import pandas as pd
import sqlite3
import uuid

def remove_pattern(input_txt, pattern):
	r = re.findall(pattern, input_txt)
	for i in r:
		input_txt = re.sub(i, '', input_txt)
	return input_txt

def clean_tweets(tweets):
	#remove twitter Return handles (RT @xxx:)
	tweets = np.vectorize(remove_pattern)(tweets, "RT @[\w]*:") 

	#remove twitter handles (@xxx)
	tweets = np.vectorize(remove_pattern)(tweets, "@[\w]*")

	#remove URL links (httpxxx)
	tweets = np.vectorize(remove_pattern)(tweets, "https?://[A-Za-z0-9./]*")

	#remove special characters, numbers, punctuations (except for #)
	tweets = np.core.defchararray.replace(tweets, "[^a-zA-Z]", " ")

	return tweets


def get_scores(df):
	scores = []# Declare variables for scores
	compound_list = []
	positive_list = []
	negative_list = []
	neutral_list = []

	for index, tweet in enumerate(df['text']):
		print(index)
		compound = analyser.polarity_scores(tweet)["compound"]
		pos = analyser.polarity_scores(tweet)["pos"]
		neu = analyser.polarity_scores(tweet)["neu"]
		neg = analyser.polarity_scores(tweet)["neg"]

		comp_norm = (compound+1)/2

		conn.execute("INSERT INTO FEELINGS (COMPOUND,COMPNORM,POSITIVE,NEGATIVE,NEUTRAL) \
			VALUES (?,?,?,?,?)", (compound, comp_norm, pos, neg, neu))

		scores.append({
			"Compound": compound,
			"Comp_norm": comp_norm,
			"Positive": pos,
			"Negative": neg,
			"Neutral": neu
		})

	return scores

if __name__ == "__main__":

	#initialise 
	conn = sqlite3.connect('test_feels.db')
	print("Opened database successfully")

	# conn.execute('''DROP TABLE IF EXISTS FEELINGS''')

	conn.execute('''CREATE TABLE IF NOT EXISTS FEELINGS
		(ID INTEGER PRIMARY KEY AUTOINCREMENT,
		COMPOUND REAL NOT NULL,
		COMPNORM REAL NOT NULL,
		POSITIVE REAL NOT NULL,
		NEGATIVE REAL NOT NULL,
		NEUTRAL REAL NOT NULL);''')
	print("Table created successfully")

	conn.commit()

	#initialise analyser
	analyser = SentimentIntensityAnalyzer()

	auth = tweepy.AppAuthHandler(os.environ['consumer_key'], os.environ['consumer_secret'])
	api = tweepy.API(auth)
	client = tweepy.Client(os.environ['bearer_token'])

	keywords = ["protest"]
	sentiments = []

	for search in keywords:
		try:
			for tweet in tweepy.Cursor(api.search_tweets, q=search).items(500):
				sentiments.append({
					"search": search,
					"text": tweet.text
				})
		except tweepy.errors.TweepyException as e:
			print(e)

	df = pd.DataFrame.from_dict(sentiments)
	df['text'] = clean_tweets(df['text'])
	df = df.drop_duplicates()

	scores = get_scores(df)
	print(scores)
	conn.commit()
	conn.close()
