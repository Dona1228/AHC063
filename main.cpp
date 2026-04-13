#include <bits/stdc++.h>
using namespace std;

struct Param {
    double w_streak;
    double w_wrong;
    double w_turn;
    double w_dist;
    double w_streak_m;
    double w_wrong_m;
    double w_turn_m;
    double w_dist_m;

    Param() {
        auto get_env = [](const char* name, double default_val) {
            if (const char* p = getenv(name)) return stod(p);
            return default_val;
        };

        w_streak = get_env("STREAK", 3615.5403);
        w_wrong = get_env("WRONG", -348.9116);
        w_turn = get_env("TURN", -11.9009);
        w_dist = get_env("DIST", -159.2805);
        w_streak_m = get_env("STREAK_M", 33815.5102);
        w_wrong_m = get_env("WRONG_M", -33298.8325);
        w_turn_m = get_env("TURN_M", -11.7797);
        w_dist_m = get_env("DIST_M", -52.0443);
    }
};

unsigned int xorshift() {
    static unsigned int tx = 123456789;
    static unsigned int ty = 362436069;
    static unsigned int tz = 521288629;
    static unsigned int tw = 88675123;
    unsigned int tt = (tx ^ (tx << 11));
    tx = ty;
    ty = tz;
    tz = tw;
    return (tw = (tw ^ (tw >> 19)) ^ (tt ^ (tt >> 8)));
}

uint64_t xorshift64() {
    const uint64_t high = static_cast<uint64_t>(xorshift());
    const uint64_t low = static_cast<int64_t>(xorshift());
    return (high << 32) | low;
}

struct ZobristHash {
    static constexpr int HASH_LEN = 8;
    array<array<uint64_t, 8>, 256> board;  // マスの座標、色
    array<array<uint64_t, 256>, HASH_LEN> snake_pos;  // ヘビの頭からのインデックス、座標
    array<uint64_t, 256> snake_len;  // ヘビの長さ
    array<uint64_t, 256> streak;  // 頭から何マス連続して好みの色をそろえているか

    ZobristHash() {
        for (int i = 0; i < 256; i++) {
            for (int j = 0; j < 8; j++) board[i][j] = xorshift64();
            for (int j = 0; j < HASH_LEN; j++) snake_pos[j][i] = xorshift64();
            snake_len[i] = xorshift64();
            streak[i] = xorshift64();
        }
    }
};

class TimeKeeper {
private:
    chrono::high_resolution_clock::time_point start_time;
    chrono::high_resolution_clock::time_point end_time;

public:
    int time_threshold;

    TimeKeeper(int time_threshold)
        : start_time(chrono::high_resolution_clock::now()),
          time_threshold(time_threshold) {
        end_time = start_time + chrono::milliseconds(time_threshold);
    }

    bool is_time_over() const {
        return chrono::high_resolution_clock::now() >= end_time;
    }

    int get_elapsed_time() const {
        auto diff = chrono::high_resolution_clock::now() - start_time;
        return chrono::duration_cast<chrono::milliseconds>(diff).count();
    }
};

struct Node {
    int parent;
    char action;
};

// unordered_mapの代わり
class HashTable {
private:
    struct Entry {
        uint64_t key;
        double val;
    };
    vector<Entry> table;
    uint32_t mask;

public:
    HashTable(int capacity_pow2) {
        table.assign(1 << capacity_pow2, {0, -1e18});
        mask = (1 << capacity_pow2) - 1;
    }
    
    bool is_worse_or_equal(uint64_t key, double val) {
        if (key == 0) key = 1;
        uint32_t idx = key & mask;
        while (table[idx].key != 0 && table[idx].key != key) {
            idx = (idx + 1) & mask;
        }
        if (table[idx].key == key) {
            if (table[idx].val >= val) return true;
            table[idx].val = val;
            return false;
        }
        table[idx].key = key;
        table[idx].val = val;
        return false;
    }
};

struct Input {
    int N;  // 盤面サイズ
    int M;  // 好みの色列の長さ
    int C;  // 色数
    array<int16_t, 256> d;  // 好みの色列
    array<int16_t, 256> f;  // 初期状態において各マスに置かれている餌の色

    bool is_in_grid(const int pos) const {
        const int x = pos / N;
        const int y = pos % N;
        return 0 <= x && x < N && 0 <= y && y < N;
    }
};

Input read_input() {
    ios_base::sync_with_stdio(false);
    cin.tie(nullptr);

    Input in;
    cin >> in.N >> in.M >> in.C;
    for (int i = 0; i < in.M; i++) cin >> in.d[i];
    for (int i = 0; i < in.N; i++) {
        for (int j = 0; j < in.N; j++) {
            cin >> in.f[i * in.N + j];
        }
    }
    return in;
}

class State {
public:
    int snake_len;
    int streak;
    int turn;
    int action_idx;
    int game_score;
    double evaluated_score;
    uint64_t board_hash;
    array<int16_t, 256> board;
    array<int16_t, 256> snake_pos;
    array<int16_t, 256> snake_color;

    State() {}

    State(const Input& in, const Param& param, const ZobristHash& hash) {
        board = in.f;
        snake_len = 5;
        turn = 0;
        action_idx = -1;

        for (int i = 0; i < 5; i++) {
            snake_pos[i] = (5 - i - 1) * in.N + 0;
            snake_color[i] = 1;
        }
        
        board_hash = 0;
        for (int i = 0; i < 256; i++) {
            board_hash ^= hash.board[i][board[i]];
        }

        calc_score(in, param);
    }

    bool operator<(const State& other) const {
        return evaluated_score < other.evaluated_score;
    }

    bool can_move(const Input& in, const char dir) const {
        int head = snake_pos[0];
        switch (dir) {
            case ('U'):
                if (head / in.N == 0) return false;
                head -= in.N;
                break;
            case ('D'):
                if (head / in.N == in.N - 1) return false;
                head += in.N;
                break;
            case ('L'):
                if (head % in.N == 0) return false;
                head--;
                break;
            case ('R'):
                if (head % in.N == in.N - 1) return false;
                head++;
                break;
        }
        if (!in.is_in_grid(head)) return false;
        if (head == snake_pos[1]) return false;
        return true;
    }

    void move(const Input& in, const Param& param, const ZobristHash& hash, const char dir) {
        // 移動
        int head = snake_pos[0];
        switch (dir) {
            case ('U'): head -= in.N; break;
            case ('D'): head += in.N; break;
            case ('L'): head--; break;
            case ('R'): head++; break;
        }
        for (int i = snake_len; i > 0; i--) snake_pos[i] = snake_pos[i - 1];
        snake_pos[0] = head;

        // 食事
        if (board[head] != 0) {
            board_hash ^= hash.board[head][board[head]];
            board_hash ^= hash.board[head][0];

            snake_color[snake_len] = board[head];
            snake_len++;
            board[head] = 0;
        }

        // 噛みちぎり
        for (int i = 1; i < snake_len; i++) {
            if (head == snake_pos[i]) {
                for (int j = i + 1; j < snake_len; j++) {
                    board_hash ^= hash.board[snake_pos[j]][0];
                    board_hash ^= hash.board[snake_pos[j]][snake_color[j]];
                    board[snake_pos[j]] = snake_color[j];
                }
                snake_len = i + 1;
                break;
            }
        }

        turn++;
        calc_score(in, param);
    }

    bool is_finished(const Input& in) const {
        return snake_len == in.M && streak == in.M;
    }

    void calc_score(const Input& in, const Param& param) {
        streak = 0;
        int error = 0;
        bool is_streaking = true;
        
        for (int i = 0; i < snake_len; i++) {
            if (snake_color[i] != in.d[i]) {
                error++;
                is_streaking = false;
            } else if (is_streaking) {
                streak++;
            }
        }
        
        game_score = turn + 10000 * (error + 2 * (in.M - snake_len));
        const int wrong = snake_len - streak;
        
        // --- 目的の色までの距離を求める (Lookahead機能追加) ---
        double distance_penalty = 0.0;
        
        if (streak < in.M) {
            const int target_color_1 = in.d[streak];
            const int curr_x = snake_pos[0] / in.N;
            const int curr_y = snake_pos[0] % in.N;
            
            int min_dist_1 = in.N * in.N;
            int best_target1_x = -1;
            int best_target1_y = -1;
            
            // 目標1（次に必要な色）への最短距離を探す
            for (int x = 0; x < in.N; x++) {
                for (int y = 0; y < in.N; y++) {
                    if (board[x * in.N + y] == target_color_1) {
                        const int d = abs(x - curr_x) + abs(y - curr_y);
                        if (d < min_dist_1) {
                            min_dist_1 = d;
                            best_target1_x = x;
                            best_target1_y = y;
                        }
                    }
                }
            }
            distance_penalty += min_dist_1;

            // 目標2（その次に必要な色）への最短距離を考慮（Lookahead）
            if (streak + 1 < in.M && best_target1_x != -1) {
                const int target_color_2 = in.d[streak + 1];
                int min_dist_2 = in.N * in.N;
                
                for (int x = 0; x < in.N; x++) {
                    for (int y = 0; y < in.N; y++) {
                        // 目標1として食べる予定のマスは除外
                        if (x == best_target1_x && y == best_target1_y) continue;
                        
                        if (board[x * in.N + y] == target_color_2) {
                            const int d = abs(x - best_target1_x) + abs(y - best_target1_y);
                            if (d < min_dist_2) {
                                min_dist_2 = d;
                            }
                        }
                    }
                }
                // 目標2への距離は少し影響力を下げる（この係数0.5は要調整）
                distance_penalty += min_dist_2 * 0.5; 
            }
        }

        // 評価値の計算（min_dist を distance_penalty に置き換え）
        evaluated_score = streak * (param.w_streak + in.M * param.w_streak_m)
                        + wrong * (param.w_wrong + in.M * param.w_wrong_m)
                        + turn * (param.w_turn + in.M * param.w_turn_m)
                        + distance_penalty * (param.w_dist + in.M * param.w_dist_m);
    }

    uint64_t calc_hash(const ZobristHash& hash) const {
        uint64_t h = board_hash;
        h ^= hash.snake_len[snake_len];
        for (int i = 0; i < min(snake_len, hash.HASH_LEN); i++) {
            h ^= hash.snake_pos[i][snake_pos[i]];
        }
        h ^= hash.streak[streak];
        return h;
    }
};

vector<char> beam_search(const Input& in, const Param& param, const ZobristHash& hash, TimeKeeper& time_keeper, const int beam_width) {
    constexpr array<char, 4> dir{'U', 'D', 'L', 'R'};

    vector<Node> actions;
    actions.reserve(2000000);

    State best_state(in, param, hash);

    vector<State> curr_beam;
    curr_beam.reserve(beam_width);
    curr_beam.emplace_back(best_state);

    HashTable seen_hash(22);
    seen_hash.is_worse_or_equal(best_state.calc_hash(hash), best_state.evaluated_score);

    while (!time_keeper.is_time_over()) {
        if (curr_beam.empty()) break;

        vector<State> next_beam;
        bool all_finished = true;

        for (const State& state : curr_beam) {

            if (state.is_finished(in)) {
                next_beam.emplace_back(state);
                continue;
            }

            all_finished = false;

            for (const char d : dir) {
                if (!state.can_move(in, d)) continue;

                State next_state = state;
                next_state.move(in, param, hash, d);
                next_state.action_idx = ssize(actions);
                actions.emplace_back(state.action_idx, d);

                const uint64_t h = next_state.calc_hash(hash);
                if (seen_hash.is_worse_or_equal(h, next_state.evaluated_score)) continue;
                next_beam.emplace_back(next_state);

                if (next_state.game_score < best_state.game_score) best_state = next_state;
            }
        }

        if (all_finished) break;

        if (ssize(next_beam) > beam_width) {
            nth_element(next_beam.begin(), next_beam.begin() + beam_width, next_beam.end(), [](const State& a, const State& b) { return a.evaluated_score > b.evaluated_score; });
            
            next_beam.resize(beam_width);
        }

        curr_beam = move(next_beam);
    }
    
    vector<char> best_actions;
    int curr = best_state.action_idx;
    while (curr != -1) {
        best_actions.emplace_back(actions[curr].action);
        curr = actions[curr].parent;
    }
    ranges::reverse(best_actions);

    return best_actions;
}

int choose_beam_width(const Input& in) {
    if (in.M < 100) return 300;
    else return 100;
}

int main() {
    constexpr int TIME_LIMIT = 1900;
    TimeKeeper time_keeper(TIME_LIMIT);

    const Param param;
    const ZobristHash hash;
    const Input in = read_input();

    const int beam_width = choose_beam_width(in);
    const vector<char> actions = beam_search(in, param, hash, time_keeper, beam_width);
    for (const char a : actions) cout << a << "\n";

    return 0;
}