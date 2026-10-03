#include "../bots/fulcrum/strategy.cpp"
#include <iostream>
int full(State s){
    if(!s.adding){RemovalSolver exact(s);return exact.solve(s.occupied,s.a,s.b).value;}
    auto moves=add_actions(s);if(moves.empty())return -1;
    for(auto m:moves){State t=s;t.apply(m);int result=full(t);int winner=result>0?t.turn:t.turn^1;if(winner==s.turn)return 1;}
    return -1;
}
int main(){GameState g;g.k=1;g.phase="add";g.board={{-4,3,-1}};g.remaining[0]={1};g.remaining[1]={1};g.torque_left=-6;g.torque_right=6;State s=from_game(g);std::cout<<"k=1 full-game oracle, player 0: "<<(full(s)>0?"WIN":"LOSS")<<"\n";}
