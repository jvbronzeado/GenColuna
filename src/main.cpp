#include <stdio.h>
#include <gurobi_c++.h>

#include "leitor.h"

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>

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
    printf("Vem dançar forró, vem\n");
    printf("Vem dançar forró, vem\n");
    printf("Vem dançar forró deixa os problemas pra depois\n");

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

    // leitor teste
    Leitor leitor(inputs[0]);
    if(!leitor.isLoaded()) {
        return -1;
    }

    std::cout << "max price: " << leitor.getMax() << '\n';
    std::cout << "item count: " << leitor.size() << '\n';
    for(int i = 0; i < leitor.size(); i++) {
        std::cout << "item["  << i << "]: " << leitor.get(i) << '\n';
    }

    try {
        // Formulate and solve model
        
        // Create an environment
        GRBEnv env = GRBEnv(true);
        env.set("LogFile", "mip1.log");
        env.start();

        GRBModel model = GRBModel(env);

        // Create variables
        GRBVar x = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, "x");
        GRBVar y = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, "y");
        GRBVar z = model.addVar(0.0, 1.0, 0.0, GRB_BINARY, "z");

        // Add constraint: x + 2 y + 3 z <= 4
        model.addConstr(x + 2 * y + 3 * z <= 4, "c0");

        // Set objective: maximize x + y + 2 z
        model.setObjective(x + y + 2 * z, GRB_MAXIMIZE);

        // Optimize model
        model.optimize();

        cout << x.get(GRB_StringAttr_VarName) << " "
             << x.get(GRB_DoubleAttr_X) << endl;
        cout << y.get(GRB_StringAttr_VarName) << " "
             << y.get(GRB_DoubleAttr_X) << endl;
        cout << z.get(GRB_StringAttr_VarName) << " "
             << z.get(GRB_DoubleAttr_X) << endl;

        cout << "Obj: " << model.get(GRB_DoubleAttr_ObjVal) << endl;
    } catch(GRBException e) {
        cout << "Error code = " << e.getErrorCode() << endl;
        cout << e.getMessage() << endl;
    } catch(...) {
        cout << "Exception during optimization" << endl;
    }
    
    return 0;
}
