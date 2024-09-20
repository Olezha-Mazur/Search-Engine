#include "SearchEngine.h"


std::set<size_t> SetIntersection(const std::set<size_t>& lhs, const std::set<size_t>& rhs) {
    std::set<size_t> nw;
    std::set_intersection(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), std::inserter(nw, nw.begin()));
    return nw;
}

std::set<size_t> SetUnion(const std::set<size_t>& lhs, const std::set<size_t>& rhs) {
    std::set<size_t> nw;
    std::set_union(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), std::inserter(nw, nw.begin()));
    return nw;
}

bool CompPair(const std::pair<it, it>& lhs, const std::pair<it, it>& rhs) {
    return (*(lhs.first)).first < (*(rhs.first)).first;
}

void ToVariintSearch(size_t num, std::string& var) {
    if (num == 0) var = char(0);
    while (num != 0) {
        uint8_t now = num - (num >> 7 << 7);
        num >>= 7;
        if (num == 0) {
            var += char(now);
        } else {
            var += char(128 + now);
        }
    }
}

size_t FromVatiintSearch(const std::vector<uint8_t>& bits) {
    size_t number = 0;
    for (size_t i = 0; i < bits.size(); ++i) {
        if ((bits[i] >> 7) % 2 == 0) {
            number += (bits[i] << (7 * i));
            break;
        } else {
            number += ((bits[i] - 128) << (7 * i));
        }
    }
    return number;
}

uint64_t GetNumberSearch(std::ifstream& file) {
    char byte;
    std::vector<uint8_t> buffer;
    file.read(&byte, 1);
    while (((uint8_t(byte) >> 7) % 2 == 1) && (!file.eof())) {
        buffer.push_back(byte);
        file.read(&byte, 1);
    }
    buffer.push_back(byte);
    uint64_t sz = FromVatiintSearch(buffer);
    return sz;
}

TermInfo1 GetTermInfo1(std::ifstream& t) {
    uint64_t sz = GetNumberSearch(t);
    std::string term;
    for (size_t i = 0; i < sz; ++i) {
        char byte;
        if (t.eof()) break;
        t.read(&byte, 1);
        term += byte;
    }
    if (t.eof()) return TermInfo1();
    uint64_t l_ind = GetNumberSearch(t);
    uint64_t p_ind = GetNumberSearch(t);
    return TermInfo1{term, l_ind, p_ind};
}

void Query::GetTerms(const std::vector<std::string>& parts, std::vector<std::string>& terms) {
    for (auto& elem: parts) {
        if  (elem != "OR" && elem != "AND" && elem != "(" && elem != ")") {
            terms.push_back(elem);
        }
    }
}

void Query::ReadTerms(std::set<std::string>& terms) {
    std::ifstream t(fs::path("../BD/terminfo"), std::ios::binary);
    TermInfo1 now = GetTermInfo1(t);
    TermInfo1 next = GetTermInfo1(t);
    while (!t.eof()) {
        if (std::find(terms.begin(), terms.end(), now.term) != terms.end()) {
            std::ifstream l(fs::path("../BD/lists"), std::ios::binary); 
            l.seekg(now.l_ind, std::ios_base::beg);
            std::vector pr_tf = {0};
            for (size_t i = 0; i < ((next.l_ind - now.l_ind) / 2); ++i) {
                size_t key = GetNumberSearch(l);
                size_t value = GetNumberSearch(l);
                if (query[now.term].first.find(key) != query[now.term].first.end()) {
                    query[now.term].first[key] += value;
                } else {
                    query[now.term].first[key] = value;
                }
            }
            for (auto& [key, value] : query[now.term].first) {
                pr_tf.push_back(value + pr_tf.back());
            }
            
            std::ifstream p(fs::path("../BD/pos"), std::ios::binary); 
            p.seekg(now.p_ind, std::ios_base::beg);
            for (size_t i = 0; i < (next.p_ind - now.p_ind); ++i) {
                if (std::find(pr_tf.begin(), pr_tf.end(), i) != pr_tf.end()) {
                    query[now.term].second.push_back(GetNumberSearch(p));
                } else {
                    query[now.term].second.push_back(GetNumberSearch(p) + query[now.term].second.back());
                }
            }
        }

        now = next;
        next = GetTermInfo1(t);
    }
}

void Query::SimulateQuery() {
    using it = std::map<size_t, size_t>::iterator;
    std::vector<std::pair<it, it>> evaluate;
    if (need_DID.empty()) return;
    for (auto& [key, value]: query) {
        auto elem = value.first.begin();
        bool flag = true;
        while ((elem != value.first.end()) && (std::find(need_DID.begin(), need_DID.end(), (*elem).first) == need_DID.end())) {
            ++elem;
        }
        if (elem != value.first.end()) {
            evaluate.push_back(std::make_pair(elem, value.first.end()));
        }
    }
    while (!evaluate.empty()) {
        std::sort(evaluate.begin(), evaluate.end(), CompPair);

        auto curr = (*(evaluate[0].first)).first;
        size_t ind = 0;
        while ((*(evaluate[ind].first)).first == curr) {
            CountScore((*(evaluate[ind].first)).second, (*(evaluate[ind].first)).first, need_DID.size());

            ++(evaluate[ind].first);
            if (evaluate[ind].first == evaluate[ind].second) {
                evaluate.erase(evaluate.begin() + ind);
            }
            if (evaluate.empty()) break; 
            while ((evaluate[ind].first != evaluate[ind].second) && (std::find(need_DID.begin(), need_DID.end(), (*(evaluate[ind].first)).first) == need_DID.end())) {
                ++(evaluate[ind].first);
            }
            if (evaluate[ind].first == evaluate[ind].second) {
                evaluate.erase(evaluate.begin() + ind);
            }
            if (evaluate.empty()) break; 
            ++ind;
            if (ind == evaluate.size()) break;
        }
    }
}

void Query::ParseQuery(std::string& tokens) {
    std::vector<std::string> parts;
    std::set<std::string> terms;
    std::smatch mat;
    std::regex expression("\\w+|\\S");
    while (std::regex_search(tokens, mat, expression)) {
        std::string token = mat.str();
        parts.push_back(token);
        if (token != "OR" && token != "AND" && token != "(" && token != ")") {
            terms.insert(token);
        }
        tokens = mat.suffix();
    }
    ReadTerms(terms);
    QueryParser parser(parts);
    if (parser.Parse(this) != nullptr) {
        need_DID = parser.Parse(this)->combine();
    } else {
        std::cout << "Invalid query\n";
    }
    GetSizeInfo();
    SimulateQuery();
    heap.Show();
}

void Query::CountScore(size_t tf, size_t DID, size_t df) {
    double IDF = log2f((N - df + 0.5) / (df + 0.5));
    double num = (tf * (k + 1)) / (tf + k * (1 - b + b * GetDocSize(DID) * N / sm));
    double func = num * IDF;
    heap.insert(std::make_pair(GetDocName(DID), func));
}

std::string Query::GetDocName(size_t DID) {
    std::ifstream file(fs::path("../BD/docs"));
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string word;
        int ind = 0;
        size_t curr_DID;
        while (iss >> word) {
            if (ind == 0) curr_DID = std::stoull(word);
            if ((ind == 1) && (curr_DID == DID)) return word;
            ++ind;
        }
    }
    return "";
}

size_t Query::GetDocSize(size_t DID) {
    std::ifstream file(fs::path("../BD/docs"));
    std::string line;
    size_t curr_DID;
    while (std::getline(file, line)) {
        std::stringstream iss(line);
        std::string word;
        iss >> word;
        curr_DID = std::stoull(word);

        iss >> word;
        iss >> word;
        if (DID == curr_DID) {
            return std::stoull(word);
        }
    }
    return 0;
}

void Query::GetSizeInfo() {
    std::ifstream file(fs::path("../BD/size"));
    std::string line;
    std::getline(file, line);
    std::stringstream iss(line);
    std::string word;
    iss >> word;
    N = std::stoull(word);
    iss >> word;
    sm = std::stoull(word);
}

void Query::GetQuery() {
    bool flag = true;
    while (flag) {
        std::cout << "Enter a query, please:\n";
        std::string q;
        std::getline(std::cin, q);
        if (q == "ESC") { flag = false; }
        heap.clear();
        query.clear();
        need_DID.clear();
        ParseQuery(q);
    }
}

void Query::Heap::insert(const value_type& pair) {
    if (docs_.find(pair.first) != docs_.end()) {
        docs_[pair.first] += pair.second;
        return;
    }
    if (queue_.size() < size) {
        queue_.push(pair);
        docs_[pair.first] = pair.second;
    } else if (pair.second > queue_.top().second) {
        docs_.erase(queue_.top().first);
        queue_.pop();
        queue_.push(pair);
        docs_[pair.first] = pair.second;
    }
}

void Query::Heap::Show() {
    std::stack<value_type> st;
    while (!queue_.empty()) {
        st.push(queue_.top());
        queue_.pop();
    }
    while (!st.empty()) {
        std::cout << st.top().first << "\t" << st.top().second << "\n";
        st.pop();
    }
    std::cout << "\n\n";
}

void Query::Heap::clear() {
    while (!queue_.empty()) {
        queue_.pop();
    }
    docs_.clear();
}

Query::Operand::Operand(std::string& val, Query* q) {
    for (auto [key, value] : q->query[val].first) {
        st.insert(key);
    }
}

bool Query::QueryParser::BuildExp(std::stack<std::string>& op, std::stack<std::shared_ptr<Node>>& words) {
    while (!op.empty()) {
        if (op.empty()) return false;
        std::string type = op.top();
        op.pop();
        if (words.empty()) return false;
        std::shared_ptr<Node> rhs = words.top();
        words.pop();
        if (words.empty()) return false;
        std::shared_ptr<Node> lhs = words.top();
        words.pop();
        words.push(std::make_shared<Operator>(type, lhs, rhs));
    }
    return true;
}

std::shared_ptr<Query::Node> Query::QueryParser::ParseExpression(size_t begin_pos, size_t end_pos, Query* q) {

    std::stack<std::string> op;
    std::stack<std::shared_ptr<Node>> words;
    for (size_t ind = begin_pos; ind < end_pos; ++ind) {
        if (exp[ind] == "AND" || exp[ind] == "OR") {
            op.push(exp[ind]);
        } else if (exp[ind] == "(") {
            size_t i = ind + 1;
            size_t cnt_brackets = 1;
            while (cnt_brackets > 0) {
                if (exp[i] == ")") {
                    --cnt_brackets;
                } else if (exp[i] == "(") {
                    ++cnt_brackets;
                }
                ++i;
            }
            words.push(ParseExpression(ind + 1, i - 1, q));
            ind = i - 1;
        } else if (exp[ind] == ")") {
            if (!BuildExp(op, words)) return nullptr;
        } else {
            words.push(std::make_shared<Operand>(exp[ind], q));
        }
    }
    if (!BuildExp(op, words)) return nullptr;
    return words.top();
}

void ParseSearch(int argc, char **argv) {
    size_t count = 5;
    bool is_need_search = false;
    for (size_t i = 0; i < argc; ++i) {
        std::string elem = argv[i];
        if (elem == "--search") {
            is_need_search = true;
        } else if (elem == "--count") {
            std::string cnt = argv[i + 1];
            count = std::stoull(cnt);
        }
    }
    Query query(count);
    query.GetQuery();
}

