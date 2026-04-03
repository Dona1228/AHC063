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
    vector<Vec2> snake_pos;
    vector<int> snake_color;
    vector<char> actions;

    State(const Input& in) {
        game_score = 0;
        evaluated_score = 0;
        board.resize(in.N, vector<int>(in.N));
        snake_pos.resize(5);
        for (int i = 0; i < 5; i++) snake_pos[i] = {5 - i - 1, 0};
        snake_color.resize(5);
        for (int i = 0; i < 5; i++) snake_color[i] = 1;
    }

    void calc_score(const Input& in) {
        int error = 0;
        for (int i = 0; i < ssize(snake_color); i++) {
            if (snake_color[i] != in.d[i]) error++;
        }
        game_score = ssize(actions) + 10000 * error + 2 * (in.M - ssize(snake_pos));
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

int main() {
    constexpr int TIME_LIMIT = 1950;
    TimeKeeper time_keeper(TIME_LIMIT);

    const Input in = read_input();
}