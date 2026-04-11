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

        w_streak = get_env("STREAK", 204.98);
        w_wrong = get_env("WRONG", -542.55);
        w_turn = get_env("TURN", -8.955);
        w_dist = get_env("DIST", -8.313);
        w_streak_m = get_env("STREAK_M", 998.02);
        w_wrong_m = get_env("WRONG_M", -1021.07);
        w_turn_m = get_env("TURN_M", -2.965);
        w_dist_m = get_env("DIST_M", -60.292);
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
    uint64_t high = static_cast<uint64_t>(xorshift());
    uint64_t low = static_cast<int64_t>(xorshift());
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

struct Input {
    int N;  // 盤面サイズ
    int M;  // 好みの色列の長さ
    int C;  // 色数
    array<int, 256> d;  // 好みの色列
    array<int, 256> f;  // 初期状態において各マスに置かれている餌の色

    bool is_in_grid(const int pos) const {
        int x = pos / N;
        int y = pos % N;
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
    int game_score;
    double evaluated_score;
    uint64_t board_hash;
    array<int, 256> board;
    array<int, 256> snake_pos;
    array<int, 256> snake_color;
    vector<char> actions;

    State(const Input& in, const Param& param, const ZobristHash& hash) {
        board = in.f;
        snake_len = 5;
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

        actions.emplace_back(dir);
        update(in, param);
    }

    bool is_finished(const Input& in) {
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
        game_score = ssize(actions) + 10000 * (error + 2 * (in.M - snake_len));
    }

    void calc_evaluated_score(const Input& in, const Param& param) {
        int streak = 0;
        for (int i = 0; i < snake_len; i++) {
            if (snake_color[i] != in.d[i]) break;
            streak++;
        }

        int wrong = snake_len - streak;

        int turn = ssize(actions);
        
        // 目的の色までのマンハッタン距離を求める
        int min_dist = in.N * in.N;
        if (streak < in.M) {
            int target_color = in.d[streak];
            int curr_pos = snake_pos[0];
            
            for (int x = 0; x < in.N; ++x) {
                for (int y = 0; y < in.N; ++y) {
                    if (board[x * in.N + y] == target_color) {
                        int d = abs(x - curr_pos / in.N) + abs(y - curr_pos % in.N);
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

    void print_actions() {
        for (const auto a : actions) cout << a << "\n";
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

State simple_zigzag(const Input& in, const Param& param, const ZobristHash& hash) {
    State state(in, param, hash);
    State best_state = state;

    for (int x = 4; x < in.N - 1; x++) {
        state.move(in, param, hash, 'D');
        if (state.game_score < best_state.game_score) best_state = state;
    }

    for (int y = 1; y < in.N; y++) {
        state.move(in, param, hash, 'R');
        if (state.game_score < best_state.game_score) best_state = state;
        if (y % 2 == 1) {
            for (int x = 0; x < in.N - 1; x++) state.move(in, param, hash, 'U');
            if (state.game_score < best_state.game_score) best_state = state;
        } else {
            for (int x = 0; x < in.N - 1; x++) state.move(in, param, hash, 'D');
            if (state.game_score < best_state.game_score) best_state = state;
        }
    }

    return best_state;
}

State bfs_all_route(const Input& in, const Param& param, const ZobristHash& hash) {
    array<char, 4> dir{'U', 'D', 'L', 'R'};

    State best_state(in, param, hash);

    queue<State> q;
    q.emplace(best_state);

    while (!q.empty()) {
        const State state = q.front();
        q.pop();

        if (state.game_score < best_state.game_score) best_state = state;

        for (const char d : dir) {
            if (!state.can_move(in, d)) continue;
            State next_state = state;
            next_state.move(in, param, hash, d);
            q.emplace(next_state);
        }
    }

    return best_state;
}

State greedy(const Input& in, const Param& param, const ZobristHash& hash, TimeKeeper& time_keeper) {
    const array<char, 4> dir{'U', 'D', 'L', 'R'};

    const double reject_prob = 0.5 * in.M / (3.0 * in.N * in.N / 4.0);

    State best_state(in, param, hash);

    while (!time_keeper.is_time_over()) {
        State state(in, param, hash);

        for (int t = 0; ssize(state.snake_color) < in.M; t++) {
            int target_color = in.d[ssize(state.snake_color)];
            if (xorshift() % 100 < reject_prob * 100) target_color = -1;
            
            const int start_pos = state.snake_pos[0];
            int target_pos = -1;

            array<int, 256> dist;
            ranges::fill(dist, -1);
            dist[start_pos] = 0;

            queue<int> q;
            q.emplace(start_pos);

            while (!q.empty()) {
                const int curr_pos = q.front();
                q.pop();

                if (state.board[curr_pos] == target_color) {
                    target_pos = curr_pos;
                    break;
                } else if (target_color == -1 && state.board[curr_pos] != 0) {
                    target_pos = curr_pos;
                    break;
                }
            
                for (const char d : dir) {
                    int next_pos = curr_pos;
                    switch(d) {
                        case ('U'):
                            next_pos -= in.N;
                            break;
                        case ('D'):
                            next_pos += in.N;
                            break;
                        case ('L'):
                            next_pos--;
                            break;
                        case ('R'):
                            next_pos++;
                            break;
                    }

                    if (!in.is_in_grid(next_pos)) continue;

                    if (find(state.snake_pos.begin() + 1, state.snake_pos.end(), next_pos) < state.snake_pos.end() - dist[curr_pos] - 1) continue;

                    if (dist[next_pos] != -1) continue;

                    if (state.board[next_pos] != 0 && state.board[next_pos] != target_color && target_color != -1) continue;

                    q.emplace(next_pos);
                    dist[next_pos] = dist[curr_pos] + 1;
                }
            }

            if (target_pos == -1) break;

            vector<char> path;
            int curr_pos = target_pos;
            while (curr_pos != start_pos) {
                if (in.is_in_grid({curr_pos - in.N}) && dist[curr_pos - in.N] == dist[curr_pos] - 1) {
                    curr_pos -= in.N;
                    path.emplace_back('D');
                } else if (in.is_in_grid({curr_pos + in.N}) && dist[curr_pos + in.N] == dist[curr_pos] - 1) {
                    curr_pos += in.N;
                    path.emplace_back('U');
                } else if (in.is_in_grid({curr_pos - 1}) && dist[curr_pos - 1] == dist[curr_pos] - 1) {
                    curr_pos--;
                    path.emplace_back('R');
                } else if (in.is_in_grid({curr_pos + 1}) && dist[curr_pos + 1] == dist[curr_pos] - 1) {
                    curr_pos++;
                    path.emplace_back('L');
                }
            }
            ranges::reverse(path);

            for (const char p : path) state.move(in, param, hash, p);
        }

        if (ssize(state.snake_color) != in.M) continue;
        if (state.game_score < best_state.game_score) best_state = state;
    }

    return best_state;
}

State custom_zigzag(const Input& in, const Param& param, const ZobristHash& hash) {
    State best_state(in, param, hash);

    State base_state(in, param, hash);
    for (int x = 4; x < in.N - 1; x++) base_state.move(in, param, hash, 'D');

    if (in.N % 2 == 0) {
        for (int bit = 0; bit < (1 << (in.N - 1)); bit++) {
            State state = base_state;
            state.move(in, param, hash, 'R');
            
            vector<bool> wall(in.N, false);
            for (int i = 0; i < in.N - 1; i++) {
                if ((bit >> i) & 1) wall[i + 1] = true;
            }
            wall[0] = true;

            int curr_row = in.N - 1;
            bool start_left = true;
            
            while (curr_row >= 0 && ssize(state.snake_color) != in.M) {
                int b = curr_row;
                int t = b;
                while (t > 0 && !wall[t]) t--;
                
                if (start_left) {
                    for (int y = 1; y < in.N; y++) {
                        if (y != 1) state.move(in, param, hash, 'R');
                        if (ssize(state.snake_color) == in.M) break;
                        
                        if ((y - 1) % 2 == 0) {
                            for (int x = b; x > t; x--) {
                                state.move(in, param, hash, 'U');
                                if (ssize(state.snake_color) == in.M) break;
                            }
                        } else {
                            for (int x = t; x < b; x++) {
                                state.move(in, param, hash, 'D');
                                if (ssize(state.snake_color) == in.M) break;
                            }
                        }
                    }
                    curr_row = t - 1;
                    start_left = false;
                    if (curr_row >= 0 && ssize(state.snake_color) != in.M) state.move(in, param, hash, 'U');
                } else {
                    for (int y = in.N - 1; y >= 1; y--) {
                        if (y != in.N - 1) state.move(in, param, hash, 'L');
                        if (ssize(state.snake_color) == in.M) break;
                        
                        if ((in.N - 1 - y) % 2 == 0) {
                            for (int x = b; x > t; x--) {
                                state.move(in, param, hash, 'U');
                                if (ssize(state.snake_color) == in.M) break;
                            }
                        } else {
                            for (int x = t; x < b; x++) {
                                state.move(in, param, hash, 'D');
                                if (ssize(state.snake_color) == in.M) break;
                            }
                        }
                    }
                    curr_row = t - 1;
                    start_left = true;
                    if (curr_row >= 0 && ssize(state.snake_color) != in.M) state.move(in, param, hash, 'U');
                }
            }

            if (ssize(state.snake_color) == in.M && state.game_score < best_state.game_score) best_state = state;
        }
    } else {
        for (int bit = 0; bit < (1 << (in.N - 2)); bit++) {
            State state = base_state;
            state.move(in, param, hash, 'R');
            
            vector<bool> wall(in.N, false);
            for (int i = 0; i < in.N - 2; i++) {
                if ((bit >> i) & 1) wall[i + 1] = true;
            }
            wall[in.N - 1] = true;

            int curr_col = 1;
            bool start_bottom = true;
            
            while (curr_col < in.N && ssize(state.snake_color) != in.M) {
                int l = curr_col;
                int r = l;
                while (r < in.N - 1 && !wall[r]) r++;
                
                if (start_bottom) {
                    for (int x = in.N - 1; x >= 0; x--) {
                        if (x != in.N - 1) state.move(in, param, hash, 'U');
                        if (ssize(state.snake_color) == in.M) break;
                        
                        if ((in.N - 1 - x) % 2 == 0) {
                            for (int y = l; y < r; y++) {
                                state.move(in, param, hash, 'R');
                                if (ssize(state.snake_color) == in.M) break;
                            }
                        } else {
                            for (int y = r; y > l; y--) {
                                state.move(in, param, hash, 'L');
                                if (ssize(state.snake_color) == in.M) break;
                            }
                        }
                    }
                    curr_col = r + 1;
                    start_bottom = false;
                    if (curr_col < in.N && ssize(state.snake_color) != in.M) state.move(in, param, hash, 'R');
                } else {
                    for (int x = 0; x < in.N; x++) {
                        if (x != 0) state.move(in, param, hash, 'D');
                        if (ssize(state.snake_color) == in.M) break;
                        
                        if (x % 2 == 0) {
                            for (int y = l; y < r; y++) {
                                state.move(in, param, hash, 'R');
                                if (ssize(state.snake_color) == in.M) break;
                            }
                        } else {
                            for (int y = r; y > l; y--) {
                                state.move(in, param, hash, 'L');
                                if (ssize(state.snake_color) == in.M) break;
                            }
                        }
                    }
                    curr_col = r + 1;
                    start_bottom = true;
                    if (curr_col < in.N && ssize(state.snake_color) != in.M) state.move(in, param, hash, 'R');
                }
            }

            if (ssize(state.snake_color) == in.M && state.game_score < best_state.game_score) best_state = state;
        }
    }

    return best_state;
}

State beam_search(const Input& in, const Param& param, const ZobristHash& hash, TimeKeeper& time_keeper, const int beam_width) {
    const array<char, 4> dir{'U', 'D', 'L', 'R'};

    State best_state(in, param, hash);

    priority_queue<State> curr_beam;
    curr_beam.emplace(best_state);

    while (!time_keeper.is_time_over()) {
        if (curr_beam.empty()) break;

        priority_queue<State> next_beam;
        bool all_finished = true;

        unordered_set<uint64_t> seen_hash;

        for (int i = 0; i < beam_width; i++) {
            if (curr_beam.empty()) break;

            State state = curr_beam.top();
            curr_beam.pop();

            if (state.is_finished(in)) {
                next_beam.emplace(state);
                continue;
            }

            all_finished = false;

            for (const char d : dir) {
                if (!state.can_move(in, d)) continue;

                State next_state = state;
                next_state.move(in, param, hash, d);

                uint64_t h = next_state.calc_hash(in, hash);
                if (seen_hash.contains(h)) continue;

                next_beam.emplace(next_state);
                seen_hash.emplace(h);

                if (next_state.game_score < best_state.game_score) best_state = next_state;
            }
        }

        curr_beam = next_beam;

        if (curr_beam.top().game_score < best_state.game_score) best_state = curr_beam.top();

        if (all_finished) break;
    }

    return best_state;
}

int main() {
    constexpr int TIME_LIMIT = 1950;
    TimeKeeper time_keeper(TIME_LIMIT);

    const Param param;
    const ZobristHash hash;
    const Input in = read_input();

    State best_state(in, param, hash);
    
    // State zigzag_state = custom_zigzag(in);
    // if (zigzag_state.game_score < best_state.game_score) best_state = zigzag_state;

    // State greedy_state = greedy(in, time_keeper);
    // if (greedy_state.game_score < best_state.game_score) best_state = greedy_state;

    constexpr int BEAM_WIDTH = 100;
    State beam_state = beam_search(in, param, hash, time_keeper, BEAM_WIDTH);
    if (beam_state.game_score < best_state.game_score) best_state = beam_state;
    
    best_state.print_actions();

    return 0;
}