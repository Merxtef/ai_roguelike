#include "roguelike.h"
#include "ecsTypes.h"
#include "raylib.h"
#include "stateMachine.h"
#include "aiLibrary.h"

template<typename... Components>
void copyComponents(entt::registry &registry, entt::entity source, entt::entity destination) {
    // Using fold expression to iterate over each component type
    ([&]() {
        // Check if the source entity has the component
        if (registry.any_of<Components>(source)) {
            // Get the component from the source entity
            const auto &component = registry.get<Components>(source);
            // Assign the component to the destination entity
            registry.emplace_or_replace<Components>(destination, component);
        }
    }(), ...); // The fold expression expands the lambda for each type in Components
}

static void add_dummy_statemachine(entt::registry& registry, entt::entity entity)
{
  auto& sm = registry.emplace_or_replace<StateMachine>(entity);
  sm.addState(create_move_to_enemy_state());
}

static void add_patrol_attack_flee_sm(entt::registry& registry, entt::entity entity)
{
    auto& sm = registry.get<StateMachine>(entity);
    int patrol = sm.addState(create_patrol_state(3.f));
    int moveToEnemy = sm.addState(create_move_to_enemy_state());
    int fleeFromEnemy = sm.addState(create_flee_from_enemy_state());

    sm.addTransition(create_enemy_available_transition(3.f), patrol, moveToEnemy);
    sm.addTransition(create_negate_transition(create_enemy_available_transition(5.f)), moveToEnemy, patrol);

    sm.addTransition(create_and_transition(create_hitpoints_less_than_transition(60.f), create_enemy_available_transition(5.f)),
                     moveToEnemy, fleeFromEnemy);
    sm.addTransition(create_and_transition(create_hitpoints_less_than_transition(60.f), create_enemy_available_transition(3.f)),
                     patrol, fleeFromEnemy);

    sm.addTransition(create_negate_transition(create_enemy_available_transition(7.f)), fleeFromEnemy, patrol);
}

static void add_patrol_flee_sm(entt::registry& registry, entt::entity entity)
{
    auto& sm = registry.get<StateMachine>(entity);
    int patrol = sm.addState(create_patrol_state(3.f));
    int fleeFromEnemy = sm.addState(create_flee_from_enemy_state());

    sm.addTransition(create_enemy_available_transition(3.f), patrol, fleeFromEnemy);
    sm.addTransition(create_negate_transition(create_enemy_available_transition(5.f)), fleeFromEnemy, patrol);
}

static void add_attack_sm(entt::registry& registry, entt::entity entity)
{
    auto& sm = registry.get<StateMachine>(entity);
    sm.addState(create_move_to_enemy_state());
}

static void add_heal_attack_sm(entt::registry& registry, entt::entity entity)
{
    auto& sm = registry.get<StateMachine>(entity);

    int followAndHeal = sm.addState(create_follow_and_heal_player_state());
    int attackEnemy = sm.addState(create_attack_enemy_state());

    sm.addTransition(create_enemy_available_transition(1.01f), followAndHeal, attackEnemy);
    sm.addTransition(create_player_far_transition(1.5f), attackEnemy, followAndHeal);
}

static void add_flee_and_arrow_attack_sm(entt::registry& registry, entt::entity entity)
{
    auto& sm = registry.get<StateMachine>(entity);
    int fleeFromEnemy = sm.addState(create_flee_from_enemy_state());
    int arrowAttack = sm.addState(create_arrow_attack_state());

    sm.addTransition(create_negate_transition(create_enemy_available_transition(7.f)), fleeFromEnemy, arrowAttack);
    sm.addTransition(create_enemy_available_transition(5.f), arrowAttack, fleeFromEnemy);
}

static entt::entity create_monster(entt::registry& registry, int x, int y, Color color)
{
    auto entity = registry.create();
    registry.emplace<Position>(entity, x, y);
    registry.emplace<MovePos>(entity, x, y);
    registry.emplace<PatrolPos>(entity, x, y);
    registry.emplace<Hitpoints>(entity, 100.f);
    registry.emplace<Action>(entity, EA_NOP);
    registry.emplace<Color>(entity, color);
    registry.emplace<StateMachine>(entity);
    registry.emplace<Team>(entity, 1);
    registry.emplace<NumActions>(entity, 1, 0);
    registry.emplace<MeleeDamage>(entity, 20.f);
    return entity;
}

static entt::entity create_slime(entt::registry& registry, int x, int y, Color color)
{
    auto entity = registry.create();
    registry.emplace<Position>(entity, x, y);
    registry.emplace<MovePos>(entity, x, y);
    registry.emplace<Hitpoints>(entity, 200.f);
    registry.emplace<Action>(entity, EA_NOP);
    registry.emplace<Color>(entity, color);
    registry.emplace<StateMachine>(entity);
    registry.emplace<Team>(entity, 1);
    registry.emplace<NumActions>(entity, 1, 0);
    registry.emplace<MeleeDamage>(entity, 20.f);
    registry.emplace<SpawnSingleCopyOnHPThreshold>(entity, 100.f);
    return entity;
}

static entt::entity create_archer(entt::registry& registry, int x, int y, Color color)
{
    auto entity = registry.create();
    registry.emplace<Position>(entity, x, y);
    registry.emplace<MovePos>(entity, x, y);
    registry.emplace<Hitpoints>(entity, 50.f);
    registry.emplace<Action>(entity, EA_NOP);
    registry.emplace<Color>(entity, color);
    registry.emplace<StateMachine>(entity);
    registry.emplace<Team>(entity, 1);
    registry.emplace<NumActions>(entity, 1, 0);
    registry.emplace<MeleeDamage>(entity, 20.f);
    return entity;
}

static void create_player(entt::registry& registry, int x, int y)
{
    auto entity = registry.create();
    registry.emplace<Position>(entity, x, y);
    registry.emplace<MovePos>(entity, x, y);
    registry.emplace<Hitpoints>(entity, 100.f);
    registry.emplace<Color>(entity, GetColor(0xeeeeeeff));
    registry.emplace<Action>(entity, EA_NOP);
    registry.emplace<IsPlayer>(entity);
    registry.emplace<Team>(entity, 0);
    registry.emplace<PlayerInput>(entity);
    registry.emplace<NumActions>(entity, 2, 0);
    registry.emplace<MeleeDamage>(entity, 50.f);
}

static entt::entity create_healer(entt::registry& registry, int x, int y)
{
    auto entity = registry.create();
    registry.emplace<Position>(entity, x, y);
    registry.emplace<MovePos>(entity, x, y);
    registry.emplace<Color>(entity, GetColor(0x000011ff));
    registry.emplace<StateMachine>(entity);
    registry.emplace<Action>(entity, EA_NOP);
    registry.emplace<Team>(entity, 0);
    registry.emplace<MeleeDamage>(entity, 50.f);
    registry.emplace<Healing>(entity);
    return entity;
}

static void create_heal(entt::registry& registry, int x, int y, float amount)
{
    auto entity = registry.create();
    registry.emplace<Position>(entity, x, y);
    registry.emplace<HealAmount>(entity, amount);
    registry.emplace<Color>(entity, GetColor(0x44ff44ff));
}

static void create_powerup(entt::registry& registry, int x, int y, float amount)
{
    auto entity = registry.create();
    registry.emplace<Position>(entity, x, y);
    registry.emplace<PowerupAmount>(entity, amount);
    registry.emplace<Color>(entity, Color{255, 255, 0, 255});
}

void progress_roguelike_systems(entt::registry &registry)
{
  static auto playersView = registry.view<PlayerInput, Action, const IsPlayer>();
  for (auto &&[entity, inp, a]: playersView.each())
  {
    bool left = IsKeyDown(KEY_LEFT);
    bool right = IsKeyDown(KEY_RIGHT);
    bool up = IsKeyDown(KEY_UP);
    bool down = IsKeyDown(KEY_DOWN);

    if (left && !inp.left) a.action = EA_MOVE_LEFT;
    if (right && !inp.right) a.action = EA_MOVE_RIGHT;
    if (up && !inp.up) a.action = EA_MOVE_UP;
    if (down && !inp.down) a.action = EA_MOVE_DOWN;

    inp.left = left;
    inp.right = right;
    inp.up = up;
    inp.down = down;
  }
  
  static auto renderablesView = registry.view<const Position, const Color>();
  for (auto &&[entity, pos, color]: renderablesView.each())
  {
    const Rectangle rect = {float(pos.x), float(pos.y), 1, 1};
    if(registry.all_of<TextureSource>(entity))
    {
      // const entt::entity& targetEntity = registry.get<TextureSource>(entity);
      // const Texture2D& texture = registry.get<Texture2D>(targetEntity);
      // DrawTextureQuad(texture, Vector2{1, 1}, Vector2{0, 0}, rect, color);
    } else {
      DrawRectangleRec(rect, color);
    }
  }
}


void init_roguelike(entt::registry &registry)
{
  add_patrol_attack_flee_sm(registry, create_monster(registry, 5, 5, GetColor(0xee00eeff)));
  add_patrol_attack_flee_sm(registry, create_monster(registry, 10, -5, GetColor(0xee00eeff)));
  add_patrol_flee_sm(registry, create_monster(registry, -5, -5, GetColor(0x111111ff)));
  add_attack_sm(registry, create_monster(registry, -5, 5, GetColor(0x880000ff)));
  add_flee_and_arrow_attack_sm(registry, create_archer(registry, 2, 2, GetColor(0xcccc00ff)));
  add_attack_sm(registry, create_slime(registry, 12, 12, GetColor(0x008800ff)));

  create_player(registry, 0, 0);
  add_heal_attack_sm(registry, create_healer(registry, 0, 1));

  create_powerup(registry, 7, 7, 10.f);
  create_powerup(registry, 10, -6, 10.f);
  create_powerup(registry, 10, -4, 10.f);

  create_heal(registry, -5, -5, 50.f);
  create_heal(registry, -5, 5, 50.f);
}

static bool is_player_acted(entt::registry &registry)
{
  static auto playerActionsView = registry.view<const Action, const IsPlayer>();
  bool playerActed = false;
  for (auto &&[entity, a]: playerActionsView.each())
  {
    playerActed = a.action != EA_NOP;
  }
  return playerActed;
}

static bool upd_player_actions_count(entt::registry &registry)
{
  static auto playerNumActionsView = registry.view<NumActions, const IsPlayer>();
  bool actionsReached = false;
  for (auto &&[entity, na]: playerNumActionsView.each())
  {
    na.curActions = (na.curActions + 1) % na.numActions;
    actionsReached |= na.curActions == 0;
  }
  return actionsReached;
}

static Position move_pos(Position pos, int action)
{
  if (action == EA_MOVE_LEFT)
    pos.x--;
  else if (action == EA_MOVE_RIGHT)
    pos.x++;
  else if (action == EA_MOVE_UP)
    pos.y--;
  else if (action == EA_MOVE_DOWN)
    pos.y++;
  return pos;
}

static void process_actions(entt::registry &registry)
{
  static auto actorsView = registry.view<Action, Position, MovePos, const MeleeDamage, const Team>();
  static auto movedActorsView = registry.view<const MovePos, Hitpoints, const Team>();
  for (auto &&[entity, a, pos, mpos, dmg, team]: actorsView.each())
  {
    Position nextPos = move_pos(pos, a.action);
    bool blocked = false;
    for (auto &&[enemy, epos, hp, enemy_team]: movedActorsView.each())
    {
      if (entity != enemy && epos == nextPos)
      {
        blocked = true;
        if (team.team != enemy_team.team)
        {
          hp.hitpoints -= dmg.damage;
          if (hp.hitpoints <= 0.f)
            registry.emplace<Dying>(enemy);
        }
      }
    }

    if (blocked)
      a.action = EA_NOP;
    else
      mpos = nextPos;
  }
  
  static auto movingUnitsView = registry.view<Action, Position, MovePos>();
  for (auto &&[entity, a, pos, mpos]: movingUnitsView.each())
  {
    pos = mpos;
    a.action = EA_NOP;
  }

  static auto playersView = registry.view<const Position, Hitpoints, MeleeDamage, const IsPlayer>();
  static auto healsView = registry.view<const Position, const HealAmount>();
  static auto powerUpsView = registry.view<const Position, const PowerupAmount>();
  for (auto &&[entity, pos, hp, dmg]: playersView.each())
  {
    for (auto &&[heal_entity, hpos, amt]: healsView.each())
    {
      if (pos == hpos)
      {
        hp.hitpoints += amt.amount;
        registry.emplace<Dying>(heal_entity);
      }
    }

    for (auto &&[powerup_entity, ppos, amt]: powerUpsView.each())
    {
      if (pos == ppos)
      {
        dmg.damage += amt.amount;
        registry.emplace<Dying>(powerup_entity);
      }
    }
  }

  static auto dyingsView = registry.view<Dying>();
  registry.destroy(dyingsView.begin(), dyingsView.end());

  // It's only reasonable if instant kill whould bypass that copy spawn
  static auto copySpawnersView = registry.view<Hitpoints, SpawnSingleCopyOnHPThreshold>();
  for (auto &&[entity, hp, spawnCopyData]: copySpawnersView.each())
  {
    if (spawnCopyData.isUnused && hp.hitpoints < spawnCopyData.threshold)
    {
      spawnCopyData.isUnused = false;

      entt::entity spawnedEntity = registry.create();
      copyComponents<
        Position, MovePos, PatrolPos,
        Hitpoints, Action, Color,
        Team, NumActions, MeleeDamage
      >(registry, entity, spawnedEntity);
      
      if (registry.any_of<StateMachine>(entity))
      {
        add_dummy_statemachine(registry, spawnedEntity);
      }

      if (registry.all_of<Position, MovePos>(spawnedEntity))
      {
        Position &pos = registry.get<Position>(spawnedEntity);
        MovePos &mpos = registry.get<MovePos>(spawnedEntity);
        pos.x += 1;
        pos.y += 1;
        mpos.x = pos.x;
        mpos.y = pos.y;
      }
    }
  }
}

void process_turn(entt::registry &registry)
{
  if (is_player_acted(registry) && upd_player_actions_count(registry))
  {
    for (auto &&[entity, sm]: registry.view<StateMachine>().each())
    {
      sm.act(0.f, registry, entity);
    }
    process_actions(registry);
  }
}

void print_stats(entt::registry &registry)
{
  static auto playerStatsView = registry.view<const Hitpoints, const MeleeDamage, const IsPlayer>();

  for (auto &&[_, hp, dmg]: playerStatsView.each())
  {
    DrawText(TextFormat("hp: %d", int(hp.hitpoints)), 20, 20, 20, WHITE);
    DrawText(TextFormat("power: %d", int(dmg.damage)), 20, 40, 20, WHITE);
  }
}

