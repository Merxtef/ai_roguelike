#pragma once
#include <vector>
#include <entt/entt.hpp>

class State
{
public:
  virtual void enter() const = 0;
  virtual void exit() const = 0;
  virtual void act(float dt, entt::registry &registry, entt::registry::entity_type entity) const = 0;
};

class StateTransition
{
public:
  virtual ~StateTransition() {}
  virtual bool isAvailable(entt::registry &registry, entt::registry::entity_type entity) const = 0;
};

class StateMachine
{
  int curStateIdx = 0;
  std::vector<State*> states;
  std::vector<std::vector<std::pair<StateTransition*, int>>> transitions;
public:
  StateMachine() = default;
  StateMachine(const StateMachine &sm) = default;
  StateMachine(StateMachine &&sm) = default;

  ~StateMachine();

  StateMachine &operator=(const StateMachine &sm) = default;
  StateMachine &operator=(StateMachine &&sm) = default;


  void act(float dt, entt::registry &registry, entt::registry::entity_type entity);

  int addState(State *st);
  void addTransition(StateTransition *trans, int from, int to);
};

