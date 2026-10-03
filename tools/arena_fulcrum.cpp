// Reproducible fast arena. Official subprocess protocol is checked separately.
#include "../bots/fulcrum/strategy.cpp"
#include <iostream>
#include <iomanip>
#include <random>
#include <fstream>
#include <filesystem>

static U64 reference_removals(const State& s) {
    U64 legal=0;
    for(int i=0;i<61;++i)if(s.occupied&(U64(1)<<i)) {
        if(s.a-s.weight[i]*(i-27)>=0 && s.b+s.weight[i]*(i-29)>=0)legal|=U64(1)<<i;
    }
    return legal;
}
static double pressure(const State& s,int player) {
    U64 inv=s.inventory[player];double value=0;int n=bits(inv);
    while(inv){int w=first(inv)+1;inv&=inv-1;int c=bits(s.places(w));value+=std::log1p(c)*20+(c?0:-45);}
    return n?value/n:60;
}
static Action opponent(State s,int mode,std::mt19937& gen) {
    if(s.adding) {
        std::vector<Action> all;
        U64 ws=s.inventory[s.turn];
        while(ws){int w=first(ws)+1;ws&=ws-1;U64 ps=s.places(w);
            while(ps){int i=first(ps);ps&=ps-1;all.push_back({i,w,0});}}
        if(all.empty())return {first(~s.occupied&SLOTS),first(s.inventory[s.turn])+1,0};
        if(mode==0)return all[gen()%all.size()];
        if(mode==1){std::vector<int> weights;ws=s.inventory[s.turn];while(ws){weights.push_back(first(ws)+1);ws&=ws-1;}
            int w=weights[gen()%weights.size()];U64 ps=s.places(w);return {first(ps?ps:(~s.occupied&SLOTS)),w,0};}
        for(auto& m:all){State t=s;t.apply(m);
            m.rank=pressure(t,s.turn)-pressure(t,s.turn^1)+10.0*m.weight/s.k;
            if(t.adding&&!t.can_add(t.turn))m.rank=WIN;
        }
        std::stable_sort(all.begin(),all.end(),[](Action x,Action y){return x.rank>y.rank;});
        if(mode==2)return all.front();
        // Strong reference: two-ply adversarial placement + exact removal <=22.
        double best=-1e100;Action chosen=all.front();int width=std::min<int>(24,all.size());
        for(int j=0;j<width;++j){Action m=all[j];State t=s;t.apply(m);double worst=1e100;
            if(!t.adding){RemovalSolver exact(t);exact.limit=150000;
                try {Proof p=exact.solve(t.occupied,t.a,t.b);int winner=p.value>0?t.turn:t.turn^1;worst=winner==s.turn?WIN:-WIN;}
                catch(Interrupted&){worst=0;}}
            else if(!t.can_add(t.turn))worst=WIN;
            else {
                U64 vs=t.inventory[t.turn];
                while(vs){int w=first(vs)+1;vs&=vs-1;U64 ps=t.places(w);
                    while(ps){int i=first(ps);ps&=ps-1;State u=t;u.apply({i,w,0});double v=pressure(u,s.turn)-pressure(u,s.turn^1);
                        if(u.adding&&!u.can_add(u.turn))v=-WIN;worst=std::min(worst,v);}}
            }
            worst+=0.05*m.rank;if(worst>best){best=worst;chosen=m;}
        }
        return chosen;
    }
    U64 moves=reference_removals(s);
    if(!moves)return {first(s.occupied),0,0};
    if(mode>=2&&bits(s.occupied)<=22){RemovalSolver exact(s);exact.limit=350000;
        try{Proof p=exact.solve(s.occupied,s.a,s.b);if(p.value>0)return {p.slot,0,0};}catch(Interrupted&){} }
    std::vector<Action> all;
    while(moves){int i=first(moves);moves&=moves-1;State t=s;t.apply({i,0,0});int replies=bits(reference_removals(t));
        if(!replies)return {i,0,0};all.push_back({i,0,-double(replies)});}
    if(mode<2)return all[gen()%all.size()];
    return *std::max_element(all.begin(),all.end(),[](Action x,Action y){return x.rank<y.rank;});
}
int main(int argc,char**argv) {
    int seeds=argc>1?std::stoi(argv[1]):4;double clock=argc>2?std::stod(argv[2]):3;
    std::filesystem::path output_path(argc>3?argv[3]:"benchmarks/local/arena.csv");
    if(output_path.has_parent_path())std::filesystem::create_directories(output_path.parent_path());
    std::ofstream output(output_path);
    if(!output){std::cerr<<"Cannot open results file: "<<output_path<<'\n';return 1;}
    output<<"k,opponent,seed,seat,win,reason,our_seconds,plies\n";
    std::array<const char*,4> names={"uniform","course_random","pressure_exact22","minimax_exact22"};
    int games=0,wins=0;double maximum=0;
    for(int k:{4,8,15,24})for(int mode=0;mode<4;++mode) {
        if(argc>4 && k!=std::stoi(argv[4]))continue;
        if(argc>5 && mode!=std::stoi(argv[5]))continue;
        int local=0;
        for(int seed=0;seed<seeds;++seed)for(int seat=0;seat<2;++seat) {
            std::mt19937 gen(20261003+seed*1009+k*97+mode*53);rng.state=0x6a09e667f3bcc909ULL+seed;
            GameState g;g.k=k;g.phase="add";g.board={{-4,3,-1}};g.torque_left=-6;g.torque_right=6;
            g.game_id=std::to_string(++games);g.clocks[seat]=clock;
            for(int p=0;p<2;++p)for(int w=1;w<=k;++w)g.remaining[p].push_back(w);
            State s=from_game(g);int winner=-1,plies=0;double used=0;std::string reason;
            while(winner<0&&plies<125) {
                int turn=s.turn;Action m;bool valid=true;auto start=Clock::now();
                if(turn==seat){g.player=turn;g.phase=s.adding?"add":"remove";g.torque_left=-s.a;g.torque_right=s.b;g.clocks[seat]=clock-used;g.board.clear();
                    for(int i=0;i<61;++i)if(s.occupied&(U64(1)<<i))g.board.push_back({i-30,s.weight[i],-1});
                    for(int p=0;p<2;++p){g.remaining[p].clear();U64 ws=s.inventory[p];while(ws){g.remaining[p].push_back(first(ws)+1);ws&=ws-1;}}
                    Move move=choose_move(g);m={move.position+30,move.weight,0};
                    used+=std::chrono::duration<double>(Clock::now()-start).count();
                    valid=m.slot>=0&&m.slot<=60&&move.has_weight==s.adding;
                    if(s.adding)valid=valid&&m.weight>=1&&m.weight<=k&&(s.inventory[turn]&(U64(1)<<(m.weight-1)))&&!(s.occupied&(U64(1)<<m.slot));
                    else valid=valid&&(s.occupied&(U64(1)<<m.slot));
                    if(used>=clock){winner=seat^1;reason="timeout";break;}
                    if(!valid){winner=seat^1;reason="invalid";break;}
                }else m=opponent(s,mode,gen);
                s.apply(m);++plies;
                if(s.a<0||s.b<0){winner=turn^1;reason="tipping";}
            }
            bool win=winner==seat;wins+=win;local+=win;maximum=std::max(maximum,used);
            output<<k<<','<<names[mode]<<','<<seed<<','<<seat<<','<<win<<','<<reason<<','<<std::fixed<<std::setprecision(6)<<used<<','<<plies<<'\n';output.flush();
        }
        std::cout<<"k="<<k<<" "<<names[mode]<<" "<<local<<"/"<<2*seeds<<std::endl;
    }
    delete cached_solver;cached_solver=nullptr;
    std::cout<<"Total "<<wins<<"/"<<games<<"; maximum clock used "<<maximum<<" / "<<clock<<"\n";
}
