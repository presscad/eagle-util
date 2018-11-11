#if defined(_WIN32) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif


#include "common_utils.h"
#include <cmath>
extern "C" {
#include "iniparser.h"
}

using namespace std;

namespace util {

dictionary * IniInitDict(const string& ini_name)
{
    return iniparser_load(ini_name.c_str());
}

void IniFreeDict(dictionary * dict)
{
    iniparser_freedict((struct _dictionary_ *)dict);
}

bool ReadIniString(dictionary *dict, const char *section, string &str, string &err)
{
    char *value = iniparser_getstring((struct _dictionary_ *)dict, section, nullptr);
    if (nullptr == value) {
        char buff[128];
        snprintf(buff, sizeof(buff)-1, "cannot find %s in config file", section);
        err = buff;
        return false;
    }
    str = value;
    return true;
}

bool ReadIniInt(dictionary *dict, const char*section, int &value, string &err)
{
    value = iniparser_getint((struct _dictionary_ *)dict, section, 0xCDCDCDCD);
    if ((unsigned int)value == 0xCDCDCDCD) {
        char buff[128];
        snprintf(buff, sizeof(buff)-1, "cannot find %s in config file", section);
        err = buff;
        return false;
    }
    return true;
}

bool ReadIniDouble(dictionary *dict, const char *section, double &value, string &err)
{
    string str;
    if (!ReadIniString(dict, section, str, err)) {
        return false;
    }

    value = atof(str.c_str());
    return true;
}

bool ReadIniInt(dictionary *dict, const string& section, int &value, string &err)
{
    return ReadIniInt(dict, section.c_str(), value, err);
}

bool ReadIniDouble(dictionary *dict, const string& section, double &value, string &err)
{
    return ReadIniDouble(dict, section.c_str(), value, err);
}

bool ReadIniString(dictionary *dict, const string& section, string &value, string &err)
{
    return ReadIniString(dict, section.c_str(), value, err);
}

bool WriteDictString(dictionary *dict, const char *section, const string &str,
    string &err)
{
    int r = iniparser_set((struct _dictionary_ *)dict, section, str.c_str());
    if (-1 == r) {
        char buff[128];
        snprintf(buff, sizeof(buff) - 1, "cannot write %s into config file", section);
        err = buff;
        return false;
    }
    return true;
}


static bool PopulateSections(vector<string>& lines, vector<string>& sections, string& err)
{
    for (auto& line : lines) {
        util::RightTrimString(line);
    }

    sections.reserve(lines.size());
    string cur_section;
    for (int i = 0; i < (int)lines.size(); ++i) {
        auto& line = lines[i];
        if (i > 0) {
            if (!lines[i - 1].empty() && lines[i - 1].back() == '\\') {
                sections.push_back(cur_section);
                continue;
            }
        }

        util::LeftTrimString(line);
        if (line.empty()) {
            sections.push_back(cur_section);
            continue;
        }

        if (line.front() == '[' && line.back() == ']') {
            cur_section = line.substr(1, line.size() - 2);
            util::TrimString(cur_section);
        }
        sections.push_back(cur_section);
    }
    return true;
}

bool WriteSectionToFile(const string& ini_pathname, const char *section,
    const string&str, string& err)
{
    vector<string> sec_params;
    util::ParseCsvLine(sec_params, section, ':');
    if (sec_params.size() != 2) {
        err = "WriteSectionToFile: invalid parameter section - " + string(section);
        return false;
    }
    if (sec_params.front().empty() || sec_params.back().empty()) {
        err = "WriteSectionToFile: invalid parameter section - " + string(section);
        return false;
    }

    string data;
    if (false == util::ReadAllFromFile(ini_pathname, data)) {
        err = "cannot read from file " + ini_pathname;
        return false;
    }
    auto lines = util::StringSplit(data, '\n');
    vector<string> sections;
    if (false == PopulateSections(lines, sections, err)) {
        return false;
    }

    bool found = false;
    vector<string> kv;
    // to find the existing section and key
    for (int i = 0; i < (int)lines.size(); ++i) {
        if (sections[i] == sec_params.front()) {
            util::ParseCsvLine(kv, lines[i], '=');
            if (kv.size() == 2 && kv.front() == sec_params.back()) {
                found = true;

                if (lines[i].back() == '\\') {
                    // remove the lines continued by '\'
                    int j = i + 1;
                    for (; j < (int)lines.size(); ++j) {
                        if (lines[j].empty()) break;
                        if (lines[j].back() != '\\') {
                            break;
                        }
                    }
                    lines.erase(lines.begin() + i + 1, lines.begin() + j + 1);
                }

                // replace the line
                lines[i] = sec_params.back() + " = " + str;
                break;
            }
        }
    }

    if (!found) {
        // append to the existing section
        for (int i = 0; i < (int)lines.size() - 1; ++i) {
            if (sections[i] == sec_params.front() && sections[i + 1] != sec_params.front()) {
                found = true;
                // back to the last meaningful entry in the section
                while (i > 0 && sections[i] == sec_params.front() && (lines[i].empty() || lines[i].front() == '#')) {
                    --i;
                }
                lines.insert(lines.begin() + i + 1, sec_params.back() + " = " + str);
                break;
            }
        }
    }

    if (!found) {
        // append to the end of the file
        if (sections.back() != sec_params.front()) {
            // never found the section
            lines.push_back("[" + sec_params.front() + "]");
        }
        lines.push_back(sec_params.back() + " = " + str);
    }

    // write to file
    data.clear();
    for (size_t i = 0; i < lines.size(); ++i) {
        data += lines[i];
        if (i != lines.size() - 1) {
            data += '\n';
        }
        else if (!lines[i].empty()) {
            data += '\n';
        }
    }
    if (false == util::WriteStrToFile(ini_pathname, data)) {
        err = "unable to write to file: " + ini_pathname;
        return false;
    }

    return true;
}

}
