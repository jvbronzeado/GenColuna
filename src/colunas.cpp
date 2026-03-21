#include "colunas.h"

Colunas::Colunas(Leitor& leitor)
    : m_leitor(leitor) {

    this->m_env.set(GRB_IntParam_OutputFlag, 0);
    this->m_env.start();
}

Colunas::~Colunas() {
    
}

bool Colunas::Solve() {
    const uint32_t size = this->m_leitor.size();
    
    // create model
    this->m_model = std::make_unique<GRBModel>(this->m_env);

    // set default variables and expressions
    this->variables.resize(size);
    this->expressions.resize(size);
    for(uint32_t i = 0; i < size; i++) {
        this->variables[i] = this->m_model->addVar(0.0, 1.0, 0.0, GRB_CONTINUOUS);
        this->expressions[i] = this->variables[i];
    }

    while(true) {
        // otimiza o atual
        this->SolveProblem();

        // pega o dual
        uint32_t numConstrs = this->m_model->get(GRB_IntAttr_NumConstrs);

        GRBConstr* allConstrs = this->m_model->getConstrs();
        std::vector<double> dual(numConstrs);

        for (uint32_t i = 0; i < numConstrs; ++i) {
            dual[i] = allConstrs[i].get(GRB_DoubleAttr_Pi);
        }

        for (uint32_t i = 0; i < numConstrs; ++i) {
            this->m_model->remove(allConstrs[i]); // remove a constraint antigas
        }

        // resolve o subproblem
        SubproblemResult result = this->SolveSubproblem(dual);
        
        // se não for valido sai do loop
        if(result.cost >= 0.0) {
            break;
        }

        // adiciona a nova coluna no modelo
        this->variables.push_back(this->m_model->addVar(0.0, 1.0, 0.0, GRB_CONTINUOUS));
        GRBVar& var = this->variables.back();

        for(uint32_t i = 0; i < size; i++) {
            if(result.results[i] >= 1e-6) {
                this->expressions[i] += var;
            }
        }
    }

    this->m_solved = true;
    return true;
}

void Colunas::SolveProblem() {    
    for(uint32_t i = 0; i < this->expressions.size(); i++) {
        this->m_model->addConstr(this->expressions[i] == 1);
    }

    GRBLinExpr objective = 0;
    for(uint32_t i = 0; i < this->variables.size(); i++) {
        objective += this->variables[i];
    }
    
    this->m_model->setObjective(objective, GRB_MINIMIZE);

    this->m_model->optimize();
}

SubproblemResult Colunas::SolveSubproblem(const std::vector<double>& dual) {
    const uint32_t size = this->m_leitor.size();
    
    GRBModel sub_model(this->m_env);

    GRBLinExpr weight_sum = 0;
    GRBLinExpr objective_sum = 0;

    std::vector<GRBVar> vars;
    vars.reserve(size);
    for(uint32_t i = 0; i < size; i++) {
        vars.push_back(sub_model.addVar(0.0, 1.0, 0.0, GRB_BINARY));
    }

    for(uint32_t i = 0; i < size; i++) {
        weight_sum += this->m_leitor.get(i) * vars[i];
        objective_sum += dual[i] * vars[i];
    }

    sub_model.addConstr(weight_sum <= this->m_leitor.getMax());
    sub_model.setObjective(1.0 - objective_sum, GRB_MINIMIZE);

    sub_model.optimize();

    std::vector<double> results;
    for(const GRBVar& v : vars) {
        results.push_back(v.get(GRB_DoubleAttr_X));
    }
    
    return {
        .results = results,
        .cost = sub_model.get(GRB_DoubleAttr_ObjVal)
    };
}

double Colunas::getBest() {
    if(!this->m_solved)
        return -1;
    return this->m_model->get(GRB_DoubleAttr_ObjVal);   
}
