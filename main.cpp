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

        // 噛みちぎり（未実装）

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

State zigzag(const Input& in) {
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

int main() {
    constexpr int TIME_LIMIT = 1950;
    TimeKeeper time_keeper(TIME_LIMIT);

    const Input in = read_input();

    State best_state = zigzag(in);
    best_state.print_actions();

    return 0;
}