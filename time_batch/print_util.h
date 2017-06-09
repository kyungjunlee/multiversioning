#ifndef PRINT_UTIL_TIME_H_
#define PRINT_UTIL_TIME_H_

class TablePrinter {
private:
  typedef std::vector<std::string> RowData;
  std::vector<RowData> table_data;
  std::string table_header;
public:
  void set_table_header(std::string t_header) {
    table_header = t_header;
  };

  void add_column_header(std::string header) {
    if (table_data.size() < 1) {
      add_row();
    }

    table_data[0].push_back(header);
  };

  void add_row() {
    table_data.push_back({});
  };

  void add_to_last_row(std::string data) {
    add_to_row(table_data.size() - 1, data);
  };

  void add_to_row(unsigned int row, std::string data) {
    while (table_data.size() < row) {
      add_row();
    }

    table_data[row].push_back(data);
  };

  void print_table() {
    if (table_data.size() == 0) return;

    // Find the length of the longest element within the table.
    unsigned int cel_len = 0;
    for (auto& row : table_data) {
      for (auto& str : row) {
        cel_len = cel_len < str.length() ? str.length() : cel_len;
      }
    }

    unsigned int cel_padding = 2;
    unsigned int table_len = (cel_padding * 2 + cel_len) * table_data[0].size();
    unsigned int i;
    
    auto pad = [](unsigned int len, char pad = ' ') {
      for (unsigned int i = 0; i < len; i++) {
        std::cout << pad;
      }
    };

    auto write_middle = [this](std::string str, unsigned int cel_len) {
      unsigned int padding = cel_len / 2;
      assert(padding > 1);
      std::cout << "|";
      pad(padding - 1);
      std::cout << str;
      pad(cel_len - padding);
    };

    // write the table title.
    unsigned int header_pad = (table_len - table_header.length()) / 2;
    write_middle(table_header, header_pad);
    std::cout << "|" << std::endl << " ";
    pad(table_len, '_');

    // write the data.
    unsigned int padding = 0;
    for (auto& row : table_data) {
      for (auto& str: row) {
        padding = ((cel_len + cel_padding * 2) - str.length()) / 2;
        write_middle(str, padding);
      }
      std::cout << "|" << std::endl;
    }
  };
};

#endif // PRINT_UTIL_TIME_H_
