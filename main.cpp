#pragma GCC target("avx2")
#pragma GCC optimize("Ofast")
#pragma GCC optimize("unroll-loops")
#include <bits/stdc++.h>
using namespace std;
#define rep(i, n) for (int i = 0; i < (int)(n); i++)
#define ALL(v) (v).begin(), (v).end()
using P = pair<int, int>;

constexpr int N = 16, M = 5000, T = 1000;
constexpr int dx[] = {0, 1, 0, -1}, dy[] = {1, 0, -1, 0};

int BEAM_SIZE[] = {3, 10};
constexpr int LIMIT[] = {800, 870};
constexpr int DAY_DEPTH = 60;

struct Vegetable {
    int r, c, s, e, v;
    Vegetable(int r_, int c_, int s_, int e_, int v_)
        : r(r_), c(c_), s(s_), e(e_), v(v_) {}
};

vector<Vegetable>
    veges_start[T];              // veges_start[i] : vegetables appear on day i
vector<Vegetable> veges_end[T];  // veges_end[i] : vegetables disappear on day i

vector<P> dists[N][N][(N - 1) * 2 + 1];

struct Action {
    vector<int> vs;

   private:
    explicit Action(const vector<int>& vs_) : vs(vs_) {}

   public:
    static Action pass() {
        return Action({-1});
    }

    static Action purchase(int r, int c) {
        return Action({r, c});
    }

    static Action move(int r1, int c1, int r2, int c2) {
        return Action({r1, c1, r2, c2});
    }
};

struct DetailedAction {
    double score;
    int pre_game;
    Action action;
    bool stop_purchase;

    DetailedAction(double score, int pre_game, Action action)
        : score(score),
          pre_game(pre_game),
          action(action),
          stop_purchase(false) {}

    bool operator<(const DetailedAction detailed_action) const {
        return score < detailed_action.score;
    }
};

struct Game {
    bool has_machine[N][N];
    int vege_values[N][N];
    int num_machine;
    int next_price;
    int money;

    int vege_end[N][N];

    int assets;    // money + value of mashine
    double score;  // assets + board_score

    bool stop_purchase;

    int pre_game;
    Action pre_action = Action::pass();

    Game() : num_machine(0), next_price(1), money(1) {
        fill(has_machine[0], has_machine[N], false);
        fill(vege_values[0], vege_values[N], 0);
        stop_purchase = false;
    }

    void purchase(int r, int c) {
        // assert(!has_machine[r][c]);
        // assert(next_price <= money);
        has_machine[r][c] = 1;
        money -= next_price;
        num_machine++;
        next_price = (num_machine + 1) * (num_machine + 1) * (num_machine + 1);
    }

    void move(int r1, int c1, int r2, int c2) {
        // assert(has_machine[r1][c1]);
        has_machine[r1][c1] = 0;
        // assert(!has_machine[r2][c2]);
        has_machine[r2][c2] = 1;
    }

    void simulate(int day, const Action& action) {
        // apply
        if (action.vs.size() == 2) {
            purchase(action.vs[0], action.vs[1]);
        } else if (action.vs.size() == 4) {
            move(action.vs[0], action.vs[1], action.vs[2], action.vs[3]);
        }
        // appear
        for (const Vegetable& vege : veges_start[day]) {
            vege_values[vege.r][vege.c] = vege.v;
            vege_end[vege.r][vege.c] = vege.e;
        }
        // harvest
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                if (has_machine[i][j] && vege_values[i][j] > 0) {
                    money += vege_values[i][j] * num_machine;
                    assets += vege_values[i][j] * num_machine;
                    vege_values[i][j] = 0;
                }
            }
        }
        // disappear
        for (const Vegetable& vege : veges_end[day]) {
            vege_values[vege.r][vege.c] = 0;
        }
        pre_action = action;
    }

    void detailed_simulate(int day, const DetailedAction& detailed_action) {
        simulate(day, detailed_action.action);
        pre_game = detailed_action.pre_game;
        stop_purchase |= detailed_action.stop_purchase;
        score = detailed_action.score;
    }

    int count_connected_machines(int r, int c) {
        vector<pair<int, int>> queue = {{r, c}};
        vector<vector<bool>> visited(N, vector<bool>(N, 0));
        visited[r][c] = 1;
        int i = 0;
        while (i < (int)queue.size()) {
            int cr = queue[i].first;
            int cc = queue[i].second;
            for (int dir = 0; dir < 4; dir++) {
                int nr = cr + dx[dir];
                int nc = cc + dy[dir];
                if (0 <= nr && nr < N && 0 <= nc && nc < N &&
                    has_machine[nr][nc] && !visited[nr][nc]) {
                    visited[nr][nc] = 1;
                    queue.push_back({nr, nc});
                }
            }
            i++;
        }
        return i;
    }

    bool connected(int r, int c) {
        return count_connected_machines(r, c) == num_machine;
    }

    static bool comp_board(const tuple<double, int, int>& a,
                           const tuple<double, int, int>& b) {
        return get<0>(a) < get<0>(b);
    }

    static bool comp_move(const pair<double, Action>& a,
                          const pair<double, Action>& b) {
        return a.first < b.first;
    }

    vector<DetailedAction> day_0(int day, int game) {  // day0
        vector<DetailedAction> detailed_actions;
        for (int d = day; d < T; d++) {
            for (const Vegetable& vege : veges_start[d]) {
                detailed_actions.emplace_back(vege.v, game,
                                              Action::purchase(vege.r, vege.c));
                if ((int)detailed_actions.size() >= BEAM_SIZE[0]) break;
            }
            if ((int)detailed_actions.size() >= BEAM_SIZE[0]) break;
        }
        return detailed_actions;
    }

    vector<vector<double>> calc_board_score(int day) {
        vector<vector<double>> board_score(N, vector<double>(N));
        rep(r, N) {
            rep(c, N) {
                if (vege_values[r][c]) {
                    bool fin = false;
                    for (int dist = 1; dist <= (N - 1) * 2; dist++) {
                        for (auto nrnc : dists[r][c][dist]) {
                            int nr, nc;
                            tie(nr, nc) = nrnc;
                            if (day + dist <= vege_end[r][c])
                                board_score[nr][nc] +=
                                    (double)vege_values[r][c] / (dist + 1);
                            if (has_machine[nr][nc]) fin = true;
                        }
                        if (fin) break;
                    }
                }
            }
        }
        for (int i = 1; i < DAY_DEPTH && day + i < T; i++) {
            for (const Vegetable& vege : veges_start[day + i]) {
                board_score[vege.r][vege.c] += vege.v / (i + 1);
                bool fin = false;
                for (int dist = 1; dist <= (N - 1) * 2; dist++) {
                    for (auto rc : dists[vege.r][vege.c][dist]) {
                        int r, c;
                        tie(r, c) = rc;
                        if (day + dist <= vege.e)
                            board_score[r][c] +=
                                (double)vege.v / ((i + 1) * (dist + 1));
                        if (has_machine[r][c]) fin = true;
                    }
                    if (fin) break;
                }
            }
        }
        return board_score;
    }

    vector<vector<int>> calc_board_assets(int day) {
        vector<vector<int>> board_assets(N, vector<int>(N));
        rep(r, N) {
            rep(c, N) {
                board_assets[r][c] += vege_values[r][c];
            }
        }
        for (const Vegetable& vege : veges_start[day]) {
            board_assets[vege.r][vege.c] += vege.v;
        }
        return board_assets;
    }

    vector<DetailedAction> move_(int day, int game) {
        vector<tuple<double, int, int>> add, del;
        auto board_score = calc_board_score(day);
        auto board_assets = calc_board_assets(day);
        if (num_machine != 1) {
            bool visited[N][N];
            fill(visited[0], visited[N], false);
            rep(r, N) {
                rep(c, N) {
                    if (!has_machine[r][c]) continue;
                    rep(i, 4) {
                        int nr = r + dx[i], nc = c + dy[i];
                        if (nr < 0 || N <= nr || nc < 0 || N <= nc) continue;
                        if (has_machine[nr][nc]) continue;
                        if (visited[nr][nc]) continue;
                        add.emplace_back(
                            board_score[nr][nc] + board_assets[nr][nc], nr, nc);
                        visited[nr][nc] = true;
                    }
                }
            }
        } else {
            rep(r, N) {
                rep(c, N) {
                    if (!has_machine[r][c]) {
                        add.emplace_back(board_score[r][c] + board_assets[r][c],
                                         r, c);
                    }
                }
            }
        }
        rep(r, N) {
            rep(c, N) {
                if (has_machine[r][c]) {
                    if (0 <= r - 1 && r + 1 < N && has_machine[r - 1][c] &&
                        has_machine[r + 1][c])
                        continue;
                    if (0 <= c - 1 && c + 1 < N && has_machine[r][c - 1] &&
                        has_machine[r][c + 1])
                        continue;
                    del.emplace_back(board_score[r][c] + board_assets[r][c], r,
                                     c);
                }
            }
        }
        double current_score = 0;
        rep(r, N) {
            rep(c, N) {
                if (has_machine[r][c]) {
                    current_score += board_score[r][c] + board_assets[r][c];
                }
            }
        }
        sort(ALL(add), comp_board);
        reverse(ALL(add));
        sort(ALL(del), comp_board);
        vector<DetailedAction> detailed_actions;
        for (int rank = 0; rank < (int)add.size() + (int)del.size(); rank++) {
            for (int i = max(0, rank - (int)del.size() + 1);
                 i < (int)add.size() && rank - i >= 0; i++) {
                int ar = get<1>(add[i]), ac = get<2>(add[i]),
                    dr = get<1>(del[rank - i]), dc = get<2>(del[rank - i]);
                has_machine[dr][dc] = false;
                has_machine[ar][ac] = true;
                if (connected(ar, ac)) {
                    has_machine[dr][dc] = true;
                    has_machine[ar][ac] = false;
                    if (get<0>(add[i]) - get<0>(del[rank - i]) < 0) {
                        detailed_actions.emplace_back(assets + current_score,
                                                      game, Action::pass());
                        if ((int)detailed_actions.size() >= BEAM_SIZE[0]) break;
                    }
                    detailed_actions.emplace_back(
                        assets + current_score + get<0>(add[i]) -
                            get<0>(del[rank - i]),
                        game, Action::move(dr, dc, ar, ac));
                    if ((int)detailed_actions.size() >= BEAM_SIZE[0]) break;
                } else {
                    has_machine[dr][dc] = true;
                    has_machine[ar][ac] = false;
                }
            }
            if ((int)detailed_actions.size() >= BEAM_SIZE[0]) break;
        }
        return detailed_actions;
    }

    vector<DetailedAction> purchase_(int day, int game) {
        vector<tuple<double, int, int>> add;
        auto board_score = calc_board_score(day);
        auto board_assets = calc_board_assets(day);
        bool visited[N][N];
        fill(visited[0], visited[N], false);
        rep(r, N) {
            rep(c, N) {
                if (!has_machine[r][c]) continue;
                rep(i, 4) {
                    int nr = r + dx[i], nc = c + dy[i];
                    if (nr < 0 || N <= nr || nc < 0 || N <= nc) continue;
                    if (has_machine[nr][nc]) continue;
                    if (visited[nr][nc]) continue;
                    add.emplace_back(board_score[nr][nc] + board_assets[nr][nc],
                                     nr, nc);
                    visited[nr][nc] = true;
                }
            }
        }
        double current_score = 0;
        rep(r, N) {
            rep(c, N) {
                if (has_machine[r][c]) {
                    current_score += board_score[r][c] + board_assets[r][c];
                }
            }
        }
        sort(ALL(add), comp_board);
        reverse(ALL(add));
        vector<DetailedAction> detailed_actions;
        for (int i = 0; i < min(BEAM_SIZE[0], (int)add.size()); i++) {
            detailed_actions.emplace_back(
                assets + current_score + get<0>(add[i]), game,
                Action::purchase(get<1>(add[i]), get<2>(add[i])));
        }
        return detailed_actions;
    }

    vector<DetailedAction> must_purchase(
        int day,
        int game) {  // If you can purchase machine, you must purchase it.
        if (money < next_price) {  // move
            return move_(day, game);
        } else {  // purchase
            return purchase_(day, game);
        }
    }

    vector<DetailedAction> purchase_and_move(int day, int game) {
        if (money >= next_price && !stop_purchase) {  // can purchase
            vector<DetailedAction> detailed_actions;
            detailed_actions = purchase_(day, game);
            auto v = move_(day, game);
            detailed_actions.insert(detailed_actions.end(), v.begin(), v.end());
            detailed_actions.back().stop_purchase = true;
            return detailed_actions;
        } else {
            return move_(day, game);
        }
    }

    vector<DetailedAction> must_move(int day, int game) {  // DON'T purchase.
        return move_(day, game);
    }

    bool operator<(const Game game) const {
        return score < game.score;
    }
};

vector<Action> solve() {
    vector<Game> que[T];
    // day = 0
    Game zero;
    auto detailed_action = zero.day_0(0, -1);
    for (DetailedAction& detailed_action : detailed_action) {
        Game game;
        game.detailed_simulate(0, detailed_action);
        que[0].emplace_back(game);
    }
    // must purchase
    for (int day = 1; day < LIMIT[0]; day++) {
        vector<DetailedAction> detailed_actions;
        set<double> hash;
        for (int i = 0; i < (int)que[day - 1].size(); i++) {
            auto detailed_action = que[day - 1][i].must_purchase(day, i);
            for (DetailedAction& detailed_action : detailed_action) {
                if (hash.count(detailed_action.score)) continue;
                hash.emplace(detailed_action.score);
                detailed_actions.emplace_back(detailed_action);
            }
        }
        sort(ALL(detailed_actions));
        reverse(ALL(detailed_actions));
        for (int i = 0; i < min(BEAM_SIZE[1], (int)detailed_actions.size());
             i++) {
            Game game = que[day - 1][detailed_actions[i].pre_game];
            game.detailed_simulate(day, detailed_actions[i]);
            que[day].emplace_back(game);
        }
    }
    while ((int)que[LIMIT[0] - 1].size() > BEAM_SIZE[1]) {
        que[LIMIT[0] - 1].pop_back();
    }
    BEAM_SIZE[0] = 1;
    // purchase and move
    for (int day = LIMIT[0]; day < LIMIT[1]; day++) {
        for (int i = 0; i < (int)que[day - 1].size(); i++) {
            auto detailed_action = que[day - 1][i].purchase_and_move(day, i);
            for (DetailedAction& detailed_action : detailed_action) {
                Game game = que[day - 1][i];
                game.detailed_simulate(day, detailed_action);
                que[day].emplace_back(game);
            }
        }
    }
    // must move
    for (int day = LIMIT[1]; day < T; day++) {
        for (int i = 0; i < (int)que[day - 1].size(); i++) {
            auto detailed_action = que[day - 1][i].must_move(day, i);
            for (DetailedAction& detailed_action : detailed_action) {
                Game game = que[day - 1][i];
                game.detailed_simulate(day, detailed_action);
                que[day].emplace_back(game);
            }
        }
    }
    int idx = -1;
    int mx = 0;
    for (int i = 0; i < (int)que[T - 1].size(); i++) {
        if (que[T - 1][i].money > mx) {
            idx = i;
            mx = que[T - 1][i].money;
        }
    }
    vector<Action> actions;
    for (int i = T - 1; i >= 0; i--) {
        actions.emplace_back(que[i][idx].pre_action);
        idx = que[i][idx].pre_game;
    }
    reverse(ALL(actions));
    return actions;
}

void init() {
    rep(r, N) {
        rep(c, N) {
            rep(nr, N) {
                rep(nc, N) {
                    dists[r][c][abs(r - nr) + abs(c - nc)].emplace_back(nr, nc);
                }
            }
        }
    }
}

void input() {
    int n, m, t;
    cin >> n >> m >> t;
    for (int i = 0; i < M; i++) {
        int r, c, s, e, v;
        cin >> r >> c >> s >> e >> v;
        Vegetable vege(r, c, s, e, v);
        veges_start[s].push_back(vege);
        veges_end[e].push_back(vege);
    }
}

void output(vector<Action>& actions) {
    for (const Action& action : actions) {
        for (int i = 0; i < (int)action.vs.size(); i++) {
            cout << action.vs[i]
                 << (i == (int)action.vs.size() - 1 ? "\n" : " ");
        }
    }
}

int main() {
    init();
    input();
    auto actions = solve();
    output(actions);
}