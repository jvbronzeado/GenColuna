#ifndef COLUNAS_H_
#define COLUNAS_H_

#include <gurobi_c++.h>
#include <memory>
#include <stack>

#include "leitor.h"

struct BoxVariable {
    GRBVar variable;
    std::vector<int> itens;
};

enum class RestrictionType {
    None,
    Together,
    Separated,
};

struct Restriction {
    std::pair<int, int> pair;
    RestrictionType type;
};

struct Node {
    Restriction restriction;
    int depth;
};

struct SubproblemResult {
    std::vector<int> results;
    double cost;
};

class Colunas {
public:
    Colunas(Leitor& leitor);
    ~Colunas();

    bool Solve();
private:
    void SolveRelaxed(bool usePd=false);
    std::pair<int, int> GetBestPair();
    
    void SolveProblem();
    SubproblemResult SolveSubproblem(const std::vector<double>& dual);
    SubproblemResult SolveSubproblemPD(const std::vector<double>& dual);

    void AddRestriction(Restriction restriction);
    Restriction PopRestriction();

    void SetVarActive(uint32_t index, bool active);
    
    bool m_solved = false;
    double best_val = 1e6;
    std::vector<std::vector<int>> best_containers;

    Leitor& m_leitor;

    GRBEnv m_env = GRBEnv(true);

    std::unique_ptr<GRBModel> m_model;

    std::vector<GRBConstr> expressions;
    std::vector<BoxVariable> variables;

    std::vector<Restriction> m_restrictions;
public:
    double getBest();
    std::vector<std::vector<int>> getBestBoxes();
};

#endif
