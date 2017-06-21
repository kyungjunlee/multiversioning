#include "batch/print_util.h"

#include <sstream>
#include <iomanip>
#include <cassert>
#include <iostream>

std::string PrintUtilities::double_to_string(double d) {
  std::stringstream ss;
  ss << std::fixed << std::setprecision(5) << d;
  return ss.str();
};

void TablePrinter::set_table_header(std::string t_header) {
  table_header = t_header;
};

// TODO: Generate the functions for vectors using a macro.
void TablePrinter::add_column_headers(std::vector<std::string> headers) {
  for (auto& h : headers) {
    add_column_header(h);
  }
};

void TablePrinter::add_column_header(std::string header) {
  if (table_data.size() < 1) {
    add_row();
  }

  table_data[0].push_back(header);
};

void TablePrinter::add_row() {
  table_data.push_back(RowData());
};

void TablePrinter::add_to_last_row(std::vector<std::string> data) {
  for (auto& d : data) {
    add_to_last_row(d);    
  }
};

void TablePrinter::add_to_last_row(std::string data) {
   add_to_row(table_data.size() - 1, data);
};

void TablePrinter::add_to_row(unsigned int row, std::vector<std::string> data) {
   for (auto& d : data) {
     add_to_row(row, d);
   }
 };

void TablePrinter::add_to_row(unsigned int row, std::string data) {
  while (table_data.size() < row) {
    add_row();
  }

  table_data[row].push_back(data);
};

void TablePrinter::print_table() {
  if (table_data.size() == 0) return;

  // Find the length of the longest element within the table for each column.
  unsigned int columns = table_data[0].size();
  std::vector<unsigned int> max_len(table_data[0].size(), 0);
  
  for (auto& row : table_data) {
    assert(row.size() == columns);
    for (unsigned int i = 0; i < columns; i++) {
      max_len[i] = max_len[i] < row[i].length() ? row[i].length() : max_len[i];
    }
  }

  unsigned int cel_padding = 2;
  // get the length of cels in each column
  std::vector<unsigned int> cel_lens;
  unsigned int table_len = 0;
  for (auto& ml : max_len) {
    unsigned int cel_len = ml + 2 * cel_padding;
    cel_lens.push_back(cel_len);
    table_len += cel_len;
  }
  
  auto pad = [](unsigned int len, char pad_char) {
    for (unsigned int i = 0; i < len; i++) {
      std::cout << pad_char;
    }
  };

  auto write_middle = [&pad](std::string str, unsigned int cel_len) {
    unsigned int padding = (cel_len - str.length()) / 2;
    assert(padding > 1);
    std::cout << "|";
    pad(padding - 1, ' ');
    std::cout << str;
    pad(cel_len - padding - str.length(), ' ');
  };

  auto hor_rule = [&pad, &table_len]() {
    std::cout << "|";
    pad(table_len - 1, '-');
    std::cout << std::endl;
  };

  // write the table title.
  write_middle(table_header, table_len);
  std::cout << std::endl;
  hor_rule();

  // write the data.
  for (auto& row : table_data) {
    for (unsigned int i = 0; i < row.size(); i++) {
      write_middle(row[i], cel_lens[i]);
    }
    std::cout <<  std::endl;
    hor_rule();
  }
};

