#ifndef _UNI_TO_PINYIN_H
#define _UNI_TO_PINYIN_H

#include <string>

namespace util {

    std::string UnicodeToPinyin(const std::wstring& hanzi_str, bool each_first_cap, bool with_tone);

}

#endif // _UNI_TO_PINYIN_H
