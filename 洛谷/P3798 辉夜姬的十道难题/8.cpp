// We don't care about the C++ standard

#ifndef TARGET

// #define TARGET 9

// Make Intellisense happy
#define TARGET 114514

#pragma error("TARGET not defined")

#endif

#include <algorithm>
#include <compare>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <utility>
#include <vector>

struct State {
  unsigned char data[3][3];

  auto operator<=>(const State &other) const = default;
};

class Vector {
private:
  unsigned char data[3];

public:
  unsigned short size = 0;

  unsigned char &operator[](std::size_t pos) { return data[pos]; }
  const unsigned char &operator[](std::size_t pos) const { return data[pos]; }

  unsigned char &back() { return data[size - 1]; }
  const unsigned char &back() const { return data[size - 1]; }

  void pushBack(unsigned char val) {
    data[size] = val;
    ++size;
  }
};

constexpr State mirrorHorizontally(const State &state) {
  State _state = state;
  std::swap(_state.data[0][0], _state.data[0][2]);
  std::swap(_state.data[1][0], _state.data[1][2]);
  std::swap(_state.data[2][0], _state.data[2][2]);
  return _state;
}

constexpr State mirrorVertically(const State &state) {
  State _state = state;
  std::swap(_state.data[0][0], _state.data[2][0]);
  std::swap(_state.data[0][1], _state.data[2][1]);
  std::swap(_state.data[0][2], _state.data[2][2]);
  return _state;
}

constexpr State getUniqueState(const State &state) {
  return std::min({state, mirrorHorizontally(state), mirrorVertically(state)});
}

constexpr unsigned char getMax(const State &state) {
  return std::max({state.data[0][0], state.data[0][1], state.data[0][2],
                   state.data[1][0], state.data[1][1], state.data[1][2],
                   state.data[2][0], state.data[2][1], state.data[2][2]});
}

// 放：随机
// 移：最大化

double solve0(State);         // 先手连放两步
double solve1(const State &); // 先手放
double solve2(const State &); // 先手移

double solve0(State state) {
  double answer = 0;
  for (unsigned short i = 0; i < 3; ++i) {
    for (unsigned short j = 0; j < 3; ++j) {
      state.data[i][j] = 1;
      answer += solve1(state) * 0.9;
      state.data[i][j] = 2;
      answer += solve1(state) * 0.1;
      state.data[i][j] = 0;
    }
  }
  return answer / 9;
}

double solve1(const State &_state) {
  static std::map<State, double> states;
  if (getMax(_state) >= TARGET)
    return 1;
  State state = getUniqueState(_state);
  const auto [iter, flag] = states.emplace(state, 0);
  if (flag) {
    unsigned short cnt = 0;
    for (unsigned short i = 0; i < 3; ++i) {
      for (unsigned short j = 0; j < 3; ++j) {
        if (state.data[i][j])
          continue;
        ++cnt;
        state.data[i][j] = 1;
        iter->second += solve2(state) * 0.9;
        state.data[i][j] = 2;
        iter->second += solve2(state) * 0.1;
        state.data[i][j] = 0;
      }
    }
    iter->second /= cnt;
  }
  return iter->second;
}

State moveUpwards(State state) {
  for (unsigned short i = 0; i < 3; ++i) {
    bool flag = false;
    // std::vector<unsigned char> v;
    Vector v;
    for (unsigned short j = 0; j < 3; ++j) {
      if (state.data[j][i]) {
        if (flag && state.data[j][i] == v.back()) {
          ++v.back();
          flag = false;
        } else {
          v.pushBack(state.data[j][i]);
          flag = true;
        }
        state.data[j][i] = 0;
      }
    }
    for (unsigned short j = 0; j < v.size; ++j)
      state.data[j][i] = v[j];
  }
  return state;
}

State moveDownwards(State state) {
  for (unsigned short i = 0; i < 3; ++i) {
    bool flag = false;
    // std::vector<unsigned char> v;
    Vector v;
    for (unsigned short j = 3; j--;) {
      if (state.data[j][i]) {
        if (flag && state.data[j][i] == v.back()) {
          ++v.back();
          flag = false;
        } else {
          v.pushBack(state.data[j][i]);
          flag = true;
        }
        state.data[j][i] = 0;
      }
    }
    for (unsigned short j = 2, k = 0; k < v.size; --j, ++k)
      state.data[j][i] = v[k];
  }
  return state;
}

State moveLeft(State state) {
  for (unsigned short i = 0; i < 3; ++i) {
    bool flag = false;
    // std::vector<unsigned char> v;
    Vector v;
    for (unsigned short j = 0; j < 3; ++j) {
      if (state.data[i][j]) {
        if (flag && state.data[i][j] == v.back()) {
          ++v.back();
          flag = false;
        } else {
          v.pushBack(state.data[i][j]);
          flag = true;
        }
        state.data[i][j] = 0;
      }
    }
    for (unsigned short j = 0; j < v.size; ++j)
      state.data[i][j] = v[j];
  }
  return state;
}

State moveRight(State state) {
  for (unsigned short i = 0; i < 3; ++i) {
    bool flag = false;
    // std::vector<unsigned char> v;
    Vector v;
    for (unsigned short j = 3; j--;) {
      if (state.data[i][j]) {
        if (flag && state.data[i][j] == v.back()) {
          ++v.back();
          flag = false;
        } else {
          v.pushBack(state.data[i][j]);
          flag = true;
        }
        state.data[i][j] = 0;
      }
    }
    for (unsigned short j = 2, k = 0; k < v.size; --j, ++k)
      state.data[i][j] = v[k];
  }
  return state;
}

double solve2(const State &_state) {
  static std::map<State, double> states;
  if (getMax(_state) >= TARGET)
    return 1;
  const State state = getUniqueState(_state);
  const auto [iter, flag] = states.emplace(state, 0);
  if (flag) {
    State p;
    p = moveUpwards(state);
    if (p != state)
      iter->second = std::max(iter->second, solve1(p));
    p = moveDownwards(state);
    if (p != state)
      iter->second = std::max(iter->second, solve1(p));
    p = moveLeft(state);
    if (p != state)
      iter->second = std::max(iter->second, solve1(p));
    p = moveRight(state);
    if (p != state)
      iter->second = std::max(iter->second, solve1(p));
  }
  return iter->second;
}

int main() {
  State state;
  for (unsigned short i = 0; i < 3; ++i) {
    for (unsigned short j = 0; j < 3; ++j)
      state.data[i][j] = 0;
  }
  std::cout << std::fixed << std::setprecision(10) << solve0(state) << '\n';
  return 0;
}