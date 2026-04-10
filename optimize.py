import json
import math
import os
import subprocess
from typing import Any

import optuna


# TODO: Write parameter suggestions here
def generate_params(trial: optuna.trial.Trial) -> dict[str, str]:
    # for more information, see https://optuna.readthedocs.io/en/stable/reference/generated/optuna.trial.Trial.html
    params = {
        "STREAK": str(trial.suggest_float("streak", 1e2, 2e3)),
        "WRONG": str(trial.suggest_float("wrong", -2e3, -1e2)),
        "TURN": str(trial.suggest_float("turn", -2e2, 0.0)),
        "DIST": str(trial.suggest_float("dist", -1e2, 0.0)),
        "STREAK_M": str(trial.suggest_float("streak_m", 1e2, 2e3)),
        "WRONG_M": str(trial.suggest_float("wrong_m", -2e3, -1e2)),
        "TURN_M": str(trial.suggest_float("turn_m", -2e2, 0.0)),
        "DIST_M": str(trial.suggest_float("dist_m", -1e2, 0.0))
    }

    return params


# TODO: Customize the score extraction code here
def extract_score(result: dict[str, Any]) -> float:
    absolute_score = result["score"]  # noqa: F841
    log10_score = math.log10(absolute_score) if absolute_score > 0.0 else 0.0  # noqa: F841
    relative_score = result["relative_score"]  # noqa: F841

    score = absolute_score  # for absolute score problems
    # score = log10_score       # for relative score problems (alternative)
    # score = relative_score    # for relative score problems

    return score


# TODO: Set the direction to minimize or maximize
def get_direction() -> str:
    direction = "minimize"
    # direction = "maximize"
    return direction


# TODO: Set the timeout (seconds) or the number of trials
def run_optimization(study: optuna.study.Study) -> None:
    study.optimize(Objective(), timeout=900)
    # study.optimize(Objective(), n_trials=100)


class Objective:
    def __init__(self) -> None:
        pass

    def __call__(self, trial: optuna.trial.Trial) -> float:
        params = generate_params(trial)
        env = os.environ.copy()
        env.update(params)

        scores = []

        cmd = [
            "pahcer",
            "run",
            "--json",
            "--shuffle",
            "--no-result-file",
            "--freeze-best-scores",
        ]

        if trial.number != 0:
            cmd.append("--no-compile")

        process = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            env=env,
        )

        # see also: https://tech.preferred.jp/ja/blog/wilcoxonpruner/
        for line in process.stdout:
            result = json.loads(line)

            # If an error occurs, stop the process and raise an exception
            if result["error_message"] != "":
                process.send_signal(subprocess.signal.SIGINT)
                raise RuntimeError(result["error_message"])

            score = extract_score(result)
            seed = result["seed"]
            scores.append(score)
            trial.report(score, seed)

            if trial.should_prune():
                print(f"Trial {trial.number} pruned.")
                process.send_signal(subprocess.signal.SIGINT)

                objective_value = sum(scores) / len(scores)
                is_better_than_best = (
                    trial.study.direction == optuna.study.StudyDirection.MINIMIZE
                    and objective_value < trial.study.best_value
                ) or (
                    trial.study.direction == optuna.study.StudyDirection.MAXIMIZE
                    and objective_value > trial.study.best_value
                )

                if is_better_than_best:
                    # Avoid updating the best value
                    raise optuna.TrialPruned()
                else:
                    # It is recommended to return the value of the objective function at the current step
                    # instead of raising TrialPruned.
                    # This is a workaround to report the evaluation information of the pruned Trial to Optuna.
                    return sum(scores) / len(scores)

        return sum(scores) / len(scores)


study = optuna.create_study(
    direction=get_direction(),
    study_name="optuna-study",
    pruner=optuna.pruners.WilcoxonPruner(),
    sampler=optuna.samplers.TPESampler(),
)

run_optimization(study)

print(f"best params = {study.best_params}")
print(f"best score  = {study.best_value}")

importances = optuna.importance.get_param_importances(study)
print('parameter importances = {', end="")
for param, importance in importances.items():
    print(f"'{param}': {importance:.4f}", end=",")
print('}')