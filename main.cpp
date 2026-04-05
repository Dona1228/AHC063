#include <bits/stdc++.h>
using namespace std;

struct Vec2 {
    int x, y;
    auto operator<=>(const Vec2 &other) const = default;
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
    vector<int> d;  // 好みの色列
    vector<vector<int>> f;  // 初期状態において各マスに置かれている餌の色
};

Input read_input() {
    ios_base::sync_with_stdio(false);
    cin.tie(nullptr);

    Input in;
    cin >> in.N >> in.M >> in.C;
    in.d.resize(in.M);
    for (int i = 0; i < in.M; i++) cin >> in.d[i];
    in.f.resize(in.N, vector<int>(in.N));
    for (int i = 0; i < in.N; i++) {
        for (int j = 0; j < in.N; j++) {
            cin >> in.f[i][j];
        }
    }
    return in;
}

bool is_in_grid(const Input& in, const Vec2& pos) {
    return 0 <= pos.x && pos.x < in.N && 0 <= pos.y && pos.y < in.N;
}

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

class State {
private:

public:
    int game_score;
    int evaluated_score;
    vector<vector<int>> board;
    vector<int> snake_color;
    vector<char> actions;
    deque<Vec2> snake_pos;

    State(const Input& in) {
        board = in.f;
        snake_pos.resize(5);
        for (int i = 0; i < 5; i++) snake_pos[i] = {5 - i - 1, 0};
        snake_color.resize(5);
        for (int i = 0; i < 5; i++) snake_color[i] = 1;

        calc_game_score(in);
    }

    bool can_move(const Input& in, const char dir) const {
        Vec2 head = snake_pos.front();
        switch (dir) {
            case ('U'):
                head.x--;
                break;
            case ('D'):
                head.x++;
                break;
            case ('L'):
                head.y--;
                break;
            case ('R'):
                head.y++;
                break;
        }
        if (!is_in_grid(in, head)) return false;
        if (head == snake_pos[1]) return false;
        return true;
    }

    void move(const Input& in, const char dir) {
        // 移動
        Vec2 head = snake_pos.front();
        switch (dir) {
            case ('U'):
                head.x--;
                break;
            case ('D'):
                head.x++;
                break;
            case ('L'):
                head.y--;
                break;
            case ('R'):
                head.y++;
                break;
        }
        snake_pos.emplace_front(head);

        // 食事
        if (board[head.x][head.y] == 0) {
            snake_pos.pop_back();
        } else {
            snake_color.emplace_back(board[head.x][head.y]);
            board[head.x][head.y] = 0;
        }

        // 噛みちぎり
        for (int i = 1; i < ssize(snake_pos); i++) {
            if (head == snake_pos[i]) {
                for (int j = i + 1; j < ssize(snake_pos); j++) {
                    board[snake_pos[j].x][snake_pos[j].y] = snake_color[j];
                }
                snake_color.erase(snake_color.begin() + i + 1, snake_color.end());
                snake_pos.erase(snake_pos.begin() + i + 1, snake_pos.end());
                break;
            }
        }

        // 状態の更新
        actions.emplace_back(dir);
        calc_game_score(in);
    }

    void calc_game_score(const Input& in) {
        int error = 0;
        for (int i = 0; i < ssize(snake_color); i++) {
            if (snake_color[i] != in.d[i]) error++;
        }
        game_score = ssize(actions) + 10000 * (error + 2 * (in.M - ssize(snake_color)));
    }

    void print_actions() {
        for (const auto a : actions) cout << a << "\n";
    }
};

State simple_zigzag(const Input& in) {
    State state(in);
    State best_state = state;

    for (int x = 4; x < in.N - 1; x++) {
        state.move(in, 'D');
        if (state.game_score < best_state.game_score) best_state = state;
    }

    for (int y = 1; y < in.N; y++) {
        state.move(in, 'R');
        if (state.game_score < best_state.game_score) best_state = state;
        if (y % 2 == 1) {
            for (int x = 0; x < in.N - 1; x++) state.move(in, 'U');
            if (state.game_score < best_state.game_score) best_state = state;
        } else {
            for (int x = 0; x < in.N - 1; x++) state.move(in, 'D');
            if (state.game_score < best_state.game_score) best_state = state;
        }
    }

    return best_state;
}

State bfs_all_route(const Input& in) {
    array<char, 4> dir{'U', 'D', 'L', 'R'};

    State best_state(in);

    queue<State> q;
    q.emplace(State(in));

    while (!q.empty()) {
        const State state = q.front();
        q.pop();

        if (state.game_score < best_state.game_score) best_state = state;

        for (const char d : dir) {
            if (!state.can_move(in, d)) continue;
            State next_state = state;
            next_state.move(in, d);
            q.emplace(next_state);
        }
    }

    return best_state;
}

State greedy(const Input& in, TimeKeeper& time_keeper) {
    const array<char, 4> dir{'U', 'D', 'L', 'R'};

    const double reject_prob = 0.5 * in.M / (3.0 * in.N * in.N / 4.0);

    State best_state(in);

    while (!time_keeper.is_time_over()) {
        State state(in);

        for (int t = 0; ssize(state.snake_color) < in.M; t++) {
            int target_color = in.d[ssize(state.snake_color)];
            if (xorshift() % 100 < reject_prob * 100) target_color = -1;
            
            const Vec2& start_pos = state.snake_pos.front();
            Vec2 target_pos = {-1, -1};

            vector<vector<int>> dist(in.N, vector<int>(in.N, -1));
            dist[start_pos.x][start_pos.y] = 0;

            queue<Vec2> q;
            q.emplace(start_pos);

            while (!q.empty()) {
                const Vec2 curr_pos = q.front();
                q.pop();

                if (state.board[curr_pos.x][curr_pos.y] == target_color) {
                    target_pos = curr_pos;
                    break;
                } else if (target_color == -1 && state.board[curr_pos.x][curr_pos.y] != 0) {
                    target_pos = curr_pos;
                    break;
                }
            
                for (const char d : dir) {
                    Vec2 next_pos = curr_pos;
                    switch(d) {
                        case ('U'):
                            next_pos.x--;
                            break;
                        case ('D'):
                            next_pos.x++;
                            break;
                        case ('L'):
                            next_pos.y--;
                            break;
                        case ('R'):
                            next_pos.y++;
                            break;
                    }

                    if (!is_in_grid(in, next_pos)) continue;
                    if (find(state.snake_pos.begin() + 1, state.snake_pos.end(), next_pos) != state.snake_pos.end()) continue;
                    if (dist[next_pos.x][next_pos.y] != -1) continue;
                    if (state.board[next_pos.x][next_pos.y] != 0 && state.board[next_pos.x][next_pos.y] != target_color && target_color != -1) continue;

                    q.emplace(next_pos);
                    dist[next_pos.x][next_pos.y] = dist[curr_pos.x][curr_pos.y] + 1;
                }
            }

            if (target_pos == Vec2{-1, -1}) break;

            vector<char> path;
            Vec2 curr_pos = target_pos;
            while (curr_pos != start_pos) {
                if (is_in_grid(in, {curr_pos.x - 1, curr_pos.y}) && dist[curr_pos.x - 1][curr_pos.y] == dist[curr_pos.x][curr_pos.y] - 1) {
                    curr_pos.x--;
                    path.emplace_back('D');
                } else if (is_in_grid(in, {curr_pos.x + 1, curr_pos.y}) && dist[curr_pos.x + 1][curr_pos.y] == dist[curr_pos.x][curr_pos.y] - 1) {
                    curr_pos.x++;
                    path.emplace_back('U');
                } else if (is_in_grid(in, {curr_pos.x, curr_pos.y - 1}) && dist[curr_pos.x][curr_pos.y - 1] == dist[curr_pos.x][curr_pos.y] - 1) {
                    curr_pos.y--;
                    path.emplace_back('R');
                } else if (is_in_grid(in, {curr_pos.x, curr_pos.y + 1}) && dist[curr_pos.x][curr_pos.y + 1] == dist[curr_pos.x][curr_pos.y] - 1) {
                    curr_pos.y++;
                    path.emplace_back('L');
                }
            }
            ranges::reverse(path);

            for (const char p : path) state.move(in, p);
        }

        if (ssize(state.snake_color) != in.M) continue;
        if (state.game_score < best_state.game_score) best_state = state;
    }

    return best_state;
}

State custom_zigzag(const Input& in) {
    State best_state(in);

    State base_state(in);
    for (int x = 4; x < in.N - 1; x++) base_state.move(in, 'D');

    if (in.N % 2 == 0) {
        for (int bit = 0; bit < (1 << in.N - 1); bit++) {
            State state = base_state;
            state.move(in, 'R');
            
            vector<bool> wall(in.N, false);
            for (int i = 0; i < in.N - 1; i++) {
                if (bit >> i & 1) wall[i + 1] = true;
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
                        if (y != 1) state.move(in, 'R');
                        if (ssize(state.snake_color) == in.M) break;
                        
                        if ((y - 1) % 2 == 0) {
                            for (int x = b; x > t; x--) {
                                state.move(in, 'U');
                                if (ssize(state.snake_color) == in.M) break;
                            }
                        } else {
                            for (int x = t; x < b; x++) {
                                state.move(in, 'D');
                                if (ssize(state.snake_color) == in.M) break;
                            }
                        }
                    }
                    curr_row = t - 1;
                    start_left = false;
                    if (curr_row >= 0 && ssize(state.snake_color) != in.M) state.move(in, 'U');
                } else {
                    for (int y = in.N - 1; y >= 1; y--) {
                        if (y != in.N - 1) state.move(in, 'L');
                        if (ssize(state.snake_color) == in.M) break;
                        
                        if ((in.N - 1 - y) % 2 == 0) {
                            for (int x = b; x > t; x--) {
                                state.move(in, 'U');
                                if (ssize(state.snake_color) == in.M) break;
                            }
                        } else {
                            for (int x = t; x < b; x++) {
                                state.move(in, 'D');
                                if (ssize(state.snake_color) == in.M) break;
                            }
                        }
                    }
                    curr_row = t - 1;
                    start_left = true;
                    if (curr_row >= 0 && ssize(state.snake_color) != in.M) state.move(in, 'U');
                }
            }

            if (ssize(state.snake_color) == in.M && state.game_score < best_state.game_score) best_state = state;
        }
    } else {
        for (int bit = 0; bit < (1 << (in.N - 2)); bit++) {
            State state = base_state;
            state.move(in, 'R');
            
            vector<bool> wall(in.N, false);
            for (int i = 0; i < in.N - 2; i++) {
                if (bit >> i & 1) wall[i + 1] = true;
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
                        if (x != in.N - 1) state.move(in, 'U');
                        if (ssize(state.snake_color) == in.M) break;
                        
                        if ((in.N - 1 - x) % 2 == 0) {
                            for (int y = l; y < r; y++) {
                                state.move(in, 'R');
                                if (ssize(state.snake_color) == in.M) break;
                            }
                        } else {
                            for (int y = r; y > l; y--) {
                                state.move(in, 'L');
                                if (ssize(state.snake_color) == in.M) break;
                            }
                        }
                    }
                    curr_col = r + 1;
                    start_bottom = false;
                    if (curr_col < in.N && ssize(state.snake_color) != in.M) state.move(in, 'R');
                } else {
                    for (int x = 0; x < in.N; x++) {
                        if (x != 0) state.move(in, 'D');
                        if (ssize(state.snake_color) == in.M) break;
                        
                        if (x % 2 == 0) {
                            for (int y = l; y < r; y++) {
                                state.move(in, 'R');
                                if (ssize(state.snake_color) == in.M) break;
                            }
                        } else {
                            for (int y = r; y > l; y--) {
                                state.move(in, 'L');
                                if (ssize(state.snake_color) == in.M) break;
                            }
                        }
                    }
                    curr_col = r + 1;
                    start_bottom = true;
                    if (curr_col < in.N && ssize(state.snake_color) != in.M) state.move(in, 'R');
                }
            }

            if (ssize(state.snake_color) == in.M && state.game_score < best_state.game_score) best_state = state;
        }
    }

    return best_state;
}

int main() {
    constexpr int TIME_LIMIT = 1950;
    TimeKeeper time_keeper(TIME_LIMIT);

    const Input in = read_input();

    State best_state(in);
    
    State zigzag_state = custom_zigzag(in);
    if (zigzag_state.game_score < best_state.game_score) best_state = zigzag_state;

    State greedy_state = greedy(in, time_keeper);
    if (greedy_state.game_score < best_state.game_score) best_state = greedy_state;
    
    best_state.print_actions();

    return 0;
}