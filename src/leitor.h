#ifndef LEITOR_H_
#define LEITOR_H_

#include <string>
#include <vector>
#include <stdint.h>

class Leitor {
public:
    Leitor(const std::string& filepath);
    ~Leitor() = default;

    uint32_t get(uint32_t i);
    uint32_t size();

    uint32_t getMax();

    bool isLoaded() { return this->m_loaded; };
private:
    bool m_loaded = false;
    
    std::vector<uint32_t> m_itens;
    uint32_t m_max;
};

#endif
