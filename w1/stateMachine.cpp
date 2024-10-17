#include "stateMachine.h"

void State::setParent(int id)
{
  parentId = id;
}

int State::getParent()
{
  return parentId;
}

void State::addChildState(int id)
{
  childrenStates.push_back(id);
}

int State::getFirstChild()
{
  if (childrenStates.size() > 0)
    return childrenStates[0];
  
  return StateMachine::NULL_STATE_ID;
}

StateMachine::~StateMachine()
{
  for (State* state : states)
    delete state;
  states.clear();
  for (auto &transList : transitions)
    for (auto &transition : transList)
      delete transition.first;
  transitions.clear();
}

void StateMachine::act(float dt, entt::registry &registry, entt::entity entity)
{
  if (curStateIdx < states.size())
  {
    int idx = curStateIdx;
    bool foundTransition = false;
    // Search for transition in current state and its parents 
    while (idx != StateMachine::NULL_STATE_ID)
    {
      for (const std::pair<StateTransition*, int> &transition : transitions[idx])
        if (transition.first->isAvailable(registry, entity))
        {
          foundTransition = true;
          states[idx]->exit();
          curStateIdx = transition.second;
          break;
        }
      
      if (foundTransition)
        break;
      
      idx = states[idx]->getParent();
    }
    
    // Move to lowest child (even if nested)
    if (foundTransition)
    {
      while (true)
      {
        idx = states[idx]->getFirstChild();

        if (idx == StateMachine::NULL_STATE_ID)
          break;
        
        curStateIdx = idx;
      }
      
      states[curStateIdx]->enter();
    }

    states[curStateIdx]->act(dt, registry, entity);
  }
  else
    curStateIdx = 0;
}

int StateMachine::addState(State *st)
{
  int idx = states.size();
  states.push_back(st);
  transitions.push_back(std::vector<std::pair<StateTransition*, int>>());
  return idx;
}

int StateMachine::addState(State *st, int parent)
{
  int idx = states.size();
  states.push_back(st);
  transitions.push_back(std::vector<std::pair<StateTransition*, int>>());

  st->setParent(parent);
  states[parent]->addChildState(idx);

  return idx;
}

void StateMachine::addTransition(StateTransition *trans, int from, int to)
{
  transitions[from].push_back(std::make_pair(trans, to));
}

