// Independent exhaustive rule / outcome oracle; never shipped as a bot wrapper.
#include "../bots/fulcrum/strategy.cpp"
#include <iostream>
#include <random>
#include <stdexcept>

static void require(bool good, const char* message) { if (!good) throw std::runtime_error(message); }
static std::pair<int,int> torque(const State& s, U64 mask) {
    int left = -9, right = -3;
    for (int i=0;i<61;++i) if (mask & (U64(1)<<i)) {
        left -= s.weight[i]*(i-27); right -= s.weight[i]*(i-29);
    }
    return {left,right};
}
static bool stable(const State& s, U64 mask) { auto t=torque(s,mask); return t.first<=0 && t.second>=0; }
int main() {
    std::mt19937 gen(20261003); int checked=0, outcomes=0, decisions=0;
    for (int game=0;game<400;++game) {
        GameState g; g.k=1+gen()%24; g.phase="add"; g.player=0; g.torque_left=-6;g.torque_right=6;
        g.board={{-4,3,-1}}; for(int p=0;p<2;++p) for(int w=1;w<=g.k;++w) g.remaining[p].push_back(w);
        State s=from_game(g);
        for(int ply=0;ply<2*g.k;++ply) {
            auto t=torque(s,s.occupied); require(s.a==-t.first && s.b==t.second,"torque update");
            U64 ws=s.inventory[s.turn]; std::vector<Action> moves;
            while(ws) {
                int w=first(ws)+1;ws&=ws-1; U64 independent=0;
                for(int i=0;i<61;++i) if(!(s.occupied&(U64(1)<<i))) {
                    int l=t.first-w*(i-27),r=t.second-w*(i-29);
                    if(l<=0 && r>=0) independent|=U64(1)<<i;
                }
                require(independent==s.places(w),"legal placement interval"); ++checked;
                while(independent) { int i=first(independent);independent&=independent-1;moves.push_back({i,w,0}); }
            }
            if(moves.empty()) break;
            s.apply(moves[gen()%moves.size()]);
        }
        if(game>=80) continue;
        // Reduce arbitrary stable boards to at most 12 blocks for exhaustive checks.
        s.adding=false;
        while(bits(s.occupied)>12 && s.removals()) {
            U64 m=s.removals();int r=gen()%bits(m);while(r--)m&=m-1;s.apply({first(m),0,0});
        }
        if(bits(s.occupied)>12) continue;
        std::vector<int> indices; U64 rem=s.occupied;
        while(rem){indices.push_back(first(rem));rem&=rem-1;}
        int size=1<<indices.size();std::vector<int> oracle(size,-2);std::vector<U64> masks(size);
        RemovalSolver solver(s);
        for(int sub=0;sub<size;++sub) {
            U64 mask=0;for(int j=0;j<int(indices.size());++j)if(sub&(1<<j))mask|=U64(1)<<indices[j]; masks[sub]=mask;
            if(!stable(s,mask)) continue;
            bool win=false;U64 independent=0;
            for(int j=0;j<int(indices.size());++j) if(sub&(1<<j)) {
                if(stable(s,mask^(U64(1)<<indices[j]))) { independent|=U64(1)<<indices[j]; if(oracle[sub^(1<<j)]==-1)win=true; }
            }
            oracle[sub]=win?1:-1;auto tor=torque(s,mask);
            require(solver.legal(mask,-tor.first,tor.second)==independent,"legal removal");
            Proof answer=solver.solve(mask,-tor.first,tor.second);
            require(answer.value==oracle[sub],"exact game outcome");
            if(win) {
                require(answer.slot>=0 && (independent&(U64(1)<<answer.slot)),"winning move legality");
                int j=int(std::find(indices.begin(),indices.end(),int(answer.slot))-indices.begin());
                require(oracle[sub^(1<<j)]==-1,"winning move oracle");
            }
            ++outcomes;
        }
        g.phase="remove";g.player=gen()%2;g.game_id=std::to_string(game);g.board.clear();
        for(int i:indices)g.board.push_back({i-30,s.weight[i],-1});
        auto tor=torque(s,s.occupied);g.torque_left=tor.first;g.torque_right=tor.second;g.remaining[0].clear();g.remaining[1].clear();g.clocks[g.player]=0.8;
        Move move=choose_move(g);int j=int(std::find(indices.begin(),indices.end(),move.position+30)-indices.begin());
        require(j<int(indices.size()),"returned occupied position");
        if(s.removals()) require(stable(s,s.occupied^(U64(1)<<(move.position+30))),"safe move when one exists");
        if(oracle[size-1]==1) require(oracle[(size-1)^(1<<j)]==-1,"choose_move preserves forced win");
        ++decisions;
    }
    delete cached_solver; cached_solver=nullptr;
    std::cout<<"PASS: "<<checked<<" inventory legality checks, "<<outcomes<<" exhaustive subset outcomes, "<<decisions<<" winning-move decisions\n";
}
