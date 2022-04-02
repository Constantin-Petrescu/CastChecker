import os
import json
import matplotlib
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
from matplotlib.widgets import Button
from matplotlib.text import Annotation
import ast
import re

def main(entropy_threshold):
	results = []
	list_base_start = ['char', 'short', 'int', 'long', 'double', 'float', 
		'void', 'unsigned', 'signed', 'uint', 'size_t','const char', 
		'const short', 'const int', 'const long', 'const double', 
		'const float', 'const void', 'const unsigned', 'const signed', 
		'const uint', 'const size_t', 'const std::']


	# reading from entropy.json
	currend_dir = os.getcwd()
	directory_path = currend_dir[0:currend_dir.index("data_analysis")]
	directory_path += "data/"
	input_data = pd.read_json(directory_path + "entropy.json")
	

	for index, row in input_data.iterrows():
		# Removing this-> from the source and destination;
		if "this->" in row['source']:
			row['source'] = row['source'][0:row['source'].index('this->')] + row['source'][row['source'].index('this->')+6:]
		if "this->" in row['destination']:
			row['destination'] = row['destination'][0:row['destination'].index('this->')]+row['destination'][row['destination'].index('this->')+6:]

		# Preprocessing filter - checking for literals
		if row['source']=="nullptr":
			continue
		if row['source'].lower() in row['destination'].lower() :
			continue
		if row['source'].isnumeric() == True:
			continue
		if row['destination'].isnumeric() == True:
			continue
		if row['source'][:1].isnumeric() == True:
			x = re.search("([0-9]+[A-Z]+)",row['source'])
			if x is not None:
				continue
		# check for string literals
		x = re.search("(^[\'\"]+.*[\'\"]+$)",row['source'])
		if x is not None:
			continue

	    # Preprocessing filter - substrings for identifiers
		if row['source'] in row['destination']:
			continue
			found = False

		# Preprocessing filter - substrings for types
		for base_start in list_base_start:
			x = re.search("(^" + base_start + ")",row['source_type'])
			if x is not None:
				found = True
				continue
			x = re.search("(^" + base_start + ")",row['destination_type'])
			if x is not None:
				found = True
				continue
	            
		if row['source_type'] in row['destination_type'] and found is False:
			continue
		if "::" in row['source_type']:
			if row['source_type'][row['source_type'].rfind("::"):] in row['destination_type']:
				continue

		# Check entropy higher than the threshold
		if row['conditional_entropy'] >= entropy_threshold:
			results.append(row)
	        
	# printing in file the flagged cases
	results_pd = pd.DataFrame(results)
	results_pd.to_json(directory_path+"flagged.json",indent=2,orient='records')


	print("There are " +  str(len(input_data) - len(results_pd)) + 
		  " cases not flagged and " + str(len(results_pd)) + " cases flagged"
		  " out of " +  str(len(input_data)) + " cases")
	print("Cases flagged were stored in " + directory_path + "flagged.json")

    

if __name__ == "__main__":
	entropy_threshold = 1
	main(entropy_threshold)

