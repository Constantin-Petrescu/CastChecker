# Data analysis scripts

Contains the code for Cast Checker tool. It generates the Conditional Entropy values.

### Dependencies
Python 3.9.7 has been used at the time of development

Let ibraries used:
- dit==1.2.3
- matplotlib==3.4.1
- pandas==1.2.3
- tabulate==0.8.7


### Usage

Menu.py is the central script to run the rest of the files

Menu.py takes two arguments:
1. `option` is the first argument and it represents what script will be executed. The argument has type int and the values used can be:
	- 0 to display the options
	- 1 to run the entropy calculations from casts from lucid files from the following path: data/final_data_casting_paper
	- 2 to flag cases from Entropy.json 
	- 3 to create plots from entropy.json

2. entropy_threshold which is the Conditional Entropy Threshold value. This argument has type int and by default it is 1. 
Commands examples:
```
python3 menu.py option entropy_threshold
python3 menu.py 1 1
python3 menu.py 2 1
python3 menu.py 3 1
```

