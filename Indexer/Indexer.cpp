#include "Indexer.h"

void ToVariint(size_t num, std::string& var) {
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

size_t FromVatiint(const std::vector<uint8_t>& bits) {
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

uint64_t GetNumber(std::ifstream& file) {
    char byte;
    std::vector<uint8_t> buffer;
    file.read(&byte, 1);
    while (((uint8_t(byte) >> 7) % 2 == 1) && (!file.eof())) {
        buffer.push_back(byte);
        file.read(&byte, 1);
    }
    buffer.push_back(byte);
    uint64_t sz = FromVatiint(buffer);
    return sz;
}

TermInfo GetTermInfo(std::ifstream& t) {
    uint64_t sz = GetNumber(t);
    std::string term;
    for (size_t i = 0; i < sz; ++i) {
        char byte;
        if (t.eof()) break;
        t.read(&byte, 1);
        term += byte;
    }
    if (t.eof()) return TermInfo();
    uint64_t l_ind = GetNumber(t);
    uint64_t p_ind = GetNumber(t);
    return TermInfo{term, l_ind, p_ind};
}

void DataBase::Init() {
    fs::create_directories(terminfo_.parent_path());
    std::ofstream terminfo(terminfo_);
    std::ofstream pos(pos_);
    std::ofstream lists(lists_);
    std::ofstream docs(docs_);
}

size_t DataBase::Size() {
    size_t size = 0;
    for (auto& elem: terms) {
        size += sizeof(elem.first);
        size += sizeof(std::pair<size_t, size_t>) * elem.second.posting.size();
        size += sizeof(size_t) * elem.second.pos.size();
    }
    return size;
}

void DataBase::SavePosition(const std::vector<size_t>& elem, std::ofstream& p) {
    size_t pref = 0;
    std::string line;
    for (size_t i = 0; i < elem.size(); ++i) {
        line.clear();
        if (((elem[i] - pref) >> (63)) % 2 == 0) {
            ToVariint(elem[i] - pref, line);
            end_pos += line.size();
        } else {
            ToVariint(elem[i], line);
            end_pos += line.size();
        }
        pref = elem[i];
        p << line;
    }
}

void DataBase::SaveTermInfo(std::ofstream& i, const std::string& str) {
    std::string buffer;
    ToVariint(str.size(), buffer);
    i << buffer; 
    i << str;    

    buffer.clear();
    ToVariint(end_lists, buffer);
    i << buffer; 

    buffer.clear();
    ToVariint(end_pos, buffer);
    i << buffer;
}

void DataBase::SavePostingLists(const std::map<size_t, size_t>& elem, std::ofstream& l) {
    for (const auto& [key, value] : elem) {
        std::string line;
        ToVariint(key, line);
        end_lists += line.size();
        l << line;

        line.clear();
        ToVariint(value, line);
        end_lists += line.size();
        l << line;
    }
}

void DataBase::Save() {
    std::ofstream i(terminfo_, std::ios::app | std::ios::binary);
    std::ofstream p(pos_, std::ios::app | std::ios::binary);
    std::ofstream l(lists_, std::ios::app | std::ios::binary);
    for (auto elem: terms) {
        SaveTermInfo(i, elem.first);

        SavePostingLists(elem.second.posting, l);

        SavePosition(elem.second.pos, p);
    }
    i.close();
    p.close();
    l.close();
}

void DataBase::WriteWord(std::string& word, size_t DID, size_t pos) {
    if (Size() <= 1024 * 1024) {
        if (terms[word].posting.find(DID) != terms[word].posting.end()) {
            ++terms[word].posting[DID];
        } else {
            terms[word].posting[DID] = 1;
        }
        terms[word].pos.push_back(pos);
        used[word] = false;
        return;
    }
    terms.clear();
    
}

void DataBase::Show() {
    for (auto& elem: terms) {
        std::cout << elem.first << " ::: ";
        for (auto& el: elem.second.posting) {
            std::cout << el.first << "-" << el.second << " ";
        }
        std::cout << " ::: ";
        for (auto& el: elem.second.pos) {
            std::cout << el << " ";
        }
        std::cout << " ::: ";
    }
}

std::string GetDir(const std::string& dir) {
    size_t ind = dir.find_last_of("/");
    if (ind != std::string::npos && ind < dir.length() - 1) {
        return dir.substr(ind + 1);
    }
    return "";
}

std::vector<std::string> DeletePunctuation(const std::string& str) {
    std::vector<std::string> ans;
    std::string word;
    for (auto elem: str) {
        if (0 <= elem && elem <= 127 && !ispunct(elem) && elem != '\t' && elem != ' ') {
            word += std::tolower(elem);
        } else {
            if (word.size() != 0) ans.push_back(word);
            word.clear();
        }
    }
    return ans;
}

size_t DocumentWork(const std::string& entry, DataBase& bd, size_t ind) {
    std::ofstream docs(bd.docs_, std::ios::app);
    std::ifstream file(entry);
    std::string line;
    size_t sm = 0;
    size_t gl_sum = 0;
    size_t pos = 0;
    while (std::getline(file, line)) {
        std::string word;
        ++pos;
        std::vector<std::string> str = DeletePunctuation(line);
        for (auto& elem: str) {
            bd.WriteWord(elem, ind, pos);
        }
        sm += str.size();
    }
    gl_sum += sm;
    docs << std::to_string(ind) + " ";
    docs << entry + " ";
    docs << std::to_string(sm);
    docs << "\n";
    docs.close();
    return gl_sum;
}

void CreateDID(const fs::path& path, size_t& ind, DataBase& bd, size_t& total) {
    for (const auto& entry : fs::directory_iterator(path)) {
        if (fs::is_directory(entry.path())) {
            if (GetDir(entry.path().string()) != "BD") CreateDID(entry.path(), ind, bd, total);
        } else {
            ++ind;
            std::string path = entry.path().string();
            total += DocumentWork(path, bd, ind);
        }
    }
}

void Parse(int argc, char **argv) {
    size_t count = 5;
    bool is_need_search = false;
    for (size_t i = 0; i < argc; ++i) {
        std::string elem = argv[i];
        if (elem == "--help") {
            std::cout << "It's a program of indexator";
        } else if (elem == "--dir") {
            size_t ind = 0;
            size_t total = 0;
            DataBase bd;
            bd.Init();
            CreateDID(fs::path(argv[i + 1]), ind, bd, total);
            std::ofstream docs_(fs::path("../BD/size"), std::ios::app);
            docs_ << std::to_string(ind) << " ";
            docs_ << std::to_string(total);
            bd.Save();
        }
    }
}

void ShowFile(std::string path) {
    std::ifstream file(path, std::ios::binary);
    if (file.is_open()) {
        char byte;
        
        while (file.read(&byte, sizeof(char))) {
            std::cout << static_cast<int>(static_cast<unsigned char>(byte)) << ' ';
        }
        file.close();
    }
}

std::ostream& outStr(std::ostream& out, const std::string& s) {
    uint32_t l = s.size();
    out.write((char*)&l,sizeof(l));
    out.write(s.c_str(),l);
    return out;
}
