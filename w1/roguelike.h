#pragma once

#include <entt/entt.hpp>

void progress_roguelike_systems(entt::registry &registry);
void init_roguelike(entt::registry &registry);
void process_turn(entt::registry &registry);
void print_stats(entt::registry &registry);
