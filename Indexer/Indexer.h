#pragma once
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <cstdint>
#include <map>

namespace fs = std::filesystem;

struct TermInfo {
    std::string term;
    size_t l_ind;
    size_t p_ind;

    bool empty() {
        return (term.size() == 0) ? 1 : 0;
    }
};

void ToVariint(size_t num, std::string& var);

size_t FromVatiint(const std::vector<uint8_t>& bits);

uint64_t GetNumber(std::ifstream& file);

TermInfo GetTermInfo(std::ifstream& t);


class DataBase {
public:
    void Init();

    size_t Size();

    void SavePosition(const std::vector<size_t>& elem, std::ofstream& p);

    void SaveTermInfo(std::ofstream& i, const std::string& str);

    void SavePostingLists(const std::map<size_t, size_t>& elem, std::ofstream& l);
 
    void Save();

    void WriteWord(std::string& word, size_t DID, size_t pos);

    void Show();

    struct Term {
        bool used = false;
        std::map<size_t, size_t> posting;
        std::vector<size_t> pos;
    };

    fs::path terminfo_ = "../BD/terminfo";
    fs::path pos_ = "../BD/pos";
    fs::path lists_ = "../BD/lists";
    fs::path docs_ = "../BD/docs";
    std::unordered_map<std::string, Term> terms;
    std::unordered_map<std::string, bool> used;
    uint64_t end_pos = 0;
    uint64_t end_lists = 0;
    bool is_saved = false;
};

std::string GetDir(const std::string& dir);

std::vector<std::string> DeletePunctuation(const std::string& str);

size_t DocumentWork(const std::string& entry, DataBase& bd, size_t ind);

void CreateDID(const fs::path& path, size_t& ind, DataBase& bd, size_t& total);

void Parse(int argc, char **argv);

void ShowFile(std::string path);

std::ostream& outStr(std::ostream& out, const std::string& s);


