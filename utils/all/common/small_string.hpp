#ifndef _SMALL_STRING_H
#define _SMALL_STRING_H

#include <string>
#include <boost/container/small_vector.hpp>

namespace util {

    template <std::size_t N>
    class SmallString
    {
    public:
        SmallString() : data_(1, '\0')
        {}
        explicit SmallString(const char *src)
        {
            const std::size_t str_len = std::strlen(src);
            data_.resize(str_len + 1);
            data_[str_len] = '\0';
            std::strncpy(&data_[0], src, str_len);
        }
        explicit SmallString(const std::string& src)
        {
            const std::size_t str_len = src.size();
            data_.resize(str_len + 1);
            data_[str_len] = '\0';
            std::strncpy(&data_[0], src.c_str(), str_len);
        }

        bool empty() const
        {
            return data_.size() == 1;
        }
        void reserve(std::size_t count)
        {
            data_.reserve(count + 1);
        }

        char& front()
        {
            return data_.front();
        }
        const char& front() const
        {
            return data_.front();
        }
        char& back()
        {
            return data_[data_.size() - 2];
        }
        char& back() const
        {
            return data_[data_.size() - 2];
        }

        const char *c_str() const
        {
            return (const char *)&data_[0];
        }

        std::size_t size() const
        {
            return data_.size() - 1;
        }

        void clear()
        {
            data_.resize(1);
            data_[0] = '\0';
        }

        SmallString& operator+=(char ch)
        {
            data_.back() = ch;
            data_.push_back('\0');
            return *this;
        }
        SmallString& operator+=(const SmallString& src)
        {
            const auto str_len1 = data_.size() - 1;
            const auto str_len2 = src.data_.size() - 1;
            data_.resize(str_len1 + str_len2 + 1);
            data_[str_len1 + str_len2] = '\0';
            std::strncpy(&data_[0] + str_len1, src.c_str(), str_len2);
            return *this;
        }
        SmallString& operator+=(const char* src)
        {
            const auto str_len1 = data_.size() - 1;
            const auto str_len2 = std::strlen(src);
            data_.resize(str_len1 + str_len2 + 1);
            data_[str_len1 + str_len2] = '\0';
            std::strncpy(&data_[0] + str_len1, src, str_len2);
            return *this;
        }
        SmallString& operator+=(const std::string& src)
        {
            const auto str_len1 = data_.size() - 1;
            const auto str_len2 = src.size();
            data_.resize(str_len1 + str_len2 + 1);
            data_[str_len1 + str_len2] = '\0';
            std::strncpy(&data_[0] + str_len1, src.c_str(), str_len2);
            return *this;
        }

    protected:
        boost::container::small_vector<char, N> data_;
    };

}

#endif //_SMALL_STRING_H
