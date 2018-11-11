/*----------------------------------------------------------------------*
 * Copyright(c) 2015 SAP SE. All rights reserved
 * Author      : SAP Custom Development
 * Description : Common utility functions header file
 *----------------------------------------------------------------------*
 * Change - History : Change history
 * Developer  Date      Description
 * I078212    20161206  Initial creation
 *----------------------------------------------------------------------*/

#ifndef _UNI_TO_PINYIN_H
#define _UNI_TO_PINYIN_H

#include <string>

namespace util {

    std::string UnicodeToPinyin(const std::wstring& hanzi_str, bool each_first_cap, bool with_tone);

}

#endif // _UNI_TO_PINYIN_H
