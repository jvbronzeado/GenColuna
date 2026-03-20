#ifndef COLUNAS_H_
#define COLUNAS_H_

#include <gurobi_c++.h>
#include <memory>

#include "leitor.h"

struct SubproblemResult {
    std::vector<double> results;
    double cost;
};

class Colunas {
public:
    Colunas(Leitor& leitor);
    ~Colunas();

    bool Solve();
private:
    void SolveProblem();
    SubproblemResult SolveSubproblem(const std::vector<double>& dual);
    
    bool m_solved = false;

    Leitor& m_leitor;

    GRBEnv m_env = GRBEnv(true);

    std::unique_ptr<GRBModel> m_model;

    std::vector<GRBLinExpr> expressions;
    std::vector<GRBVar> variables;
public:
    double getBest();
};

#endif
