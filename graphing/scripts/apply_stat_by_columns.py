#! ENV/bin/python

import pandas
import argparse

parser = argparse.ArgumentParser()
parser.add_argument(
        "-cols", help="Columns to split on", nargs="+", required=True);
parser.add_argument("-data_path", help="Path to data file", required=True);
parser.add_argument("-out_path", help="Path to out data file", required=True);
parser.add_argument("-stat", help="Function to apply. Supported mean and std", required=True)

args = parser.parse_args()

data = pandas.DataFrame.from_csv(args.data_path, index_col=None)
cols = args.cols
if (args.stat == 'mean'):
    data = data.groupby(cols, as_index=False).mean()[data.columns.values]
elif (args.stat == 'std'):
    data = data.groupby(cols, as_index=False).std()[data.columns.values]

data.to_csv(args.out_path, sep = ",", index=False)
