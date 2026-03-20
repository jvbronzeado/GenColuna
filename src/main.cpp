#include <chrono>
#include <stdio.h>
#include <gurobi_c++.h>

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>

#include "leitor.h"
#include "colunas.h"

std::vector<std::string> cmd_tokens;
bool cmd_exists(const std::string& option)
{
    return std::find(cmd_tokens.begin(), cmd_tokens.end(), option) != cmd_tokens.end();
}

void display_help_message()
{
    std::cout   << "Usage: " << cmd_tokens[0] << " [options] [files] [directories]\n"
                << "Options:\n"
                << "  --help         Shows this help message\n";
}

using namespace std;

int main(int argc, char** argv) {
    // get string tokens
    for(int i = 0; i < argc; i++)
    {
        cmd_tokens.push_back(std::string(argv[i]));
    }

    // check for help option
    if(cmd_exists("--help"))
    {
        display_help_message();
    }

    // get instances to be executed
    std::vector<std::string> inputs;
    for(int i = 1; i < argc; i++) {
        if(cmd_tokens[i] == "--help")
            continue;

        std::filesystem::path input_path = cmd_tokens[i];
        if(std::filesystem::is_directory(input_path)) {
            for(const auto& file : std::filesystem::directory_iterator(input_path))
            {
                if(!file.is_directory())
                {
                    inputs.push_back(file.path());
                }
            }
        }
        else {
            inputs.push_back(cmd_tokens[i]);
        }
    }

    if(inputs.size() == 0) {
        std::cout << "no input files received" << std::endl;
        return 0;
    }

    // setup result header
    int name_width = 0;
    int time_width = 15;
    int cost_width = 15; 
    for(std::string& path : inputs)
    {
        name_width = std::max(name_width, (int)path.size());
    }

    name_width += 2;

    // print header
    std::cout << std::left << std::setw(name_width) << "instances"
              << std::setw(time_width) << "time (s)"
              << std::setw(cost_width) << "cost" << std::endl;

    // print separator
    std::cout << std::setfill('-') << std::setw(name_width + time_width + cost_width) << "" << std::endl;
    std::cout << std::setfill(' '); // Reset fill character

    for(const std::string& path : inputs) {    
        try {       
            Leitor leitor(path);
            if(!leitor.isLoaded()) {
                std::cerr << "failed to load instance: " << path << std::endl;
                continue;
            }

            auto start = std::chrono::high_resolution_clock::now();
            Colunas c(leitor);
            c.Solve();

            double cost = c.getBest();

            auto end = std::chrono::high_resolution_clock::now();

            // Calculate the duration
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            double t = duration.count()/1000000.0;

            std::cout << std::left << std::setw(name_width) << path
                      << std::setw(time_width) << std::setprecision(6) << t
                      << std::setw(cost_width) << cost << std::endl; 
        } catch(GRBException e) {
            cout << "Error code = " << e.getErrorCode() << endl;
            cout << e.getMessage() << endl;
        } catch(...) {
            cout << "Exception during optimization" << endl;
        }
    }
    return 0;
}
