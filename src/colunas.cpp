#include "colunas.h"
#include "gurobi_c++.h"
#include "src/combo.h"
#include <cstddef>
#include <cmath>
#include <assert.h>

#define EPSILON 1e-6

Colunas::Colunas(Leitor& leitor)
    : m_leitor(leitor) {

    this->m_env.set(GRB_IntParam_OutputFlag, 0);
    this->m_env.start();
}

Colunas::~Colunas() {
    
}

bool Colunas::Solve() {
    const uint32_t size = this->m_leitor.size();

    // cria modelo
    this->m_model = std::make_unique<GRBModel>(this->m_env);

    // configura as variaveis e as condicoes
    this->variables.resize(size);
    this->expressions.resize(size);

    for(uint32_t i = 0; i < size; i++) {
        this->variables[i].variable = this->m_model->addVar(0.0, GRB_INFINITY, 0.0, GRB_CONTINUOUS);
        this->variables[i].itens.resize(size, 0);
        this->variables[i].itens[i] = 1;

        this->expressions[i] = this->m_model->addConstr(this->variables[i].variable == 1);
    }

    std::stack<Node> nodes;
    double best_cost = std::numeric_limits<double>::infinity();

    nodes.push({
       .restriction = {{-1,-1}, RestrictionType::None},
       .depth = 0,
    });

    int currentDepth = 0;
    while(!nodes.empty()) {
        // pega a node do topo
        // DFS
        Node node = nodes.top();
        nodes.pop();

        // retorna na tree e tira as restricoes adicionada da depth
        while(node.depth <= currentDepth) {
            this->PopRestriction();
            currentDepth--;
        }
        
        // atualiza a profundidade
        currentDepth = node.depth;

        // adiciona a restricao do node e resolve
        this->AddRestriction(node.restriction);
        this->SolveRelaxed(node.restriction.type == RestrictionType::None);

        // checa se eh valido
        int status = this->m_model->get(GRB_IntAttr_Status);
        if(status != GRB_OPTIMAL)
            continue;
        
        double cost = this->m_model->get(GRB_DoubleAttr_ObjVal);
        if(std::ceil(cost) >= best_cost)
            continue;

        // acha par mais fracionado
        std::pair<int, int> pair = this->GetBestPair();

        if(pair.first == -1) { // nao ha par fracionado
            best_cost = cost;

            this->best_containers.clear();
            for(int i = 0; i < this->variables.size(); i++) {
                double value = this->variables[i].variable.get(GRB_DoubleAttr_X);
                if(value >= EPSILON) {
                    std::vector<int> vec = this->variables[i].itens;
                    this->best_containers.push_back(vec);
                }
            }
        }
        else {
            // branch
            Node togetherNode = {};
            togetherNode.restriction.pair = pair;
            togetherNode.restriction.type = RestrictionType::Together;
            togetherNode.depth = currentDepth + 1;

            Node separatedNode = {};
            separatedNode.restriction.pair = pair;
            separatedNode.restriction.type = RestrictionType::Separated;
            separatedNode.depth = currentDepth + 1;

            nodes.push(togetherNode);
            nodes.push(separatedNode);
        }
    }

    this->m_solved = true;

    best_val = best_cost;
    return true;
}

void Colunas::SolveRelaxed(bool usePD) {
    const uint32_t size = this->m_leitor.size();

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

        delete[] allConstrs;
        
        // resolve o subproblem
        SubproblemResult result = usePD ? this->SolveSubproblemPD(dual) : this->SolveSubproblem(dual);
        
        // se nao for valido sai do loop
        if(result.cost >= -EPSILON) {
            break;
        }

        // adiciona a nova coluna no modelo
        GRBColumn column;
        for(uint32_t i = 0; i < size; i++) {
            if(result.results[i] >= EPSILON) {                    
                column.addTerm(1.0, this->expressions[i]);
            }
        }

        GRBVar var = this->m_model->addVar(0.0, GRB_INFINITY, 0.0, GRB_CONTINUOUS, column, NULL);
        this->variables.push_back({
            .variable = var,
            .itens = result.results
        });
    }
}

std::pair<int, int> Colunas::GetBestPair() {
    const uint32_t size = this->m_leitor.size();

    std::pair<int, int> best(-1,-1);
    double best_frac = 1e6;
    for(int i = 0; i < size; i++) {
        for(int j = i + 1; j < size; j++) {
            double frac = 0.0;

            // pega a fracionalidade
            for(BoxVariable& variable : this->variables) {
                if(variable.itens[i] && variable.itens[j]) {
                    double value = variable.variable.get(GRB_DoubleAttr_X);
                    frac += value;
                }
            }

            // as mais fracionaveis estao mais perto de 0.5
            frac = std::abs(frac - 0.5);
            if(frac < best_frac) {
                if(frac >= 0.5-EPSILON) { // se a distancia de 0.5 for 0.5 então significa q a soma foi 0 ou 1, podendo ignorar a branch pra esse caso
                    continue;
                }

                best = std::make_pair(i, j);
                best_frac = frac;
            }
        }
    }

    return best;
}

void Colunas::SolveProblem() {    
    GRBLinExpr objective = 0;
    for(uint32_t i = 0; i < this->variables.size(); i++) {
        objective += this->variables[i].variable;
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

    // adiciona restricoes do branching
    for(Restriction& restriction : this->m_restrictions) {
        std::pair<int,int>& pair = restriction.pair;
        if(restriction.type == RestrictionType::Together) {
            sub_model.addConstr(vars[pair.first] - vars[pair.second] == 0);
        }
        else if(restriction.type == RestrictionType::Separated) {
            sub_model.addConstr(vars[pair.first] + vars[pair.second] <= 1);
        }
    }
    
    sub_model.setObjective(1.0 - objective_sum, GRB_MINIMIZE);

    sub_model.optimize();

    std::vector<int> results;
    for(const GRBVar& v : vars) {
        int value = std::round(v.get(GRB_DoubleAttr_X));
        results.push_back(value);

    }
    return {
        .results = results,
        .cost = sub_model.get(GRB_DoubleAttr_ObjVal)
    };
}

SubproblemResult Colunas::SolveSubproblemPD(const std::vector<double>& dual) {
    const uint32_t size = this->m_leitor.size();
    const double scale = 100000.0;
    
    item* itens = new item[size];
    
    for(int i = 0; i < size; i++) {
        itens[i].p = (long)(dual[i] * scale);
        itens[i].w = this->m_leitor.get(i);
        itens[i].x = false;
        itens[i].index = i;
    }

    long value = combo(&itens[0], &itens[size-1], this->m_leitor.getMax(), 0, 1e6, true, false);
    
    std::vector<int> results(size);
    for(int i = 0; i < size; i++) {
        results[itens[i].index] = (itens[i].x ? 1 : 0);
    }

    delete[] itens;
    
    return {
        .results = results,
        .cost = 1.0 - value/scale
    };
}

void Colunas::AddRestriction(Restriction restriction) {
    if(restriction.type == RestrictionType::None)
        return;

    this->m_restrictions.push_back(restriction);

    // desativa variaveis q nao seguem a restricao
    for(int i = 0; i < this->variables.size(); i++) {
        std::vector<int>& itens = this->variables[i].itens;
        bool disable = false;
        if((restriction.type == RestrictionType::Together && (itens[restriction.pair.first] ^ itens[restriction.pair.second])) ||
            (restriction.type == RestrictionType::Separated && (itens[restriction.pair.first] && itens[restriction.pair.second]))) {

            disable = true;
        }

        if(disable) {
            this->SetVarActive(i, false);
        }
    }
}

Restriction Colunas::PopRestriction() {
    if(this->m_restrictions.size() == 0)
        return {};
    
    Restriction restriction = this->m_restrictions.back();
    this->m_restrictions.pop_back();

    // ativa variaveis q nao seguiam a restricao
    for(int i = 0; i < this->variables.size(); i++) {
        this->SetVarActive(i, true); // tenta ativar as variaveis
    }

    return restriction;
}

void Colunas::SetVarActive(uint32_t index, bool active) {
    assert(this->variables.size() > index);

    if(active) {
        // verifica se tem alguma restricao ainda afetando a variavel
        for (const auto& rest : m_restrictions) {
            std::vector<int>& itens = this->variables[index].itens;
            if ((rest.type == RestrictionType::Together && (itens[rest.pair.first] ^ itens[rest.pair.second])) ||
                (rest.type == RestrictionType::Separated && (itens[rest.pair.first] && itens[rest.pair.second]))) {
                return; // ainda deve ficar desativada
            }
        }
        this->variables[index].variable.set(GRB_DoubleAttr_UB, GRB_INFINITY);
    }
    else {
        this->variables[index].variable.set(GRB_DoubleAttr_UB, 0.0);
    }
}

double Colunas::getBest() {
    if(!this->m_solved)
        return -1;
    return this->best_val;   
}

std::vector<std::vector<int>> Colunas::getBestBoxes() {
    if(!this->m_solved)
        return {};
    return this->best_containers;
}
