import dit
import os
import json
import re
import hashlib
import string
from collections import Counter
from decimal import *

# Our util file
import util

# counter for each type of cast
implicit_cast_count_CE = 0
implicit_cast_count_BO = 0
reinterpret_cast_count_CE = 0
reinterpret_cast_count_BO = 0
dynamic_cast_count_CE = 0
dynamic_cast_count_BO = 0
const_cast_count_CE = 0
const_cast_count_BO = 0
static_cast_count_CE = 0
static_cast_count_BO = 0

no_tokens_count = 0

global failed 
global failed_atomic 
global count_total 
global count_processed 

# how many casts failed at processing
failed = 0
failed_atomic = 0

# count of how many casts were processed
count_processed = 0
count_total = 0




def check_list_empty(inputList):
    flattened_list = [a for b in inputList for a in b]
    flattened_list = [e for e in flattened_list if e]
    if (len(flattened_list) < 1):
        return False
    return True


def calculate_stats(item):
    global implicit_cast_count_CE, implicit_cast_count_BO, no_tokens_count
    global reinterpret_cast_count_CE, reinterpret_cast_count_BO
    global dynamic_cast_count_CE, dynamic_cast_count_BO, const_cast_count_CE
    global const_cast_count_BO, static_cast_count_CE, static_cast_count_BO

    if item['ast_meta_info']['node_type'].split()[0] == "CallExpr":
        if "static_cast" in item['ast_meta_info']['tokens']:
            static_cast_count_CE += 1
        if "reinterpret_cast" in item['ast_meta_info']['tokens']:
            reinterpret_cast_count_CE += 1
        if "dynamic_cast" in item['ast_meta_info']['tokens']:
            dynamic_cast_count_CE += 1
        if "const_cast" in item['ast_meta_info']['tokens']:
            const_cast_count_CE += 1
    else:
        if "static_cast" in item['ast_meta_info']['tokens']:
            static_cast_count_BO += 1
        if "reinterpret_cast" in item['ast_meta_info']['tokens']:
            reinterpret_cast_count_BO += 1
        if "dynamic_cast" in item['ast_meta_info']['tokens']:
            dynamic_cast_count_BO += 1
        if "const_cast" in item['ast_meta_info']['tokens']:
            const_cast_count_BO += 1

    if check_list_empty(item["x_children"]) is False:
        no_tokens_count += 1
    # print("no kids " + str(flattened_list) + str(item))


def print_stats():
    print("\nTotal stats: \n")
    print("static count:" + str(static_cast_count_CE + static_cast_count_BO))
    print("dynamic count:" + str(dynamic_cast_count_CE + dynamic_cast_count_BO))
    print("reinterpret count:" + str(reinterpret_cast_count_CE + reinterpret_cast_count_BO))
    print("const count:" + str(const_cast_count_CE + const_cast_count_BO))
    print("\nCall expr stats: \n")
    print("static count:" + str(static_cast_count_CE))
    print("dynamic count:" + str(dynamic_cast_count_CE))
    print("reinterpret count:" + str(reinterpret_cast_count_CE))
    print("const count:" + str(const_cast_count_CE))
    print("\nBinary operator stats: \n")
    print("static count:" + str(static_cast_count_BO))
    print("dynamic count:" + str(dynamic_cast_count_BO))
    print("reinterpret count:" + str(reinterpret_cast_count_BO))
    print("const count:" + str(const_cast_count_BO))


def utf_encode(s):
    if isinstance(s, str):
        s = s.encode('utf-8')
    return str(s)


def is_list_empty(inList):
    if isinstance(inList, list):  # Is a list
        return all(map(is_list_empty, inList))
    return False  # Not a list


def splitByChar(inputList, char):
    flattened = []
    for item in inputList:
        if char in item:
            for it in item.split(char):
                flattened.append(it)
        else:
            flattened.append(item)
    return flattened


def removeSpecialFromListStrings(inputList, chars, i):
    for it in range(0, len(inputList)):
        for char in chars:
            # ignore for types and the macros to split them by this chars
            if i == "1":
                continue
            inputList[it] = inputList[it].replace(char, "")
        if i == "0":
            inputList[it] = inputList[it].replace("->", "")
            inputList[it] = inputList[it].replace("<", "")
            inputList[it] = inputList[it].replace(">", "")
            inputList[it] = inputList[it].replace("-", "")
        if len(inputList[it]) != 1 and i != "1":
            inputList[it] = inputList[it].replace(" ", "")
    return inputList


def pmf(x, i, size_of_hash, list_last_tokens):
    flattened_list = [a for b in x for a in b]
    unwanted_chars = ["(", ")", ",", "&", "+", "]", "[", ".", "/", "*", "-", " ", ">", "<"]
    flattened_list = splitByChar(flattened_list, "_")
    flattened_list = splitByChar(flattened_list, "->")
    for char in unwanted_chars:
        flattened_list = splitByChar(flattened_list, char)
    flattened_list = splitByChar(flattened_list, "::")
    flattened_list = removeSpecialFromListStrings(flattened_list, unwanted_chars, i)
    flattened = [e for e in flattened_list if e]

    # print(flattened)
    # Special cases for types and the macros that could not be tokenised
    if i == "1":
        flattened = removeSpecialFromListStrings(flattened, ".", 0)
        flattened = removeSpecialFromListStrings(flattened, "-", 0)
    flattened = [e for e in flattened if e]
    v = []

    for item in flattened:
        splitted_list = re.findall('([A-Z](?=[a-z]+[0-9]*)[a-z]*[0-9]*|[A-Z]+(?=[A-Z]{2,})[A-Z])', item)
        if len(splitted_list) != 0:
            for it in splitted_list:
                it = it.lower()
                h = hashlib.blake2b(digest_size=size_of_hash)
                list_last_tokens.append(it)
                h.update(it.encode('utf-8'))
                v.append(h.hexdigest())
            if 'z' >= item[0] >= 'a':
                h = hashlib.blake2b(digest_size=size_of_hash)
                list_last_tokens.append(item[0:item.index(splitted_list[0])])
                h.update(item[0:item.index(splitted_list[0])].encode('utf-8'))
                v.append(h.hexdigest())
        else:
            list_last_tokens.append(item)
            h = hashlib.blake2b(digest_size=size_of_hash)
            h.update(item.encode('utf-8'))
            v.append(h.hexdigest())
    C = Counter(v)
    total = float(sum(C.values()))
    for key in C:
        C[key] = Decimal(C[key] / total)
    return [k for k, v in C.items()], [v for k, v in C.items()]



def extract_data_from_lucid_files(data_path):
    global failed 
    global failed_atomic 
    global count_total 
    global count_processed 

    full_list = []
    files = util.get_list_of_files(data_path + "final_data_casting_paper")
    print("file size: " + str(len(files)))
    entropy_data_return = []
    for filename in files[:]:
        print(str(files.index(filename)) + " " + str(filename))
        
        if "lucid" not in filename:
            continue
        
        with open(filename) as json_file:
            data = json.load(json_file)
            for item in data[:]:
                calculate_stats(item)
                count_total += 1
                if len(item["to"]["text"]) != 0 and len(item["from"]["text"]) != 0:  
                # and check_list_empty(item["x_children"]) is False:
                    done = False
                    size_of_hash = 60
                    while not done:
                        try:
                            list_last_tokens = []
                            list_last_tokens_type = []
                            token_list = item["x_children"][:]

                            if check_list_empty(item["x_children"]) is False:
                                from_txt = item["from"]["text"]
                                from_txt = from_txt.replace(item["from"]["type"], "")
                                token_list.append([from_txt])
                                p_rhs = pmf(token_list, "1", size_of_hash, [])
                                token_list.append([item["to"]["text"]])
                                list_last_tokens[:] = []
                                p_joint = pmf(token_list, "1", size_of_hash, list_last_tokens)
                            else:
                                p_rhs = pmf(token_list, "0", size_of_hash, [])
                                token_list.append([item["to"]["text"]])
                                list_last_tokens[:] = []
                                p_joint = pmf(token_list, "0", size_of_hash, list_last_tokens)

                            check_lst_has_chars = re.search('[a-zA-Z]', item["to"]["text"])
                            if check_lst_has_chars is None:
                                je_lhs = 0
                            else:
                                p_lhs = pmf([[item["to"]["text"]]], "1", size_of_hash, [])
                                d_lhs = dit.Distribution(p_lhs[0], p_lhs[1])
                                je_lhs = dit.shannon.entropy(d_lhs)

                            d_rhs = dit.Distribution(p_rhs[0], p_rhs[1])
                            d_joint = dit.Distribution(p_joint[0], p_joint[1])
                            je_rhs = dit.shannon.entropy(d_rhs)
                            je_joint = dit.shannon.entropy(d_joint)

                            je_type_joint = 0
                            je_type = 0

                            token_list2 = [[item["from"]["text"], item["from"]["type"]]]
                            p_type_rhs = pmf(token_list2, "1", size_of_hash, [])
                            token_list2.append([item["to"]["text"], item["to"]["type"]])
                            list_last_tokens_type[:] = []
                            p_type_joint = pmf(token_list2, "1", size_of_hash, list_last_tokens_type)
                            p_type_lhs = pmf([[item["to"]["text"], item["to"]["type"]]], "1", size_of_hash, [])

                            d_type_rhs = dit.Distribution(p_type_rhs[0], p_type_rhs[1])
                            d_type_lhs = dit.Distribution(p_type_lhs[0], p_type_lhs[1])
                            d_type_joint = dit.Distribution(p_type_joint[0], p_type_joint[1])

                            je_type_rhs = dit.shannon.entropy(d_type_rhs)
                            je_type_lhs = dit.shannon.entropy(d_type_lhs)
                            je_type_joint = dit.shannon.entropy(d_type_joint)

                            tmp = (item["from"]["text"], item["from"]["type"],
                                   item["to"]["text"], item["to"]["type"],
                                   item['ast_meta_info']['node_type'],
                                   len(item["from"]["text"]),
                                   je_joint - je_rhs, je_rhs,
                                   je_lhs, je_type_joint - je_type_rhs,
                                   je_type_rhs, je_type_lhs,
                                   item["ast_meta_info"]["file"],
                                   je_joint, je_type_joint,
                                   list_last_tokens, list_last_tokens_type,
                                   check_list_empty(item["x_children"]))
                            count_processed += 1
                            entropy_data_return.append(tmp)
                            done = True
                        except Exception as e:
                            if str(e) != "Python int too large to convert to C long":
                                if str(e) == "`outcomes` must be nonempty if no sample space is given":
                                    failed_atomic += 1
                                    print("ERROR: " + str(e) + '\n\n')
                                    print("\nERROR: " + str(item))
                                else:
                                    print("ERROR: " + str(e) + '\n\n')
                                    # traceback.print_exc(file=sys.stdout)
                                    print("\nERROR: " + str(item))
                            elif str(e) == "Python int too large to convert to C long":
                                if size_of_hash == 60:
                                    size_of_hash = 40
                                    continue
                                if size_of_hash == 40:
                                    size_of_hash = 20
                                    continue
                                if size_of_hash == 20:
                                    size_of_hash = 15
                                    continue
                                if size_of_hash == 15:
                                    size_of_hash = 10
                                    continue
                                done = True
                                failed += 1
                            None

    return entropy_data_return



def write_data(entropy_data, data_path):
    global failed 
    global failed_atomic 
    global count_total 
    global count_processed 


    # Clear the file where we put data. 
    sorted_by_sec = sorted(entropy_data, key=lambda tup: tup[4])
    f = open( data_path+"entropy.json", 'w')
    f.close()

    res = []
    for i in sorted_by_sec[0:]:
        tmp2 = {'source': i[0], 'source_type': i[1], 'destination': i[2], 
                'destination_type': i[3], 'type_of_cast': i[4], 
                'length_source': i[5], 'conditional_entropy': i[6], 
                'source_entropy': i[7], 'destination_entropy': i[8], \
                'conditional_type_entropy': i[9], 'source_type_entropy': i[10], 
                'destination_type_entropy': i[11],  "joint_entropy": i[13], 
                "joint_type_entropy": i[14], "file": i[12]}

        if len(i[15]) > 1:
            tmp2['tokens_mixed_identifiers'] = i[15]
        if len(i[16]) > 1:
            tmp2['tokens_mixed_types'] = i[16]
        tmp2['tokeniser'] = '"' + str(i[17]) + '"'
        # print (i)
        res.append(tmp2)

    with open(data_path+"entropy.json", 'w', encoding='utf-8') as outputFile:
        json.dump(res, outputFile, ensure_ascii=False, indent=4)
    print_stats()
    print("\ntotal casts: " + str(count_total)
          + " casts processed:" + str(count_processed) + " casts with no tokens: " +
          str(no_tokens_count) + " failed with int too large error: " +
          str(failed) + " failed for atomic problem: " +
          str(failed_atomic) + "\n")


def main():
    currend_dir = os.getcwd()
    data_path = currend_dir[0:currend_dir.index("data_analysis")] + "data/"
    print(data_path + "\n")

    entropy_data = extract_data_from_lucid_files(data_path)
    write_data(entropy_data, data_path)

if __name__ == "__main__":
    main()







