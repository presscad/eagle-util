#ifndef _OPT_GZ_STREAM_H
#define _OPT_GZ_STREAM_H

#include <string>
#include <iostream>
#include <fstream>
#include <memory>
#include "zfstream.h"


namespace util {

static inline bool has_ext(const std::string &str, const std::string &ext)
{
    return str.size() >= ext.size() &&
        str.compare(str.size() - ext.size(), ext.size(), ext) == 0;
}

static inline std::shared_ptr<std::istream> get_opt_gz_istream(const std::string& filepath,
    std::ios_base::openmode mode = std::ios_base::in)
{
    std::shared_ptr<std::istream> stream;

    if (has_ext(filepath, ".gz")) {
        stream = std::make_shared<gzifstream>(filepath.c_str(), mode);
    }
    else {
        stream = std::make_shared<std::ifstream>(filepath.c_str(), mode);
    }

    return stream;
}

static inline std::shared_ptr<std::ostream> get_opt_gz_ostream(const std::string& filepath,
    std::ios_base::openmode mode = std::ios_base::binary | std::ios_base::out)
{
    std::shared_ptr<std::ostream> stream;

    if (has_ext(filepath, ".gz")) {
        stream = std::make_shared<gzofstream>(filepath.c_str(), mode);
    }
    else {
        stream = std::make_shared<std::ofstream>(filepath.c_str(), mode);
    }

    return stream;
}

} // end of namespace util

#endif //_OPT_GZ_STREAM_H
