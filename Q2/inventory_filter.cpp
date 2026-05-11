/**
 * FILTER INVENTORY DATA - C++ Coding Assignment
 *
 * Reads inventory.json and filters entries based on:
 *   Memory  -> entry with highest memory
 *   CPU     -> entry with highest CPU speed
 *   Linux   -> all entries with OS = Linux
 *   Windows -> all entries with OS = Windows
 *
 * Compile:  g++ -std=c++17 -O2 -o inventory_filter inventory_filter.cpp
 * Usage:    ./inventory_filter <inventory.json> <filterCriteria>
 * Example:  ./inventory_filter inventory.json Memory
 *           ./inventory_filter inventory.json Linux
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <cctype>

// ============================================================
// Simple lightweight JSON parser for the given inventory format
// ============================================================

struct HostEntry {
    std::string ip;
    std::string os;
    double      memoryGB  = 0.0;   // parsed from "2GB" -> 2.0
    double      cpuGhz    = 0.0;   // parsed from "3.1Ghz" -> 3.1
    std::string disk;
};

// ---- Helper: trim whitespace and surrounding quotes ----
static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n\"");
    size_t end   = s.find_last_not_of(" \t\r\n\"");
    if (start == std::string::npos) return "";
    return s.substr(start, end - start + 1);
}

// ---- Helper: case-insensitive compare ----
static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

// ---- Helper: parse "2GB" -> 2.0, "16GB" -> 16.0 ----
static double parseMemory(const std::string& val) {
    try { return std::stod(val); } catch (...) { return 0.0; }
}

// ---- Helper: parse "3.1Ghz" -> 3.1 ----
static double parseCpu(const std::string& val) {
    try { return std::stod(val); } catch (...) { return 0.0; }
}

// ---- Extract the numeric prefix from a string like "16GB" or "3.8Ghz" ----
static std::string numericPrefix(const std::string& s) {
    std::string result;
    for (char c : s) {
        if (std::isdigit(c) || c == '.') result += c;
        else break;
    }
    return result;
}

// ============================================================
// InventoryParser  – reads the JSON file into a vector of HostEntry
// ============================================================
class InventoryParser {
public:
    explicit InventoryParser(const std::string& filePath) {
        std::ifstream file(filePath);
        if (!file.is_open())
            throw std::runtime_error("Cannot open file: " + filePath);

        std::ostringstream ss;
        ss << file.rdbuf();
        rawJson_ = ss.str();
    }

    std::vector<HostEntry> parse() {
        std::vector<HostEntry> entries;
        size_t pos = 0;

        // Walk through JSON looking for IP-keyed objects
        while ((pos = rawJson_.find('"', pos)) != std::string::npos) {
            // Read a key
            size_t keyStart = pos + 1;
            size_t keyEnd   = rawJson_.find('"', keyStart);
            if (keyEnd == std::string::npos) break;

            std::string key = rawJson_.substr(keyStart, keyEnd - keyStart);
            pos = keyEnd + 1;

            // Skip to ':'
            size_t colon = rawJson_.find(':', pos);
            if (colon == std::string::npos) break;
            pos = colon + 1;

            // Skip whitespace
            while (pos < rawJson_.size() && std::isspace(rawJson_[pos])) ++pos;

            // If the value starts with '{', it could be a host block
            if (pos < rawJson_.size() && rawJson_[pos] == '{') {
                size_t blockStart = pos;
                size_t blockEnd   = rawJson_.find('}', blockStart);
                if (blockEnd == std::string::npos) break;

                std::string block = rawJson_.substr(blockStart + 1, blockEnd - blockStart - 1);
                HostEntry entry = parseBlock(block);

                // Only add if we got an IP (skip the outer "inventory" object)
                if (!entry.ip.empty()) {
                    entries.push_back(entry);
                }
                pos = blockEnd + 1;
            }
        }
        return entries;
    }

private:
    std::string rawJson_;

    // Parse a single { ... } block into a HostEntry
    HostEntry parseBlock(const std::string& block) {
        HostEntry entry;
        size_t pos = 0;

        while (pos < block.size()) {
            // Find next key
            size_t ks = block.find('"', pos);
            if (ks == std::string::npos) break;
            size_t ke = block.find('"', ks + 1);
            if (ke == std::string::npos) break;
            std::string key = block.substr(ks + 1, ke - ks - 1);
            pos = ke + 1;

            // Find ':'
            size_t colon = block.find(':', pos);
            if (colon == std::string::npos) break;
            pos = colon + 1;

            // Find value (quoted string)
            size_t vs = block.find('"', pos);
            if (vs == std::string::npos) break;
            size_t ve = block.find('"', vs + 1);
            if (ve == std::string::npos) break;
            std::string val = block.substr(vs + 1, ve - vs - 1);
            pos = ve + 1;

            std::string k = toLower(key);
            if      (k == "ip")     entry.ip     = val;
            else if (k == "os")     entry.os      = val;
            else if (k == "memory") entry.memoryGB = parseMemory(numericPrefix(val));
            else if (k == "cpu")    entry.cpuGhz   = parseCpu(numericPrefix(val));
            else if (k == "disk")   entry.disk     = val;
        }
        return entry;
    }
};

// ============================================================
// InventoryFilter  – applies the filter criteria
// ============================================================
class InventoryFilter {
public:
    explicit InventoryFilter(const std::vector<HostEntry>& entries)
        : entries_(entries) {}

    std::vector<HostEntry> filter(const std::string& criteria) const {
        std::string c = toLower(criteria);

        if (c == "memory") {
            return maxBy([](const HostEntry& e){ return e.memoryGB; });
        } else if (c == "cpu") {
            return maxBy([](const HostEntry& e){ return e.cpuGhz; });
        } else if (c == "linux" || c == "windows") {
            return byOS(criteria);
        } else {
            throw std::invalid_argument(
                "Invalid filter criteria: '" + criteria +
                "'. Valid options: Memory, CPU, Linux, Windows.");
        }
    }

private:
    const std::vector<HostEntry>& entries_;

    // Return all entries that share the maximum value of extractor
    template<typename Extractor>
    std::vector<HostEntry> maxBy(Extractor extractor) const {
        if (entries_.empty()) return {};

        double maxVal = extractor(entries_[0]);
        for (const auto& e : entries_)
            maxVal = std::max(maxVal, extractor(e));

        std::vector<HostEntry> result;
        for (const auto& e : entries_)
            if (extractor(e) == maxVal)
                result.push_back(e);
        return result;
    }

    // Return all entries matching the OS (case-insensitive)
    std::vector<HostEntry> byOS(const std::string& os) const {
        std::vector<HostEntry> result;
        for (const auto& e : entries_)
            if (toLower(e.os) == toLower(os))
                result.push_back(e);
        return result;
    }
};

// ============================================================
// Display helpers
// ============================================================
static void printEntry(const HostEntry& e) {
    std::cout << "  {\n"
              << "    \"ip\": \""     << e.ip       << "\",\n"
              << "    \"os\": \""     << e.os       << "\",\n"
              << "    \"memory\": \"" << e.memoryGB << "GB\",\n"
              << "    \"cpu\": \""    << e.cpuGhz   << "Ghz\",\n"
              << "    \"disk\": \""   << e.disk     << "\"\n"
              << "  }\n";
}

// ============================================================
// main
// ============================================================
int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: inventory_filter <inventory.json> <filterCriteria>\n"
                  << "  filterCriteria: Memory | CPU | Linux | Windows\n";
        return 1;
    }

    std::string filePath = argv[1];
    std::string criteria = argv[2];

    // Note 1: Raise exception if filter criteria param is missing (handled above)
    if (criteria.empty()) {
        std::cerr << "Error: filter criteria param is missing.\n";
        return 1;
    }

    try {
        InventoryParser parser(filePath);
        std::vector<HostEntry> entries = parser.parse();

        InventoryFilter inventoryFilter(entries);
        std::vector<HostEntry> results = inventoryFilter.filter(criteria);

        if (results.empty()) {
            std::cout << "No entries found for criteria: " << criteria << "\n";
        } else {
            std::cout << "=== Results for criteria: " << criteria
                      << " (" << results.size() << " entries) ===\n[\n";
            for (const auto& e : results) printEntry(e);
            std::cout << "]\n";
        }

    } catch (const std::invalid_argument& ex) {
        // Note 2: Raise exception if filter criteria doesn't match
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
