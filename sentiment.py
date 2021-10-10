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

	for index, tweet in enumerate(df['tweet']):
		if index%50 == 0: print(index)
		compound = analyser.polarity_scores(tweet)["compound"]
		pos = analyser.polarity_scores(tweet)["pos"]
		neu = analyser.polarity_scores(tweet)["neu"]
		neg = analyser.polarity_scores(tweet)["neg"]

		comp_norm = (compound+1)/2

		conn.execute("INSERT INTO FEELINGS (COMPOUND,COMPNORM,POSITIVE,NEGATIVE,NEUTRAL) \
			VALUES (?,?,?,?,?)", (compound, comp_norm, pos, neg, neu))

	return

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


	df = pd.read_csv('datasets/protest.csv', sep='\t')
	df['tweet'] = clean_tweets(df['tweet'])
	df = df.drop_duplicates()

	get_scores(df)
	# print(scores)
	conn.commit()
	conn.close()
