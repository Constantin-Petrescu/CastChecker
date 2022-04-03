dataset.zip contains three json files and :

- entropy.json - results of extracting the casts from the lucid files
- flagged.json - casts flagged by the tool
- sampled.json - casts used in the manual evaluation for the paper
- a folder data_cast_paper with .lucid file which contain casts collected
	from Chromium project

The four pdf plots are the plots used in the paper. They show all the casts
	from the chromium project; the coloured points represent flagged cases.

The file data_analysis_manual_evaluation_results are the results of our ratings
	from the manual evaluation. 

All of the json files can be also generated with the help of the python scripts
	from data_analysis_scripts. 