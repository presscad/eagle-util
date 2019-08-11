#include <iostream>
#include <string>
#include <vector>
#include "../../../utils/all/common/common_utils.h"

std::vector<std::string> lines2, lines3, lines4;
bool read_data(std::string pathname)
{
    std::ifstream in(pathname.c_str());
    if (!in.good()) {
        false;
    }

    lines2.clear();
    lines3.clear();
    lines4.clear();

    std::string line;
    while (util::GetLine(in, line)) {
        util::TrimString(line);
        if (line.empty()) continue;
        char front = line.front();

        if (front == '#') continue;
        if (front != '2' && front != '3' && front != '4') {
            continue;
        }

        line = line.substr(1);
        util::TrimString(line);
        if (!line.empty()) {
            if (front == '2') {
                lines2.push_back(line);
            }
            else if (front == '3') {
                lines3.push_back(line);
            }
            else {
                lines4.push_back(line);
            }
        }
    }

    return true;
}

struct counts {
    int total{};
    int count_a{}, count_b{}, count_c{}, count_d{};
    int count_questions{};
};

bool count_choices(const std::vector<std::string>& lines, counts& result)
{
    result.count_questions = lines.size();

    for (auto& line : lines) {
        for (auto ch : line) {
            switch (ch) {
                case 'A':
                    result.count_a++;
                    break;
                case 'B':
                    result.count_b++;
                    break;
                case 'C':
                    result.count_c++;
                    break;
                case 'D':
                    result.count_d++;
                    break;
                default:
                    return false;
            }
            result.total++;
        }
    }

    return true;
}

void print_result(std::string title, const counts& r)
{
    std::cout << "----------------------------------" << std::endl;
    std::cout << title << std::endl;
    std::cout << "题目数：" << r.total << std::endl;
    std::cout << "A：" << 100.0 * r.count_a / r.total << "%, ";
    std::cout << "B：" << 100.0 * r.count_b / r.total << "%, ";
    std::cout << "C：" << 100.0 * r.count_c / r.total << "%, ";
    std::cout << "D：" << 100.0 * r.count_d / r.total << "%";

    std::cout << std::endl << std::endl;;
}

int main()
{
    if (!read_data("data/EnlishChoiceQuestions.txt")) {
        return 1;
    }

    counts r2, r3, r4;
    count_choices(lines2, r2);
    count_choices(lines3, r3);
    count_choices(lines4, r4);

    print_result("包含 A,B 选项题目", r2);
    print_result("包含 A,B,C 选项题目", r3);
    print_result("包含 A,B,C,D 选项题目", r4);

    return 0;
}
