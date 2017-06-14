#ifndef PRINT_UTIL_TIME_H_
#define PRINT_UTIL_TIME_H_

#include <vector>
#include <string>

namespace PrintUtilities {
  std::string double_to_string(double d);
};

class TablePrinter {
private:
  typedef std::vector<std::string> RowData;
  std::vector<RowData> table_data;
  std::string table_header;
public:
  void set_table_header(std::string t_header);

  void add_column_headers(std::vector<std::string> headers);
  void add_column_header(std::string header);

  void add_row();
  
  void add_to_last_row(std::vector<std::string> data);
  void add_to_last_row(std::string data);
  
  void add_to_row(unsigned int row, std::vector<std::string> data);
  void add_to_row(unsigned int row, std::string data);

  void print_table();
};

#endif // PRINT_UTIL_TIME_H_
