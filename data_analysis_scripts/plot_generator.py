import os
import json
import matplotlib
import matplotlib.pyplot as plt
from matplotlib.widgets import Button
from matplotlib.text import Annotation
import pandas as pd
from tabulate import tabulate

# import sys


def main(entropy_threshold):


    # reading from entropy.json
    currend_dir = os.getcwd()
    directory_path = currend_dir[0:currend_dir.index("data_analysis")]
    directory_path += "data/"
    input_data = pd.read_json(directory_path + "entropy.json")


    input_data['outlier'] = input_data.apply(lambda row: row["conditional_entropy"] <= entropy_threshold - 0.001, axis=1)
    print("outside of layer:", len(input_data[input_data["outlier"]== False]))
    print("inside of layer:" , len(input_data[input_data["outlier"]== True]))

    bop_re_data = input_data[
        input_data['type_of_cast'].str.contains("BinaryOperator CXXReinterpretCastExpr|VarDecl CXXReinterpretCastExpr")]
    bop_st_data = input_data[
        input_data['type_of_cast'].str.contains("BinaryOperator CXXStaticCastExpr|VarDecl CXXStaticCastExpr")]
    bop_co_data = input_data[
        input_data['type_of_cast'].str.contains("BinaryOperator CXXConstCastExpr|VarDecl CXXConstCastExpr")]
    bop_dy_data = input_data[
        input_data['type_of_cast'].str.contains("BinaryOperator CXXDynamicCastExpr|VarDecl CXXDynamicCastExpr")]

    ce_re_data = input_data[input_data['type_of_cast'].str.contains("CallExpr CXXReinterpretCastExpr")]
    ce_st_data = input_data[input_data['type_of_cast'].str.contains("CallExpr CXXStaticCastExpr")]
    ce_co_data = input_data[input_data['type_of_cast'].str.contains("CallExpr CXXConstCastExpr")]
    ce_dy_data = input_data[input_data['type_of_cast'].str.contains("CallExpr CXXDynamicCastExpr")]

    ce_data = (ce_re_data, ce_st_data, ce_co_data, ce_dy_data)
    bop_data = (bop_re_data, bop_st_data, bop_co_data, bop_dy_data)
    print("\n")
    print (len(ce_re_data))
    print (len(ce_st_data))
    print (len(ce_co_data))
    print (len(ce_dy_data))
    print("\n")
    print (len(bop_re_data))
    print (len(bop_st_data))
    print (len(bop_co_data))
    print (len(bop_dy_data))
    print("\n")
    print (len(bop_co_data[bop_co_data["outlier"]== True ]))
    print (len(bop_co_data[bop_co_data["outlier"]== False ]))
    bop_co_data[bop_co_data["outlier"]== False ].to_csv("dataframe.csv", index=False, header=True)

    
    colours_combined = ("red", "darkviolet", "blue", "pink")
    # colours_combined = ( "0.5", "0.5", "0.5", "0.8")
    symbols_combined = ("o","s","D","^")

    groups_all = ("R", "S", "C","D")

    fig, ax = plt.subplots()
    global generated_labels
    generated_labels = []
    ann_list = []


    def plot_len_value(data, colours, groups, len_string, value, xlabel, ylabel, markers):
        index = 0
        for subset, colour, group, marker in zip(data, colours, groups, markers):
            if subset is not None:
                plt.scatter(subset[(subset["length_source"]>0) & subset["outlier"]== True][len_string].str.len(),
                            subset[(subset["length_source"]>0) & subset["outlier"]== True][value], marker=marker, picker=True, edgecolors="0.5", facecolors="none", s=13, label=group, alpha=0.5)
                print(subset[(subset["length_source"]>0) & subset["outlier"]== True][value])
                plt.scatter(subset[subset["outlier"]== False][len_string].str.len(),
                            subset[subset["outlier"]== False][value], marker=marker,
                            picker=True, c = colour, s=13,
                            label=group+"*", alpha=0.5)
            index += 1

        plt.xlabel(xlabel)
        plt.ylabel(ylabel)
        plt.legend(numpoints=1)
        # plt.grid(True)

    def plot_len_value_left_conditional(data, colours, groups, len_string, xlabel, ylabel, markers):
        index = 0
        for subset, colour, group, marker in zip(data, colours, groups, markers):
            plt.scatter(subset[len_string].str.len(), subset["conditional_entropy"] + subset["source_entropy"] - subset["destination_entropy"], marker=marker,
                        picker=True, c=colour, s=13, label=group, alpha=0.5)
            index += 1
        plt.xlabel(xlabel)
        plt.ylabel(ylabel)
        plt.legend(numpoints=1)
        plt.grid(True)


    def plot_(data, colours, groups, len_string, xlabel, ylabel, markers):
        index = 0
        for subset, colour, group, marker in zip(data, colours, groups,markers):
            plt.scatter(subset[len_string].str.len(), subset["conditional_entropy"] + subset["source_entropy"] - subset["destination_entropy"], marker=marker,
                        picker=True, c=colour, s=13, label=group, alpha=0.5)
            index += 1
        plt.xlabel(xlabel)
        plt.ylabel(ylabel)
        plt.legend(numpoints=1)
        plt.grid(True)


    def plot_type(data, colours, groups, len_string, value, xlabel, ylabel, 
        markers):
        index = 0
        for subset, colour, group, marker in zip(data, colours, groups, markers):
            plt.scatter(subset[(subset["length_source"]<20) & (subset["source_entropy"]< 4.1) & (subset["outlier"]== False)][len_string].str.len() 
                + subset[(subset["length_source"]<20) & (subset["source_entropy"]< 4.1) & (subset["outlier"]== False)]["source_type"].str.len(), 
                subset[(subset["length_source"]<20) & (subset["source_entropy"]< 4.1) & (subset["outlier"]== False)][value], marker=marker,
                        picker=True, c='red', s=13, 
                        label=group+" outlier", alpha=0.5)
            plt.scatter(subset[(subset["length_source"]<20) & (subset["source_entropy"]< 4.1) & (subset["outlier"]== True)][len_string].str.len() 
                + subset[(subset["length_source"]<20) & (subset["source_entropy"]< 4.1) & (subset["outlier"]== True)]["source_type"].str.len(), 
                subset[(subset["length_source"]<20) & (subset["source_entropy"]< 4.1) & (subset["outlier"]== True)][value], marker=marker,
                        picker=True, c=colour, s=13, label=group, alpha=0.5)
            index += 1
        plt.xlabel(xlabel)
        plt.ylabel(ylabel)
        plt.legend(numpoints=1)


    def on_pick(event):
        ind = event.ind
        for i, a in enumerate(ann_list):
            a.remove()
        ann_list[:] = []
        label_pos_x = event.mouseevent.xdata
        label_pos_y = event.mouseevent.ydata
        label_string = ""
        all_data = bop_data + ce_data
        for subset in all_data:
            if not subset[(subset["source"].str.len() + subset["source_type"].str.len() >= label_pos_x - 0.9) &
                          (subset["source"].str.len() + subset["source_type"].str.len()<= label_pos_x + 0.9) &
                          (subset['source_type_entropy'] >= label_pos_y - 0.9) &
                          (subset['source_type_entropy'] <= label_pos_y + 0.9) &
                          (subset["outlier"]!=0) ].empty:
                label_data = subset[(subset["source"].str.len() + 
                            subset["source_type"].str.len() >= label_pos_x - 0.9) &
                            (subset["source"].str.len() + 
                            subset["source_type"].str.len() <= label_pos_x + 0.9) &
                            (subset['source_type_entropy'] >= label_pos_y - 0.5) &
                            (subset['source_type_entropy'] <= label_pos_y + 0.5) & 
                            (subset["outlier"]!=0)]
                # print((label_data.to_string()))
                for row in label_data.head().itertuples():
                    label_string += "Source: " + str(row[1]) + "\n"
                    label_string += "Source type: " + str(row[2]) + "\n"
                    label_string += "Destination: " + str(row[3]) + "\n"
                    label_string += "Destination type: " + str(row[4]) + "\n"
                    label_string += "Source entropy: " + str(row[8]) + "\n"
                    label_string += "Destination entropy: " + str(row[9]) + "\n"
                    label_string += "Joint entropy: " + str(row[14]) + "\n"
                    label_string += "File: " + str(row[13]) + "\n"
                    label_string += "Outlier: " + str(row[17]) + "\n"
                    label_string += "---------------------------------------------------------\n"
        print(label_string)
        ann = ax.annotate(label_string, (label_pos_x, label_pos_y), fontsize=10,
                          bbox=dict(boxstyle='round,pad=0.2', fc='blue', alpha=0.5))
        ann_list.append(ann)
        ax.figure.canvas.draw_idle()





    xlabel = 'Length of Source Expression'
    ylabel = 'Conditional Entropy'
    plot_len_value([ce_st_data], ["orange"], ["S"],
                  "source", "conditional_entropy", xlabel, ylabel, ["s"])
    plt.savefig(directory_path + 'callexpr_condEntropy_sourceLength_S.pdf', format='pdf',dpi=1200)
    plt.clf()
    # ce_data = (ce_re_data, ce_st_data, ce_co_data, ce_dy_data)

    plot_len_value([ce_re_data, None, ce_co_data, bop_dy_data], colours_combined, groups_all, "source", "conditional_entropy", xlabel, ylabel, symbols_combined)
    plt.savefig(directory_path + 'callexpr_condEntropy_sourceLength_rest.pdf', format='pdf', dpi=1200)
    plt.clf()

    plot_len_value([bop_st_data], ["orange"], ["S"],
                  "source", "conditional_entropy", xlabel, ylabel, ["s"])
    plt.savefig(directory_path +'bop_condEntropy_sourceLength_S.pdf', format='pdf',dpi=1200)
    plt.clf()

    plot_len_value([bop_re_data, None, bop_co_data, bop_dy_data], colours_combined, groups_all, "source", "conditional_entropy", xlabel, ylabel, symbols_combined)
    plt.savefig(directory_path + 'bop_condEntropy_sourceLength_rest.pdf', format='pdf',dpi=1200)
    plt.clf()



if __name__ == "__main__":
    entropy_threshold = 1
    main(entropy_threshold)
