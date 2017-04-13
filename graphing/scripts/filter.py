#! ENV/bin/python

import pandas
import argparse

def filter(data_frame, cols, col_vals):
    for i in range(0, len(cols)):
        data_frame = data_frame.loc[data_frame[cols[i]] == int(col_vals[i])]
    return data_frame

def write(data_frame, path):
    data_frame.to_csv(path, index=False)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
            "-cols", help="Columns to filter by", 
            nargs="+", required=True);
    parser.add_argument(
            "-col_vals", help="Column values to order by", 
            nargs="+", required=True);
    parser.add_argument("-data_path", help="Path to data file", required=True);
    parser.add_argument("-out_path", help="Path to out data file", required=True);

    args = parser.parse_args()
    data = pandas.DataFrame.from_csv(args.data_path, index_col=None)
    write(filter(data, args.cols, args.col_vals), args.out_path);
