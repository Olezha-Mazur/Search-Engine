#pragma once
#include <iostream>
#include <regex>
#include <memory>
#include <queue>
#include <stack>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <cstdint>
#include <map>

namespace fs = std::filesystem;

static const uint16_t k = 2;
static const float b = 0.75f;

using value_type = std::pair<std::string, float>;

class Compare {
public:
    bool operator()(const value_type& lhs, const value_type& rhs) {
        return (lhs.second > rhs.second) ? true : false;
    }
};

std::set<size_t> SetIntersection(const std::set<size_t>& lhs, const std::set<size_t>& rhs);

std::set<size_t> SetUnion(const std::set<size_t>& lhs, const std::set<size_t>& rhs);

using it = std::map<size_t, size_t>::iterator;

bool CompPair(const std::pair<it, it>& lhs, const std::pair<it, it>& rhs);

bool Comp(const std::pair<size_t, size_t>& pair1, const std::pair<size_t, size_t>& pair2) {
    return pair1.first < pair2.first;
}

struct TermInfo1 {
    std::string term;
    size_t l_ind;
    size_t p_ind;

    bool empty() {
        return (term.size() == 0) ? 1 : 0;
    }
};

void ToVariintSearch(size_t num, std::string& var);

size_t FromVatiintSearch(const std::vector<uint8_t>& bits);

uint64_t GetNumberSearch(std::ifstream& file);

TermInfo1 GetTermInfo1(std::ifstream& t);

class Query {
public:
    Query(size_t cnt) {
        heap.size = cnt;
    }

    void GetTerms(const std::vector<std::string>& parts, std::vector<std::string>& terms);

    void ReadTerms(std::set<std::string>& terms);

    void SimulateQuery();

    void ParseQuery(std::string& tokens);

    void CountScore(size_t tf, size_t DID, size_t df);

    std::string GetDocName(size_t DID);

    size_t GetDocSize(size_t DID);

    void GetSizeInfo();

    void GetQuery();

    class Heap {
    public:
        void insert(const value_type& pair);

        void Show();

        void clear();

        int size = 5;

    private:
        std::priority_queue<value_type, std::vector<value_type>, Compare> queue_;
        std::unordered_map<std::string, float> docs_;
    };


private:
    class Node {
    public:
        virtual std::set<size_t> combine() const = 0;
    };

    class Operand : public Node {
    public:
        Operand(std::string& val, Query* q);

        virtual std::set<size_t> combine() const override {
            return st;
        }

    private:
        std::set<size_t> st;
    };

    class Operator : public Node {
    public:
        Operator(std::string& type, std::shared_ptr<Node> lhs, std::shared_ptr<Node> rhs)
            : type(type)
            , lhs(lhs)
            , rhs(rhs) 
        {}

        virtual std::set<size_t> combine() const override {
            if (type == "AND") {
                return SetIntersection(lhs->combine(), rhs->combine());
            } else if (type == "OR") {
                return SetUnion(lhs->combine(), rhs->combine());;
            } else {
                throw std::invalid_argument("Invalid symbol");
            }
        }

    private:
        std::string type;
        std::shared_ptr<Node> lhs;
        std::shared_ptr<Node> rhs;
    };

    class QueryParser {
    public:
        QueryParser(const std::vector<std::string>& exp) : exp(exp) {}

        std::shared_ptr<Node> Parse(Query* q) {
            return ParseExpression(0, exp.size(), q);
        }

    private:
        bool BuildExp(std::stack<std::string>& op, std::stack<std::shared_ptr<Node>>& words);

        std::shared_ptr<Node> ParseExpression(size_t begin_pos, size_t end_pos, Query* q);

        std::vector<std::string> exp;
    };

    std::string q_;
    size_t N;
    size_t sm;
    Heap heap;
    std::set<size_t> need_DID;
    std::unordered_map<std::string, std::pair<std::map<size_t, size_t>, std::vector<size_t>>> query;
};

void ParseSearch(int argc, char **argv);