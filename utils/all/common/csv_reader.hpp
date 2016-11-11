#ifndef _CSV_READER_H
#define _CSV_READER_H

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#if (COMM_UTIL_WITH_ZLIB == 1)
#include "zfstream.h"
#endif
#include "common/common_utils.h"


namespace util {

class csv_reader
{
public:
    explicit csv_reader(const std::string &csv_path_name)
        : csv_path_name_(csv_path_name),
        istream_(std::make_shared<std::ifstream>(csv_path_name.c_str()))
    {}

    explicit csv_reader(const std::string &csv_path_name, const std::shared_ptr<std::istream>& p_is)
        : csv_path_name_(csv_path_name), istream_(p_is)
    {}

    bool good() const
    {
        return (istream_ != nullptr) && (istream_->good());
    }

    const std::string& get_pathname() const
    {
        return csv_path_name_;
    }

    std::istream& get_istream() const
    {
        return *istream_;
    }

    size_t get_lines(std::vector<std::string> &lines, size_t num)
    {
        std::string line;
        lines.clear();
        lines.reserve(num);
        for (size_t i = 0; i < num; i++) {
            if (get_line(line) == false) {
                break;
            }
            lines.push_back(line);
        }
        return lines.size();
    }

    bool get_line(std::string &line)
    {
        if (istream_ == nullptr) {
            return false;
        }

        line.clear();
        do {
            if (getline(*istream_, line)) {
                if (istream_->good() && line.empty()) {
                    continue;
                }
                return true;
            } else {
                return false;
            }
        } while (true);

        return false;
    }

    // batch read lines using by getting a block
    size_t batch_read_lines(std::vector<char *> &lines, size_t block_size)
    {
        lines.clear();
        batch_buff_.resize(block_size + 1024);

        istream_->read((char *)batch_buff_.data(), block_size);
        batch_buff_.resize(istream_->gcount());

        if (batch_buff_.size() > 0) {
            auto v = istream_->peek();
            if (v != '\n' && v != EOF) {
                // if not ending with "\n", so get additional line to make sure it
                std::string line;
                if (true == get_line(line)) {
                    batch_buff_.append(line);
                }
            }

            util::ParseCsvLineInPlace(lines, (char *)batch_buff_.data(), '\n', true);
        }

        return lines.size();
    }

protected:
    std::string csv_path_name_;
    std::shared_ptr<std::istream> istream_;
    std::string batch_buff_;
};

#if (COMM_UTIL_WITH_ZLIB == 1)
class gz_csv_reader : public csv_reader
{
public:
    gz_csv_reader(const std::string &csv_path_name)
        : csv_reader(csv_path_name, std::make_shared<gzifstream>(csv_path_name.c_str()))
    {}
};
#endif

static inline csv_reader* get_csv_reader(const std::string &csv)
{
    if (csv.rfind(".gz") != std::string::npos) {
#if (COMM_UTIL_WITH_ZLIB == 1)
        return new gz_csv_reader(csv);
#else
        return nullptr;
#endif
    }
    return new csv_reader(csv);
}

}

#endif //_CSV_READER_H
