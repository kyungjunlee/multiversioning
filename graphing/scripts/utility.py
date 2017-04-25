#! ENV/bin/python

import pandas
import argparse
import os
import sys

def get_this_file_path():
    return os.path.abspath(__file__);

def static_var(varname, value):
    def decorate(func):
        setattr(func, varname, value)
        return func
    return decorate

def get_tmp_dir_path():
    return os.path.join(
            os.path.dirname(get_this_file_path()), 
            "temporaries_for_graphing")

def create_tmp_dir():
    os.makedirs(get_tmp_dir_path())

# Write a data frame to a temporary file.
@static_var("counter", 0)
def write_to_tmp(data):
    dir = os.path.join(get_tmp_dir_path(), "tmp" + str(write_to_tmp.counter))
    write_to_tmp.counter += 1
    write_data(data, dir)

# We assume the following about the cols:
#   1) Cols are the columns we will use in plotting.
#   2) cols[0] is the x-axis variable.
#   3) cols[1] is the y-axis variable.
#   4) cols[2] is the error bar (if any)
#   5) cols[2] and further may be used as additional information
def get_plot_format(data, cols):
    return data[cols]

def destroy_tmp_dir_and_files():
    # Make sure we don't destroy anything that is not a file inside the 
    # directory
    dir = get_tmp_dir_path()
    for f in os.listdir(dir):
        file_path = os.path.join(dir, f)
        try:
            if os.path.isfile(file_path):
                os.unlink(file_path)
            else:
                # Make sure that as soon as we realize there are directories
                # we quit and raise exception to avoid further damage
                raise Exception;
        except Exception as e:
            print(e.message)
            sys.exit()
    # At this point we have deleted everything
    os.rmdir(dir)

# Return only those rows of data frame for which every col within 
# cols has the corresponding value within col_vals. The correspondance
# is defined "index-wise."
def filter_by_col_vals(data_frame, cols, col_vals):
    df = data_frame
    for i in range(0, len(cols)):
        df = df.loc[data_frame[cols[i]] == int(col_vals[i])]
    return df

# Return an array of data frames such that each of the data frames
# has a unique combination of values corresponding to cols.
def split_by_unique_vals(data, cols):
    split_data = []

    unique_cols_data = data[cols].groupby(cols).size().reset_index()[cols]
    unique_col_names = list(unique_cols_data.columns.values)

    for i in range(0, len(unique_cols_data)):
        split_data.append(filter_by_col_vals(
            data, 
            unique_col_names,
            unique_cols_data.loc[i, :].values));

    return split_data

# Returns a dataframe with averages of all fields grouped by
# cols.
def get_means_by_unique_vals(data, cols):
    columns = data.columns.values
    return data.groupby(cols, as_index=False).mean()[columns]

# Returns a dataframe with std of all fields grouped by
# cols.
def get_std_by_unique_vals(data, cols):
    columns = data.columns.values
    return data.groupby(cols, as_index=False).std()[columns]

def read_data_in(path):
    return pandas.DataFrame.from_csv(path, index_col=None);

def write_data(data, path):
    data.to_csv(path, index=False)

def get_gnuplot_as_string(script_path):
    f = open(script_path, 'r')
    string_contents = f.read()
    f.close()
    return string_contents

def exec_gnuplot_script(name, arg_names, arg_values):
    command = "gnuplot << EOF\n"

    for arg, arg_val in zip(arg_names, arg_values):
        command +=  arg + "=" + str(arg_val) + "\n"

    # Actually read in the gnuplot script and append it to what already exists.
    script_path = os.path.join(
            os.path.dirname(get_this_file_path()), 
            "../gnu_plot_scripts/" + name)
    command += get_gnuplot_as_string(script_path)

    command += "EOF"
    print "Executing gnuplot script:", command
    os.system(command);
