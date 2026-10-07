#include <array>
#include <cassert>
#include <fcntl.h>
#include <fmt/format.h>
#include <iterator>
#include <list>
#include <set>
#include <string.h>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{

constexpr size_t ttp(size_t x)
{
    return 1 << x;
}

constexpr size_t VOCAB_SIZE = 10000;
constexpr size_t BUFF_SIZE  = ttp(12);

struct Node {
    size_t token;
    std::list<std::list<Node>::iterator>::iterator location_reverse;
};

using TokenizedStr     = std::list<Node>;
using StrArray         = std::array<std::string, VOCAB_SIZE>;
using Freq             = size_t;
using Index            = size_t;
using Pair             = std::pair<size_t, size_t>;
using FreqPairPair     = std::pair<Freq, Pair>;
using FreqPairPriQueue = std::set<FreqPairPair>;

struct PairData {
    Freq freq = 0;
    std::list<TokenizedStr::iterator> location;
};

struct MyPairHash {
    size_t operator()(const Pair& p) const
    {
        size_t ret = std::hash<size_t>{}(p.first);
        ret ^= std::hash<size_t>{}(p.second) + 0x9e3779b9 + (ret << 6) +
               (ret >> 2);

        return ret;
    };
};

using FreqMap = std::unordered_map<Pair, PairData, MyPairHash>;

StrArray vocab;
size_t vocab_end = 0;

int64_t exists(const std::unordered_map<char, size_t>& map, char c)
{
    auto it = map.find(c);

    if (it != map.end()) {
        return it->second;
    }

    return -1; // didn't find
}

} // namespace

FreqPairPriQueue init_vocab(const char* data, size_t data_size, FreqMap& map,
                            TokenizedStr& tstr)
{
    std::unordered_map<char, size_t> hist{};

    for (int i = 0; i < data_size; ++i) {
        char c = data[i];

        int64_t index = exists(hist, c);
        if (index != -1) { // exists
            tstr.push_back(Node{
                static_cast<size_t>(index),
                std::list<std::list<Node>::iterator>::iterator{},
            });

            continue;
        }

        // doesn't exist
        vocab[vocab_end] = c;
        hist[c]          = vocab_end;
        tstr.push_back(Node{
            static_cast<size_t>(vocab_end),
            std::list<std::list<Node>::iterator>::iterator{},
        });

        vocab_end += 1;
    }

    for (auto it = tstr.begin(); it != std::prev(tstr.end()); ++it) {
        Pair new_pair = {it->token, std::next(it)->token};
        auto jt       = map.find(new_pair);
        if (jt == map.end()) { // didn't find...
            map[new_pair]        = {1, std::list<TokenizedStr::iterator>{it}};
            it->location_reverse = map[new_pair].location.begin();
        } else { // found...
            ++(jt->second.freq);
            // if (Pair{std::prev(it)->token, it->token} != new_pair) {
            jt->second.location.push_back(it);
            it->location_reverse = std::prev(jt->second.location.end());
            // }
        }
    }

    FreqPairPriQueue fppq{};
    for (const auto& kv : map) {
        assert(kv.second.freq != 0);
        auto result = fppq.insert({kv.second.freq, kv.first});
        assert(result.second);
    }

    return fppq;
}

void process_data(TokenizedStr& tstr, FreqMap& pfm, FreqPairPriQueue& fppq)
{

    while (vocab_end < VOCAB_SIZE) {
        if (fppq.empty()) {
            break;
        }

        auto max_pair_it = std::prev(fppq.end());
        if (max_pair_it->first == 1) {
            break;
        }

        auto most_freq_pair = max_pair_it->second;
        auto& mfq_locations = pfm[most_freq_pair].location;

        vocab[vocab_end] =
            vocab[most_freq_pair.first] + vocab[most_freq_pair.second];

        // for (auto& loc : mfq_locations) {
        for (auto loc_it = mfq_locations.begin(); loc_it != mfq_locations.end();
             ++loc_it) {
            auto& loc = *loc_it;

#ifdef DEBUG
            size_t cur_loc_dist = std::distance(tstr.begin(), loc);
            std::vector<size_t> cur_locations{};
            for (auto& n : mfq_locations) {
                cur_locations.push_back(std::distance(tstr.begin(), n));
            }
            std::vector<size_t> cur_tstr{};
            for (auto& n : tstr) {
                cur_tstr.push_back(n.token);
            }
#endif

            if (loc != tstr.begin()) {
                Pair pair_to_update = {std::prev(loc)->token, loc->token};
                // if (pair_to_update != most_freq_pair) {
                auto map_it = pfm.find(pair_to_update);
                assert(map_it != pfm.end());

                auto queue_it =
                    fppq.find({map_it->second.freq, pair_to_update});
                assert(queue_it != fppq.end());
                fppq.erase(queue_it);

                --(map_it->second.freq);
                if (map_it->second.freq <= 0) {
                    pfm.erase(map_it);
                } else {
                    fppq.insert({map_it->second.freq, pair_to_update});
                    pfm[pair_to_update].location.erase(
                        std::prev(loc)->location_reverse);
                }
                // }
            }

            if (std::next(loc) != tstr.end() &&
                std::next(std::next(loc)) != tstr.end()) {
                Pair pair_to_update = {std::next(loc)->token,
                                       std::next(std::next(loc))->token};
                if (pair_to_update == most_freq_pair) {
                    pfm[pair_to_update].location.erase(std::next(loc_it));
                } else {
                    auto map_it = pfm.find(pair_to_update);
                    assert(map_it != pfm.end());

                    auto queue_it =
                        fppq.find({map_it->second.freq, pair_to_update});
                    assert(queue_it != fppq.end());
                    fppq.erase(queue_it);

                    --(map_it->second.freq);
                    if (map_it->second.freq <= 0) {
                        pfm.erase(map_it);
                    } else {
                        fppq.insert({map_it->second.freq, pair_to_update});
                        pfm[pair_to_update].location.erase(
                            std::next(loc)->location_reverse);
                    }
                }
            }

            loc->token = vocab_end;
            tstr.erase(std::next(loc));

            if (loc != tstr.begin()) {
                Pair new_pair = {std::prev(loc)->token, loc->token};
                if (auto it = pfm.find(new_pair); it == pfm.end()) {
                    pfm[new_pair] = {
                        1, std::list<TokenizedStr::iterator>{std::prev(loc)}};
                    std::prev(loc)->location_reverse =
                        pfm[new_pair].location.begin();
                    fppq.insert({1, new_pair});
                } else {
                    size_t old_freq = it->second.freq;

                    ++(it->second.freq);
                    it->second.location.push_back(std::prev(loc));
                    std::prev(loc)->location_reverse =
                        std::prev(pfm[new_pair].location.end());

                    auto queue_it = fppq.find({old_freq, new_pair});
                    assert(queue_it != fppq.end());
                    fppq.erase(queue_it);
                    fppq.insert({it->second.freq, new_pair});
                }
            }

            if (std::next(loc) != tstr.end()) {
                Pair new_pair = {loc->token, std::next(loc)->token};
                if (auto it = pfm.find(new_pair); it == pfm.end()) {
                    pfm[new_pair] = {1, std::list<TokenizedStr::iterator>{loc}};
                    loc->location_reverse = pfm[new_pair].location.begin();
                    fppq.insert({1, new_pair});
                } else {
                    size_t old_freq = it->second.freq;

                    ++(it->second.freq);
                    it->second.location.push_back(loc);
                    loc->location_reverse =
                        std::prev(pfm[new_pair].location.end());

                    auto queue_it = fppq.find({old_freq, new_pair});
                    assert(queue_it != fppq.end());
                    fppq.erase(queue_it);
                    fppq.insert({it->second.freq, new_pair});
                }
            }
        }

        fppq.erase(max_pair_it);
        pfm.erase(most_freq_pair);

        vocab_end += 1;
    }
}

int main(int argc, char* argv[])
{

#ifdef DEBUG
    if (argc < 2) {
        fprintf(stderr, "usage: main <string>\n");
        return 1;
    }
    char* data = argv[1];
#else
    // char* path = NULL;
    // if (argc < 2) {
    //     fprintf(stderr, "usage: main <path>\n");
    //     return 1;
    // }
    // path = argv[1];
    const char* path = "/home/aregmk/Coding/twopairencoding/data/clean/"
                       "cleaned_data_output.txt";

    struct stat st;
    if (stat(path, &st) != 0) {
        fprintf(stderr, "stat: couldn't open file\n");
        return 1;
    }

    size_t file_size = st.st_size;
    printf("file size: %zu\n", file_size);

    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "open: couldn't open file\n");
        return 1;
    }

    char* data = (char*)mmap(NULL, file_size, PROT_READ, MAP_SHARED, fd, 0);
#endif

    FreqMap tfm;
    TokenizedStr tstr;

#ifdef DEBUG
    FreqPairPriQueue fppq = init_vocab(data, strlen(data), tfm, tstr);
#else
    FreqPairPriQueue fppq = init_vocab(data, file_size, tfm, tstr);
#endif

    process_data(tstr, tfm, fppq);

    fmt::print("Compressed String:\n");
    for (Node n : tstr) {
        fmt::print("{} ", n.token);
    }
    fmt::print("\n\n");
    fmt::print("===================================================\n");
    fmt::print("\n");

    for (int i = 0; i < vocab_end; ++i) {
        fmt::print("Token {}: {}\n", i, vocab[i]);
    }

#ifndef DEBUG
    munmap(data, file_size);
#endif

    return 0;
}
