#include <bits/stdc++.h>
using namespace std;

struct Param {
    static constexpr double w_streak = 1148.6310;
    static constexpr double w_wrong = -145.6768;
    static constexpr double w_turn = -68.9185;
    static constexpr double w_dist = -94.6722;
    static constexpr double w_streak_m = 9503.1988;
    static constexpr double w_wrong_m = -7898.7681;
    static constexpr double w_turn_m = -31.3595;
    static constexpr double w_dist_m = -195.7998;

    // double w_streak;
    // double w_wrong;
    // double w_turn;
    // double w_dist;
    // double w_streak_m;
    // double w_wrong_m;
    // double w_turn_m;
    // double w_dist_m;

    // Param() {
    //     auto get_env = [](const char* name, double default_val) {
    //         if (const char* p = getenv(name)) return stod(p);
    //         return default_val;
    //     };

    //     w_streak = get_env("STREAK", 1148.6310);
    //     w_wrong = get_env("WRONG", -145.6768);
    //     w_turn = get_env("TURN", -68.9185);
    //     w_dist = get_env("DIST", -94.6722);
    //     w_streak_m = get_env("STREAK_M", 9503.1988);
    //     w_wrong_m = get_env("WRONG_M", -7898.7681);
    //     w_turn_m = get_env("TURN_M", -31.3595);
    //     w_dist_m = get_env("DIST_M", -195.7998);
    // }
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

struct Input {
    int N;  // 盤面サイズ
    int M;  // 好みの色列の長さ
    int C;  // 色数
    array<int, 256> d;  // 好みの色列
    array<int, 256> f;  // 初期状態において各マスに置かれている餌の色

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
private:

public:
    int snake_len;
    int turn;
    int action_idx;
    int game_score;
    double evaluated_score;
    uint64_t board_hash;
    array<int, 256> board;
    array<int, 256> snake_pos;
    array<int, 256> snake_color;

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

        update(in, param);
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
            case ('U'):
                head -= in.N;
                break;
            case ('D'):
                head += in.N;
                break;
            case ('L'):
                head--;
                break;
            case ('R'):
                head++;
                break;
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
        update(in, param);
    }

    bool is_finished(const Input& in) const {
        if (snake_len != in.M) return false;
        for (int i = 0; i < snake_len; i++) {
            if (snake_color[i] != in.d[i]) return false;
        }
        return true;
    }

    void calc_game_score(const Input& in) {
        int error = 0;
        for (int i = 0; i < snake_len; i++) {
            if (snake_color[i] != in.d[i]) error++;
        }
        game_score = turn + 10000 * (error + 2 * (in.M - snake_len));
    }

    void calc_evaluated_score(const Input& in, const Param& param) {
        int streak = 0;
        for (int i = 0; i < snake_len; i++) {
            if (snake_color[i] != in.d[i]) break;
            streak++;
        }

        const int wrong = snake_len - streak;
        
        // 目的の色までのマンハッタン距離を求める
        int min_dist = in.N * in.N;
        if (streak < in.M) {
            const int target_color = in.d[streak];
            const int curr_pos = snake_pos[0];
            
            for (int x = 0; x < in.N; ++x) {
                for (int y = 0; y < in.N; ++y) {
                    if (board[x * in.N + y] == target_color) {
                        const int d = abs(x - curr_pos / in.N) + abs(y - curr_pos % in.N);
                        if (d < min_dist) min_dist = d;
                    }
                }
            }
        }

        evaluated_score = streak * (param.w_streak + in.M * param.w_streak_m)
                        + wrong * (param.w_wrong + in.M * param.w_wrong_m)
                        + turn * (param.w_turn + in.M * param.w_turn_m)
                        + min_dist * (param.w_dist + in.M * param.w_dist_m);
    }

    void update(const Input& in, const Param& param) {
        calc_game_score(in);
        calc_evaluated_score(in, param);
    }

    uint64_t calc_hash(const Input& in, const ZobristHash& hash) const {
        uint64_t h = board_hash;

        h ^= hash.snake_len[snake_len];

        for (int i = 0; i < min(snake_len, hash.HASH_LEN); i++) {
            h ^= hash.snake_pos[i][snake_pos[i]];
        }

        int streak = 0;
        for (int i = 0; i < snake_len; i++) {
            if (snake_color[i] != in.d[i]) break;
            streak++;
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

    unordered_map<uint64_t, double> seen_hash;
    seen_hash.reserve(2000000);
    seen_hash[best_state.calc_hash(in, hash)] = best_state.evaluated_score;

    while (!time_keeper.is_time_over()) {
        if (curr_beam.empty()) break;

        vector<State> next_beam;
        bool all_finished = true;
        int iter = 0;

        for (const State& state : curr_beam) {
            iter++;
            if (iter % 10 == 0 && time_keeper.is_time_over()) break;

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

                const uint64_t h = next_state.calc_hash(in, hash);
                if (seen_hash.contains(h) && seen_hash[h] >= next_state.evaluated_score) continue;

                next_beam.emplace_back(next_state);
                seen_hash[h] = next_state.evaluated_score;

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

    // cerr << "actions size: " << ssize(actions) << "\n";
    // cerr << "seen_hash size: " << ssize(seen_hash) << "\n";

    return best_actions;
}

int main() {
    constexpr int TIME_LIMIT = 1800;
    TimeKeeper time_keeper(TIME_LIMIT);

    const Param param;
    const ZobristHash hash;
    const Input in = read_input();

    State best_state(in, param, hash);

    constexpr int BEAM_WIDTH = 100;
    const vector<char> actions = beam_search(in, param, hash, time_keeper, BEAM_WIDTH);
    for (const char a : actions) cout << a << "\n";

    return 0;
}