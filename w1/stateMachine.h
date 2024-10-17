#pragma once
#include <vector>
#include <entt/entt.hpp>

class State;

class StateTransition
{
public:
  virtual ~StateTransition() {}
  virtual bool isAvailable(entt::registry &registry, entt::entity entity) const = 0;
};

class StateMachine
{
  int curStateIdx = 0;
  std::vector<State*> states;
  std::vector<std::vector<std::pair<StateTransition*, int>>> transitions;
public:
  static const int NULL_STATE_ID = -1;

  StateMachine() = default;
  StateMachine(const StateMachine &sm) = default;
  StateMachine(StateMachine &&sm) = default;

  ~StateMachine();

  StateMachine &operator=(const StateMachine &sm) = default;
  StateMachine &operator=(StateMachine &&sm) = default;


  void act(float dt, entt::registry &registry, entt::entity entity);

  int addState(State *st);
  int addState(State *st, int parent);
  void addTransition(StateTransition *trans, int from, int to);
};

class State
{
  int parentId = StateMachine::NULL_STATE_ID;
  std::vector<int> childrenStates;
public:

  virtual void enter() const = 0;
  virtual void exit() const = 0;
  virtual void act(float dt, entt::registry &registry, entt::entity entity) const = 0;

  void setParent(int id);
  int getParent();
  void addChildState(int id);
  // First child is entrance point to lower hierarchy nodes
  // StateMachine::NULL_STATE_ID if no children
  int getFirstChild();
};
