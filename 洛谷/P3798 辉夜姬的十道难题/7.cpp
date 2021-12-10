// We don't care about the C++ standard

#include <algorithm>
#include <compare>
#include <iostream>
#include <limits>
#include <map>
#include <utility>
#include <vector>

struct State {
  unsigned char data[3][3];

  auto operator<=>(const State &other) const = default;
};

constexpr unsigned long INF = std::numeric_limits<unsigned long>::max();

State mirrorHorizontally(const State &state) {
  State _state = state;
  std::swap(_state.data[0][0], _state.data[0][2]);
  std::swap(_state.data[1][0], _state.data[1][2]);
  std::swap(_state.data[2][0], _state.data[2][2]);
  return _state;
}

State mirrorVertically(const State &state) {
  State _state = state;
  std::swap(_state.data[0][0], _state.data[2][0]);
  std::swap(_state.data[0][1], _state.data[2][1]);
  std::swap(_state.data[0][2], _state.data[2][2]);
  return _state;
}

State getUniqueState(const State &state) {
  return std::min({state, mirrorHorizontally(state), mirrorVertically(state)});
}

// 放：最小化
// 移：最大化

unsigned long solve0(State);         // 先手连放两步
unsigned long solve1(const State &); // 先手放
unsigned long solve2(const State &); // 先手移

unsigned long solve0(State state) {
  unsigned long answer = INF;
  for (unsigned short i = 0; i < 3; ++i) {
    for (unsigned short j = 0; j < 3; ++j) {
      state.data[i][j] = 1;
      answer = std::min(answer, solve1(state));
      state.data[i][j] = 2;
      answer = std::min(answer, solve1(state));
      state.data[i][j] = 0;
    }
  }
  return answer;
}

unsigned long solve1(const State &_state) {
  static std::map<State, unsigned long> states;
  State state = getUniqueState(_state);
  const auto [iter, flag] = states.emplace(state, INF);
  if (flag) {
    for (unsigned short i = 0; i < 3; ++i) {
      for (unsigned short j = 0; j < 3; ++j) {
        if (state.data[i][j])
          continue;
        state.data[i][j] = 1;
        iter->second = std::min(iter->second, solve2(state));
        state.data[i][j] = 2;
        iter->second = std::min(iter->second, solve2(state));
        state.data[i][j] = 0;
      }
    }
  }
  return iter->second;
}

std::pair<unsigned long, State> moveUpwards(State state) {
  unsigned long s = 0;
  for (unsigned short i = 0; i < 3; ++i) {
    bool flag = false;
    std::vector<unsigned long> v;
    for (unsigned short j = 0; j < 3; ++j) {
      if (state.data[j][i]) {
        if (flag && state.data[j][i] == v.back()) {
          ++v.back();
          s += 1UL << v.back();
          flag = false;
        } else {
          v.push_back(state.data[j][i]);
          flag = true;
        }
        state.data[j][i] = 0;
      }
    }
    for (unsigned short j = 0; j < v.size(); ++j)
      state.data[j][i] = v[j];
  }
  return {s, std::move(state)};
}

std::pair<unsigned long, State> moveDownwards(State state) {
  unsigned long s = 0;
  for (unsigned short i = 0; i < 3; ++i) {
    bool flag = false;
    std::vector<unsigned long> v;
    for (unsigned short j = 3; j--;) {
      if (state.data[j][i]) {
        if (flag && state.data[j][i] == v.back()) {
          ++v.back();
          s += 1UL << v.back();
          flag = false;
        } else {
          v.push_back(state.data[j][i]);
          flag = true;
        }
        state.data[j][i] = 0;
      }
    }
    for (unsigned short j = 2, k = 0; k < v.size(); --j, ++k)
      state.data[j][i] = v[k];
  }
  return {s, std::move(state)};
}

std::pair<unsigned long, State> moveLeft(State state) {
  unsigned long s = 0;
  for (unsigned short i = 0; i < 3; ++i) {
    bool flag = false;
    std::vector<unsigned long> v;
    for (unsigned short j = 0; j < 3; ++j) {
      if (state.data[i][j]) {
        if (flag && state.data[i][j] == v.back()) {
          ++v.back();
          s += 1UL << v.back();
          flag = false;
        } else {
          v.push_back(state.data[i][j]);
          flag = true;
        }
        state.data[i][j] = 0;
      }
    }
    for (unsigned short j = 0; j < v.size(); ++j)
      state.data[i][j] = v[j];
  }
  return {s, std::move(state)};
}

std::pair<unsigned long, State> moveRight(State state) {
  unsigned long s = 0;
  for (unsigned short i = 0; i < 3; ++i) {
    bool flag = false;
    std::vector<unsigned long> v;
    for (unsigned short j = 3; j--;) {
      if (state.data[i][j]) {
        if (flag && state.data[i][j] == v.back()) {
          ++v.back();
          s += 1UL << v.back();
          flag = false;
        } else {
          v.push_back(state.data[i][j]);
          flag = true;
        }
        state.data[i][j] = 0;
      }
    }
    for (unsigned short j = 2, k = 0; k < v.size(); --j, ++k)
      state.data[i][j] = v[k];
  }
  return {s, std::move(state)};
}

unsigned long solve2(const State &_state) {
  static std::map<State, unsigned long> states;
  const State state = getUniqueState(_state);
  const auto [iter, flag] = states.emplace(state, 0);
  if (!flag)
    return iter->second;
  std::pair<unsigned long, State> p;
  p = moveUpwards(state);
  if (p.second != state)
    iter->second = std::max(iter->second, p.first + solve1(p.second));
  p = moveDownwards(state);
  if (p.second != state)
    iter->second = std::max(iter->second, p.first + solve1(p.second));
  p = moveLeft(state);
  if (p.second != state)
    iter->second = std::max(iter->second, p.first + solve1(p.second));
  p = moveRight(state);
  if (p.second != state)
    iter->second = std::max(iter->second, p.first + solve1(p.second));
  return iter->second;
}

int main() {
  State state;
  for (unsigned short i = 0; i < 3; ++i) {
    for (unsigned short j = 0; j < 3; ++j)
      state.data[i][j] = 0;
  }
  std::cout << solve0(state) << '\n';
  return 0;
}