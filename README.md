# AHC063

[AtCoder Heuristic Contest 063](https://atcoder.jp/contests/ahc063) に参加した際のソースコードと考察メモをまとめたリポジトリです。

## 問題概要

N×N のグリッド上でヘビを操作し、好みの色列 `d[0], d[1], ..., d[M-1]` の順番で餌を食べることを目指します。

- ヘビは上下左右に 1 マス移動できます
- 餌があるマスに進むと餌を食べ、ヘビが 1 マス伸びます
- 自分の胴体に頭をぶつけると、そこから後ろが切り離されます（噛みちぎり）
- ターン数 `T` と誤った色の数 `E`、未回収の色の数で実スコアが決まります

## アルゴリズム

### ビームサーチ

時間制限（1950 ms）内でビームサーチを実行します。

- **評価関数** : 以下の 4 つの要素を線形結合した値を使用します
  - `streak` : ヘビの頭から好みの色が連続している数
  - `wrong` : 好みの色と異なる色の数
  - `turn` : 経過ターン数
  - `distance` : 次に必要な色までのマンハッタン距離（2 手先の Lookahead あり）
- **多様性の確保** : Zobrist Hash による同一盤面の除去
- **ビーム幅** : M < 100 のとき 300、M ≥ 100 のとき 100

### パラメータチューニング

`optimize.py` で [Optuna](https://optuna.org/) + [pahcer](https://github.com/terry-u16/pahcer) を用いて評価関数の重みを自動最適化しています。

最終パラメータ:

| パラメータ | 値 |
|---|---|
| `w_streak` | 628.3613 |
| `w_wrong` | -517.3913 |
| `w_turn` | -45.4974 |
| `w_dist` | -6.4570 |
| `w_streak_m` | 17486.3402 |
| `w_wrong_m` | -17147.2961 |
| `w_turn_m` | -38.6634 |
| `w_dist_m` | -57.7393 |

`w_*_m` はそれぞれの重みに `M` を掛けた項の係数で、`M` の大小による最適解の変化に対応しています。

## 実装の工夫

- `State` クラス内の配列を `int16_t` 型に変更してメモリ使用量を削減
- ビームサーチ内でアクション列を `vector<Node>` で木として管理し、`State` のコピーコストを削減
- Zobrist Hash の盤面部分を差分更新
- `unordered_map` の代わりに自作の `HashTable`（オープンアドレス法）を使用して実行時間を安定化
- `priority_queue` から `nth_element` ベースの実装に変更して高速化

## ファイル構成

```
.
├── main.cpp       # メインの C++ ソルバー
├── optimize.py    # Optuna によるパラメータ最適化スクリプト
├── memo.md        # 考察メモ
└── pyproject.toml # Python プロジェクト設定
```

## 環境構築

### C++ ソルバーのビルド

```bash
g++ -O2 -std=c++23 -o main main.cpp
```

### パラメータ最適化

[pahcer](https://github.com/terry-u16/pahcer) が必要です。

```bash
uv sync
uv run python optimize.py
```