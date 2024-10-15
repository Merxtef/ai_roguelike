#include "aiLibrary.h"
#include <entt/entt.hpp>
#include "ecsTypes.h"
#include "raylib.h"
#include <cfloat>
#include <cmath>

class AttackEnemyState : public State
{
public:
  void enter() const override {}
  void exit() const override {}
  void act(float/* dt*/, entt::registry &/*registry*/, entt::entity /*entity*/) const override {}
};

template<typename T>
T sqr(T a){ return a*a; }

template<typename T, typename U>
static float dist_sq(const T &lhs, const U &rhs) { return float(sqr(lhs.x - rhs.x) + sqr(lhs.y - rhs.y)); }

template<typename T, typename U>
static float dist(const T &lhs, const U &rhs) { return sqrtf(dist_sq(lhs, rhs)); }

template<typename T, typename U>
static int move_towards(const T &from, const U &to)
{
  int deltaX = to.x - from.x;
  int deltaY = to.y - from.y;
  if (abs(deltaX) > abs(deltaY))
    return deltaX > 0 ? EA_MOVE_RIGHT : EA_MOVE_LEFT;
  return deltaY < 0 ? EA_MOVE_UP : EA_MOVE_DOWN;
}

static int inverse_move(int move)
{
  return move == EA_MOVE_LEFT ? EA_MOVE_RIGHT :
         move == EA_MOVE_RIGHT ? EA_MOVE_LEFT :
         move == EA_MOVE_UP ? EA_MOVE_DOWN :
         move == EA_MOVE_DOWN ? EA_MOVE_UP : move;
}


template<typename Callable>
static void on_closest_enemy_pos(entt::registry &registry, entt::entity entity, Callable c)
{
    auto view = registry.view<const Position, const Team>();
    auto [pos, t, a] = registry.get<Position, Team, Action>(entity); // Get components for the entity

    entt::entity closestEnemy = entt::null;
    float closestDist = FLT_MAX;
    Position closestPos;

    // Iterate over all entities in the view
    view.each([&](entt::entity enemy, const Position &epos, const Team &et)
    {
        if (t.team == et.team)
            return; // Skip if on the same team

        float curDist = dist(epos, pos);
        if (curDist < closestDist)
        {
            closestDist = curDist;
            closestPos = epos;
            closestEnemy = enemy;
        }
    });

    // If a valid closest enemy was found, call the provided function
    if (registry.valid(closestEnemy))
    {
        c(a, pos, closestPos);
    }
}

class MoveToEnemyState : public State
{
public:
  void enter() const override {}
  void exit() const override {}
  void act(float/* dt*/, entt::registry &registry, entt::entity entity) const override
  {
    on_closest_enemy_pos(registry, entity, [&](Action &a, const Position &pos, const Position &enemy_pos)
    {
      a.action = move_towards(pos, enemy_pos);
    });
  }
};

class FleeFromEnemyState : public State
{
public:
  FleeFromEnemyState() {}
  void enter() const override {}
  void exit() const override {}
  void act(float/* dt*/, entt::registry &registry, entt::entity entity) const override
  {
    on_closest_enemy_pos(registry, entity, [&](Action &a, const Position &pos, const Position &enemy_pos)
    {
      a.action = inverse_move(move_towards(pos, enemy_pos));
    });
  }
};

class PatrolState : public State
{
  float patrolDist;
public:
  PatrolState(float dist) : patrolDist(dist) {}
  void enter() const override {}
  void exit() const override {}
  void act(float/* dt*/, entt::registry &registry, entt::entity entity) const override
  {
    auto &pos = registry.get<Position>(entity);
    auto &ppos = registry.get<PatrolPos>(entity);
    auto &a = registry.get<Action>(entity);

    if (dist(pos, ppos) > patrolDist)
    {
      a.action = move_towards(pos, ppos); // do a recovery walk
    }
    else
    {
      // do a random walk
      a.action = GetRandomValue(EA_MOVE_START, EA_MOVE_END - 1);
    }
  }
};

class NopState : public State
{
public:
  void enter() const override {}
  void exit() const override {}
  void act(float/* dt*/, entt::registry &registry, entt::entity entity) const override {}
};

class EnemyAvailableTransition : public StateTransition
{
  float triggerDist;
public:
  EnemyAvailableTransition(float in_dist) : triggerDist(in_dist) {}
  bool isAvailable(entt::registry &registry, entt::entity entity) const override
  {
    auto &pos = registry.get<Position>(entity);
    auto &t = registry.get<Team>(entity);

    static auto enemiesView = registry.view<const Position, const Team>();
    bool enemiesFound = false;

    enemiesView.each([&](entt::entity enemy, const Position &epos, const Team &et)
    {
      if (t.team == et.team)
        return;
      float curDist = dist(epos, pos);
      enemiesFound |= curDist <= triggerDist;
    });

    return enemiesFound;
  }
};

class HitpointsLessThanTransition : public StateTransition
{
  float threshold;
public:
  HitpointsLessThanTransition(float in_thres) : threshold(in_thres) {}
  bool isAvailable(entt::registry &registry, entt::entity entity) const override
  {
    const auto &hp = registry.get<Hitpoints>(entity);
    return hp.hitpoints < threshold;
  }
};

class EnemyReachableTransition : public StateTransition
{
public:
  bool isAvailable(entt::registry &registry, entt::entity entity) const override
  {
    return false;
  }
};

class NegateTransition : public StateTransition
{
  const StateTransition *transition; // we own it
public:
  NegateTransition(const StateTransition *in_trans) : transition(in_trans) {}
  ~NegateTransition() override { delete transition; }

  bool isAvailable(entt::registry &registry, entt::entity entity) const override
  {
    return !transition->isAvailable(registry, entity);
  }
};

class AndTransition : public StateTransition
{
  const StateTransition *lhs; // we own it
  const StateTransition *rhs; // we own it
public:
  AndTransition(const StateTransition *in_lhs, const StateTransition *in_rhs) : lhs(in_lhs), rhs(in_rhs) {}
  ~AndTransition() override
  {
    delete lhs;
    delete rhs;
  }

  bool isAvailable(entt::registry &registry, entt::entity entity) const override
  {
    return lhs->isAvailable(registry, entity) && rhs->isAvailable(registry, entity);
  }
};


// states
State *create_attack_enemy_state()
{
  return new AttackEnemyState();
}
State *create_move_to_enemy_state()
{
  return new MoveToEnemyState();
}

State *create_flee_from_enemy_state()
{
  return new FleeFromEnemyState();
}


State *create_patrol_state(float patrol_dist)
{
  return new PatrolState(patrol_dist);
}

State *create_nop_state()
{
  return new NopState();
}

// transitions
StateTransition *create_enemy_available_transition(float dist)
{
  return new EnemyAvailableTransition(dist);
}

StateTransition *create_enemy_reachable_transition()
{
  return new EnemyReachableTransition();
}

StateTransition *create_hitpoints_less_than_transition(float thres)
{
  return new HitpointsLessThanTransition(thres);
}

StateTransition *create_negate_transition(StateTransition *in)
{
  return new NegateTransition(in);
}
StateTransition *create_and_transition(StateTransition *lhs, StateTransition *rhs)
{
  return new AndTransition(lhs, rhs);
}

