import sys
import argparse

import entropy
import flagger
import plot_generator

def main(args):
	parser = argparse.ArgumentParser()
	parser.add_argument("option", type=int, default=0, choices=[0,1,2,3], 
		help="1 - Generate entropy calculations from "
			 "data/final_data_casting_paper\n"
			 "2 - Flag cases from Entropy.json \n" 
			 "3 - Create plots from entropy.json")
	parser.add_argument("entropy_threshold", type=int, default=1, 
		help="Conditional Entropy Threshold value - by default is 1")
	args = parser.parse_args(args)
	if args.option == 0 or args.option > 3:
		print("Please introduce a correct option!")
		print("1 - Generate entropy calculations from "
			  "data/final_data_casting_paper")
		print("2 - Flag cases from file Entropy.json")
		print("3 - Create plots from file Entropy.json")
		sys.exit()

	if args.option == 1:
		print("Option 1 selected! Generating Entropy.json for files from ")
		entropy.main()
		sys.exit()

	if args.option == 2:
		print("Option 2 selected! Flag cases from Entropy.json \n\n")
		flagger.main(args.entropy_threshold)
		sys.exit()

	if args.option == 3:
		print("Option 3 selected! Create plots from file Entropy.json\n\n")
		plot_generator.main(args.entropy_threshold)
		sys.exit()



if __name__ == '__main__':
    main(sys.argv[1:])