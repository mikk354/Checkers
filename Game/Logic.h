#pragma once
#include <random>
#include <vector>

#include "../Models/Move.h"
#include "Board.h"
#include "Config.h"

const int INF = 1e9;

class Logic
{
public:
    Logic(Board* board, Config* config) : board(board), config(config)
    {
        rand_eng = std::default_random_engine(
            !((*config)("Bot", "NoRandom")) ? unsigned(time(0)) : 0);
        scoring_mode = (*config)("Bot", "BotScoringType");
        optimization = (*config)("Bot", "Optimization");
    }

    vector<move_pos> find_best_turns(const bool color)
    {
        next_best_state.clear();  // очистка лучших ходов
        next_move.clear();        // очистка ходов

        find_first_best_turn(board->get_board(), color, -1, -1, 0);  // функция поиска лучшего хода с запросом игрового поля

        int cur_state = 0;                                           // текущий ход
        vector<move_pos> res;    // вектор для хранения результата
        do
        {
            res.push_back(next_move[cur_state]);
            cur_state = next_best_state[cur_state];                  // проход по лучшим ходам, пока это можно делать, пока не станет -1
        } while (cur_state != -1 && next_move[cur_state].x != -1);
        return res;              // возвращается список ходов
    }

private:
    vector<vector<POS_T>> make_turn(vector<vector<POS_T>> mtx, move_pos turn) const
    {
        if (turn.xb != -1)
            mtx[turn.xb][turn.yb] = 0;
        if ((mtx[turn.x][turn.y] == 1 && turn.x2 == 0) || (mtx[turn.x][turn.y] == 2 && turn.x2 == 7))
            mtx[turn.x][turn.y] += 2;
        mtx[turn.x2][turn.y2] = mtx[turn.x][turn.y];
        mtx[turn.x][turn.y] = 0;
        return mtx;
    }

    double calc_score(const vector<vector<POS_T>>& mtx, const bool first_bot_color) const
    {
        // color - who is max player
        double w = 0, wq = 0, b = 0, bq = 0;
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                w += (mtx[i][j] == 1);
                wq += (mtx[i][j] == 3);
                b += (mtx[i][j] == 2);
                bq += (mtx[i][j] == 4);
                if (scoring_mode == "NumberAndPotential")
                {
                    w += 0.05 * (mtx[i][j] == 1) * (7 - i);
                    b += 0.05 * (mtx[i][j] == 2) * (i);
                }
            }
        }
        if (!first_bot_color)
        {
            swap(b, w);
            swap(bq, wq);
        }
        if (w + wq == 0)
            return INF;
        if (b + bq == 0)
            return 0;
        int q_coef = 4;
        if (scoring_mode == "NumberAndPotential")
        {
            q_coef = 5;
        }
        return (b + bq * q_coef) / (w + wq * q_coef);
    }



    double find_first_best_turn(vector<vector<POS_T>> mtx, const bool color, const POS_T x, const POS_T y, size_t state, double alpha = -1) {   // состояния доски и игрока\ бота
        next_move.emplace_back(-1, -1, -1, -1);
        next_best_state.push_back(-1);              // начало хода
        if (state != 0)                             // просмотр всех шашек
            find_turns(x, y, mtx);
        auto now_turns = turns;                     // сохранение позиций шашек и тех шашек которые биты
        bool now_have_beats = have_beats;
        if (!now_have_beats && state != 0) {
            return find_best_turn_rec(mtx, 1 - color, 0, alpha);       // проверка если битых нет
        }
        double best_score = -1;
        for (auto turn : now_turns) {
            size_t new_state = next_move.size();   // запоминает какой это ход и переменная для счета
            double score;
            if (now_have_beats) {
                score = find_first_best_turn(make_turn(mtx, turn), color, turn.x2, turn.y2, new_state, best_score);  // если есть битые шашки
            }
            else {
                score = find_first_best_turn(make_turn(mtx, turn), 1 - color, 0, turn.y2, new_state, best_score);     // если нет, ход противника
            }
            if (score > best_score) {
                best_score = score;
                next_best_state[state] = (now_have_beats ? new_state : -1);   // обновление best score если текущий ход лучше
                next_move[state] = turn;
            }
        }
        return best_score;
    }




    double find_best_turn_rec(vector<vector<POS_T>> mtx, const bool color, const size_t depth, double alpha = -1,    // состояние доски
        double beta = INF + 1, const POS_T x = -1, const POS_T y = -1) {
        if (depth == Max_depth) {
            return calc_score(mtx, (depth % 2 == color));           // проверяет чей ход 
        }
        if (x != -1) {
            find_turns(x, y, mtx);
        }
        else {                                 // поиск ходов 
            find_turns(color, mtx);
        }
        auto turns_now = turns;                      // список ходов и битых шашек
        bool have_beats_now = have_beats;
        if (!have_beats_now && x != -1) {
            return find_best_turn_rec(mtx, 1 - color, depth + 1, alpha, beta);
        }                                             // передача хода противнику
        if (turns.empty()) {
            return (depth % 2 ? 0 : INF);
        }
        double min_score = INF + 1;
        double max_score = -1;                      // границы альфа бета отсечения
        for (auto turn : turns_now) {
            double score;
            if (have_beats_now) {
                score = find_best_turn_rec(make_turn(mtx, turn), color, depth, alpha, beta, turn.x2, turn.y2);
            }
            else {
                score = find_best_turn_rec(make_turn(mtx, turn), 1 - color, depth + 1, alpha, beta);
            }
            min_score = min(min_score, score);
            max_score = max(max_score, score);    // обновление минимального и максимального счета
            if (depth % 2)
                alpha = max(alpha, max_score);
            else
                beta = min(beta, min_score);        // отсечение ветки
            if (optimization != "O0" && alpha >= beta)
                return (depth % 2 ? max_score + 1 : min_score - 1);
        }
        return (depth % 2 ? max_score : min_score);   // возвращает лучший ход
    }


public:
    void find_turns(const bool color)
    {
        find_turns(color, board->get_board()); // запрос цвета игрока или бота
    }

    void find_turns(const POS_T x, const POS_T y)
    {
        find_turns(x, y, board->get_board());  // запрос координат
    }

private:
    void find_turns(const bool color, const vector<vector<POS_T>>& mtx)
    {
        vector<move_pos> res_turns;
        bool have_beats_before = false;
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                if (mtx[i][j] && mtx[i][j] % 2 != color)
                {
                    find_turns(i, j, mtx);
                    if (have_beats && !have_beats_before)       // результат хода, куда передвинется, будет ли "съедена" шашка
                    {
                        have_beats_before = true;
                        res_turns.clear();
                    }
                    if ((have_beats_before && have_beats) || !have_beats_before)
                    {
                        res_turns.insert(res_turns.end(), turns.begin(), turns.end());
                    }
                }
            }
        }
        turns = res_turns;
        shuffle(turns.begin(), turns.end(), rand_eng);
        have_beats = have_beats_before;
    }

    void find_turns(const POS_T x, const POS_T y, const vector<vector<POS_T>>& mtx)
    {
        turns.clear();                                                                   // начало хода
        have_beats = false;
        POS_T type = mtx[x][y];
        // check beats
        switch (type)
        {
        case 1:
        case 2:
            // check pieces
            for (POS_T i = x - 2; i <= x + 2; i += 4)
            {
                for (POS_T j = y - 2; j <= y + 2; j += 4)
                {
                    if (i < 0 || i > 7 || j < 0 || j > 7)
                        continue;
                    POS_T xb = (x + i) / 2, yb = (y + j) / 2;
                    if (mtx[i][j] || !mtx[xb][yb] || mtx[xb][yb] % 2 == type % 2)            // проверка координат шашек, съедена ли, передвинулась ли
                        continue;
                    turns.emplace_back(x, y, i, j, xb, yb);
                }
            }
            break;
        default:
            // check queens
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    POS_T xb = -1, yb = -1;
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])
                        {
                            if (mtx[i2][j2] % 2 == type % 2 || (mtx[i2][j2] % 2 != type % 2 && xb != -1))  // проверка координат дамок, съедена ли, передвинулась ли
                            {
                                break;
                            }
                            xb = i2;
                            yb = j2;
                        }
                        if (xb != -1 && xb != i2)
                        {
                            turns.emplace_back(x, y, i2, j2, xb, yb);
                        }
                    }
                }
            }
            break;
        }
        // check other turns
        if (!turns.empty())                                             // после хода
        {
            have_beats = true;
            return;
        }
        switch (type)
        {
        case 1:
        case 2:
            // check pieces
        {
            POS_T i = ((type % 2) ? x - 1 : x + 1);
            for (POS_T j = y - 1; j <= y + 1; j += 2)
            {
                if (i < 0 || i > 7 || j < 0 || j > 7 || mtx[i][j])   // проверка координат шашек, съедена ли, передвинулась ли
                    continue;
                turns.emplace_back(x, y, i, j);
            }
            break;
        }
        default:
            // check queens
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j) // проверка координат дамок, съедена ли, передвинулась ли
                    {
                        if (mtx[i2][j2])
                            break;
                        turns.emplace_back(x, y, i2, j2);
                    }
                }
            }
            break;
        }
    }

public:
    vector<move_pos> turns;
    bool have_beats;
    int Max_depth;

private:
    default_random_engine rand_eng;
    string scoring_mode;
    string optimization;
    vector<move_pos> next_move;
    vector<int> next_best_state;
    Board* board;
    Config* config;
};