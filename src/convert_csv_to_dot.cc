#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

using std::cerr;
using std::cout;
using std::endl;
using std::exception;
using std::getline;
using std::ifstream;
using std::map;
using std::runtime_error;
using std::set;
using std::string;
using std::to_string;
using std::tuple;
using std::vector;

struct CsvGraph
{
    map<string, string> vertex_labels;
    vector<tuple<string, string, string>> edges;
    bool seen_directed_edge;
};

auto escape_quotes(const string & s) -> string
{
    string result;
    for (char c : s) {
        if (c == '"' || c == '\\')
            result += '\\';
        result += c;
    }
    return result;
}

auto parse_csv(ifstream & infile, const string & filename) -> CsvGraph
{
    CsvGraph graph;
    graph.seen_directed_edge = false;

    string line;
    unsigned line_number = 0;

    while (getline(infile, line)) {
        ++line_number;

        auto pos = line.find_first_of(",>");
        if (string::npos == pos)
            throw runtime_error(filename + ": line " + to_string(line_number) + ": expected a comma but didn't get one");

        string left = line.substr(0, pos), right = line.substr(pos + 1), label;
        char delim = line.at(pos);

        auto pos2 = right.find(',');
        if (string::npos != pos2) {
            label = right.substr(pos2 + 1);
            right = right.substr(0, pos2);
        }

        if (delim == '>')
            graph.seen_directed_edge = true;

        if (right.empty() && ! left.empty()) {
            if (! label.empty())
                graph.vertex_labels[left] = label;
        }
        else {
            graph.edges.emplace_back(left, right, label);
        }
    }

    return graph;
}

auto main(int argc, char * argv[]) -> int
{
    try {
        if (argc < 2) {
            cerr << "Usage: " << argv[0] << " <csv-file>" << endl;
            return EXIT_FAILURE;
        }

        ifstream infile(argv[1]);
        if (! infile) {
            cerr << "Error: cannot open file '" << argv[1] << "'" << endl;
            return EXIT_FAILURE;
        }

        auto graph = parse_csv(infile, argv[1]);
        infile.close();

        cout << (graph.seen_directed_edge ? "digraph G {" : "graph G {") << endl;

        for (auto & [v, label] : graph.vertex_labels)
            cout << "    \"" << escape_quotes(v) << "\" [label=\"" << escape_quotes(label) << "\"];" << endl;

        for (auto & [from, to, label] : graph.edges) {
            cout << "    \"" << escape_quotes(from) << "\" " << (graph.seen_directed_edge ? "->" : "--") << " \"" << escape_quotes(to) << "\"";
            if (! label.empty())
                cout << " [label=\"" << escape_quotes(label) << "\"]";
            cout << ";" << endl;
        }

        cout << "}" << endl;

        return EXIT_SUCCESS;
    }
    catch (const exception & e) {
        cerr << "Error: " << e.what() << endl;
        return EXIT_FAILURE;
    }
}
