#! ENV/bin/python

import pandas
import argparse
import filter

parser = argparse.ArgumentParser()
parser.add_argument(
        "-cols", help="Columns to split on", nargs="+", required=True);
parser.add_argument("-data_path", help="Path to data file", required=True);
parser.add_argument("-out_path", help="Path to out data file", required=True);

args = parser.parse_args()

data = pandas.DataFrame.from_csv(args.data_path, sep="\, ");
cols = args.cols;
unique_cols_data = data[cols].groupby(cols).size().reset_index()[cols]
unique_col_names = list(unique_cols_data.columns.values)

for i in range(0, len(unique_cols_data)):
    filtered_data = filter.filter(
        data, 
        unique_col_names, 
        unique_cols_data.loc[i, :].values)
    filter.write(filtered_data, args.out_path+"_tmp"+str(i))
