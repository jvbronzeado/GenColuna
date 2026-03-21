#include "leitor.h"
#include "combo.h"

#include <iostream>
#include <fstream>

Leitor::Leitor(const std::string& filepath) {
    std::ifstream f(filepath, std::ifstream::ios_base::in);
    if(f.is_open()) {
        // get item count
        std::string size_str;
        if(!std::getline(f, size_str)) {
            std::cerr << "[Loader] failed to get item count" << std::endl;
            return;
        }

        uint32_t n = std::stoi(size_str);
        this->m_itens.resize(n);
        
        // get max price
        std::string max_str;
        if(!std::getline(f, max_str)) {
            std::cerr << "[Loader] failed to get max price" << std::endl;
            return;
        }

        uint32_t price = std::stoi(max_str);
        this->m_max = price;

        // get items values
        std::string item;
        for(uint32_t i = 0; i < n; i++) {
            if(!std::getline(f, item)) {
                std::cerr << "[Loader] failed to get item of index " << i << std::endl;
                return;
            }

            uint32_t value = std::stoi(item);
            this->m_itens[i] = value;
        }

        this->m_loaded = true;
    }
    else {
        std::cerr << "[Loader] failed to read" << std::endl;
    } 
}

uint32_t Leitor::get(uint32_t i) {
    if(!m_loaded)
        return -1;
    return this->m_itens[i];
}

uint32_t Leitor::size() {
    if(!m_loaded)
        return -1;
    return this->m_itens.size();
}

uint32_t Leitor::getMax() {
    if(!m_loaded)
        return -1;
    return this->m_max;
}
