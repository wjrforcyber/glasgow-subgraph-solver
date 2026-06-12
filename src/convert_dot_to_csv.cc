#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::exception;
using std::getline;
using std::ifstream;
using std::make_pair;
using std::map;
using std::pair;
using std::set;
using std::string;
using std::vector;

struct Edge
{
    string from;
    string to;
    string label;
    bool directed;
};

struct DotGraph
{
    bool directed;
    vector<pair<string, string>> vertex_labels;
    vector<Edge> edges;
};

auto unquote(const string & s) -> string
{
    if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\'')))
        return s.substr(1, s.size() - 2);
    return s;
}

auto unhtml(const string & s) -> string
{
    string result;
    for (size_t i = 0; i < s.size(); ++i) {
        if (i + 3 < s.size() && s[i] == '&' && s[i + 1] == '#') {
            size_t semi = s.find(';', i);
            if (semi != string::npos) {
                auto num = s.substr(i + 2, semi - i - 2);
                try {
                    int code = std::stoi(num);
                    result += static_cast<char>(code);
                }
                catch (...) {
                    result += s.substr(i, semi - i + 1);
                }
                i = semi;
                continue;
            }
        }
        result += s[i];
    }
    return result;
}

auto extract_label(const string & attrs) -> string
{
    std::regex label_re(R"raw(label\s*=\s*"([^"]*)")raw");
    std::smatch match;
    if (std::regex_search(attrs, match, label_re))
        return unhtml(match[1].str());

    std::regex label_html_re(R"raw(label\s*=\s*<([^>]*)>)raw");
    if (std::regex_search(attrs, match, label_html_re))
        return unhtml(match[1].str());

    return "";
}

auto parse_dot(ifstream & infile) -> DotGraph
{
    DotGraph graph;
    graph.directed = false;
    set<string> declared_vertices;
    string line;

    while (getline(infile, line)) {
        string trimmed;
        auto start = line.find_first_not_of(" \t\r\n");
        auto end_pos = line.find_last_not_of(" \t\r\n");
        if (start == string::npos)
            continue;
        trimmed = line.substr(start, end_pos - start + 1);

        if (trimmed.rfind("digraph", 0) == 0) {
            graph.directed = true;
            continue;
        }
        if (trimmed.rfind("graph", 0) == 0) {
            size_t skip = 5;
            while (skip < trimmed.size() && (trimmed[skip] == ' ' || trimmed[skip] == '{' || trimmed[skip] == '_' || trimmed[skip] == '-' || (trimmed[skip] >= 'a' && trimmed[skip] <= 'z') || (trimmed[skip] >= 'A' && trimmed[skip] <= 'Z') || (trimmed[skip] >= '0' && trimmed[skip] <= '9')))
                ++skip;
            continue;
        }
        if (trimmed == "{" || trimmed == "}" || trimmed.rfind("//", 0) == 0 || trimmed.rfind("/*", 0) == 0 || trimmed.rfind("#", 0) == 0)
            continue;

        if (trimmed.find("--") != string::npos) {
            graph.directed = false;
            auto pos = trimmed.find("--");
            string left = trimmed.substr(0, pos);
            string right_and_attrs = trimmed.substr(pos + 2);

            auto bracket = right_and_attrs.find('[');
            string right, attrs;
            if (bracket != string::npos) {
                right = right_and_attrs.substr(0, bracket);
                auto close_b = right_and_attrs.rfind(']');
                if (close_b != string::npos)
                    attrs = right_and_attrs.substr(bracket + 1, close_b - bracket - 1);
            }
            else {
                auto semi = right_and_attrs.find(';');
                right = (semi != string::npos) ? right_and_attrs.substr(0, semi) : right_and_attrs;
            }

            auto trim = [](string & s) {
                auto a = s.find_first_not_of(" \t");
                auto b = s.find_last_not_of(" \t");
                if (a == string::npos)
                    s = "";
                else
                    s = s.substr(a, b - a + 1);
            };
            trim(left);
            trim(right);

            if (left.empty() || right.empty())
                continue;

            left = unquote(left);
            right = unquote(right);

            string label = extract_label(attrs);
            graph.edges.push_back({left, right, label, false});
            declared_vertices.insert(left);
            declared_vertices.insert(right);
        }
        else if (trimmed.find("->") != string::npos) {
            graph.directed = true;
            auto pos = trimmed.find("->");
            string left = trimmed.substr(0, pos);
            string right_and_attrs = trimmed.substr(pos + 2);

            auto bracket = right_and_attrs.find('[');
            string right, attrs;
            if (bracket != string::npos) {
                right = right_and_attrs.substr(0, bracket);
                auto close_b = right_and_attrs.rfind(']');
                if (close_b != string::npos)
                    attrs = right_and_attrs.substr(bracket + 1, close_b - bracket - 1);
            }
            else {
                auto semi = right_and_attrs.find(';');
                right = (semi != string::npos) ? right_and_attrs.substr(0, semi) : right_and_attrs;
            }

            auto trim = [](string & s) {
                auto a = s.find_first_not_of(" \t");
                auto b = s.find_last_not_of(" \t");
                if (a == string::npos)
                    s = "";
                else
                    s = s.substr(a, b - a + 1);
            };
            trim(left);
            trim(right);

            if (left.empty() || right.empty())
                continue;

            left = unquote(left);
            right = unquote(right);

            string label = extract_label(attrs);
            graph.edges.push_back({left, right, label, true});
            declared_vertices.insert(left);
            declared_vertices.insert(right);
        }
        else if (trimmed.find('[') != string::npos) {
            auto bracket = trimmed.find('[');
            string node_name = trimmed.substr(0, bracket);
            auto close_b = trimmed.rfind(']');
            string attrs;
            if (close_b != string::npos)
                attrs = trimmed.substr(bracket + 1, close_b - bracket - 1);

            auto trim = [](string & s) {
                auto a = s.find_first_not_of(" \t");
                auto b = s.find_last_not_of(" \t");
                if (a == string::npos)
                    s = "";
                else
                    s = s.substr(a, b - a + 1);
            };
            trim(node_name);

            if (node_name.empty())
                continue;

            node_name = unquote(node_name);

            string label = extract_label(attrs);
            if (! label.empty()) {
                graph.vertex_labels.emplace_back(node_name, label);
                declared_vertices.insert(node_name);
            }
        }
    }

    return graph;
}

auto main(int argc, char * argv[]) -> int
{
    try {
        if (argc < 2) {
            cerr << "Usage: " << argv[0] << " <dot-file>" << endl;
            return EXIT_FAILURE;
        }

        ifstream infile(argv[1]);
        if (! infile) {
            cerr << "Error: cannot open file '" << argv[1] << "'" << endl;
            return EXIT_FAILURE;
        }

        auto graph = parse_dot(infile);
        infile.close();

        for (auto & [v, label] : graph.vertex_labels)
            cout << v << ",," << label << endl;

        for (auto & edge : graph.edges) {
            char delim = edge.directed ? '>' : ',';
            cout << edge.from << delim << edge.to;
            if (! edge.label.empty())
                cout << "," << edge.label;
            cout << endl;
        }

        return EXIT_SUCCESS;
    }
    catch (const exception & e) {
        cerr << "Error: " << e.what() << endl;
        return EXIT_FAILURE;
    }
}
