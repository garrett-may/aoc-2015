#include <cstdio>
#include <cstdint>
#include <experimental/array>
#include <tuple>
#include <optional>
#include <algorithm>

struct Boss{
    std::int32_t hit_points;
    std::int32_t damage;
};

struct Player{
    std::int32_t hit_points;
    std::int32_t armour;
    std::int32_t mana;
};

struct Env{
    std::int32_t shield_timer;
    std::int32_t poison_timer;
    std::int32_t recharge_timer;
};

struct Battle{
    Player player;
    Boss boss;
    Env env;
    std::int32_t cost;
};

template<typename T>
constexpr auto can_afford(T const& t, std::int32_t cost) noexcept{
    return t.mana >= cost;
}

template<typename T>
constexpr auto is_dead(T const& t) noexcept{
    return t.hit_points <= 0;
}

// Spells

constexpr static std::int32_t magic_missile_cost = 53;

constexpr auto magic_missile(Battle& battle) noexcept{
    battle.player.mana -= magic_missile_cost;
    battle.boss.hit_points -= 4;
    return magic_missile_cost;
}

constexpr static std::int32_t drain_cost = 73;

constexpr auto drain(Battle& battle) noexcept{
    battle.player.mana -= drain_cost;
    battle.boss.hit_points -= 2;
    battle.player.hit_points += 2;
    return drain_cost;
}

constexpr static std::int32_t shield_cost = 113;

constexpr auto shield(Battle& battle) noexcept{
    battle.player.mana -= shield_cost;
    battle.env.shield_timer = 6;
    return shield_cost;
}

constexpr static std::int32_t poison_cost = 173;

constexpr auto poison(Battle& battle) noexcept{
    battle.player.mana -= poison_cost;
    battle.env.poison_timer = 6;
    return poison_cost;
}

constexpr static std::int32_t recharge_cost = 229;

constexpr auto recharge(Battle& battle) noexcept{
    battle.player.mana -= recharge_cost;
    battle.env.recharge_timer = 5;
    return recharge_cost;
}

// Effects

constexpr void turn_on_shield(Battle& battle) noexcept{
    battle.player.armour = 7;
    battle.env.shield_timer -= 1;
}

constexpr void turn_off_shield(Battle& battle) noexcept{
    battle.player.armour = 0;
}

constexpr void turn_on_poison(Battle& battle) noexcept{
    battle.boss.hit_points -= 3;
    battle.env.poison_timer -= 1;
}

constexpr void turn_on_recharge(Battle& battle) noexcept{
    battle.player.mana += 101;
    battle.env.recharge_timer -= 1;
}

// Boss strike

constexpr void strike(Battle& battle) noexcept{
    auto damage = std::max(battle.boss.damage - battle.player.armour, 1);
    battle.player.hit_points -= damage;
}

// Play game

constexpr void apply_effects(Battle& battle) noexcept{
    if(battle.env.shield_timer > 0){
        turn_on_shield(battle);
    } else{
        turn_off_shield(battle);
    }
    if(battle.env.poison_timer > 0){
        turn_on_poison(battle);
    }
    if(battle.env.recharge_timer > 0){
        turn_on_recharge(battle);
    }
}

constexpr static auto next_actions = std::experimental::make_array(
    std::make_tuple(magic_missile_cost, magic_missile),
    std::make_tuple(drain_cost, drain),
    std::make_tuple(shield_cost, shield),
    std::make_tuple(poison_cost, poison),
    std::make_tuple(recharge_cost, recharge)
);

constexpr auto play_impl(Battle const& battle) noexcept{
    return std::apply([&](auto const&... nexts){
        return std::experimental::make_array([&](auto const& next_action){
            auto const& [cost, action] = next_action;
            if(!is_dead(battle.player) && can_afford(battle.player, cost)){
                auto next_battle = battle;
                next_battle.cost += action(next_battle);
                return std::optional<Battle>{next_battle};
            } else{
                return std::optional<Battle>{std::nullopt};
            }
        }(nexts)...);
    }, next_actions);
}

template<bool is_hard_mode, auto depth>
constexpr auto play(Battle const& original) noexcept{
    auto queue = std::array<Battle, depth>{};
    queue[0] = original;
    std::size_t j = 1;
    for(std::size_t i = 0; i < queue.size(); ++i){
        auto battle = queue[i];
        if constexpr(is_hard_mode){
            battle.player.hit_points -= 1;
            if(is_dead(battle.player)){
                continue;
            }
        }
        apply_effects(battle);
        if(is_dead(battle.boss)){
            return std::make_tuple(false, 0);
        }
        for(auto maybe_next_battle : play_impl(battle)){
            if(!maybe_next_battle.has_value()){
                continue;
            }
            auto next_battle = maybe_next_battle.value();
            apply_effects(next_battle);
            if(is_dead(next_battle.boss)){
                return std::make_tuple(true, next_battle.cost);
            }
            strike(next_battle);
            if(is_dead(next_battle.player)){
                continue;
            }
            if(j < queue.size()){
                queue[j++] = next_battle;
            } else{
                return std::make_tuple(false, 0);
            }
        }
    }
    return std::make_tuple(false, 0);
}

constexpr static auto easy_mode_answer = play<false, 1048576>(Battle{
    Player{50, 0, 500},
    Boss{58, 9},
    Env{},
    0
});

constexpr static auto hard_mode_answer = play<true, 1048576>(Battle{
    Player{50, 0, 500},
    Boss{58, 9},
    Env{},
    0
});

int main(){
    std::printf("easy mode : player %s, cost %d\n", (std::get<0>(easy_mode_answer) ? "won" : "lost"), std::get<1>(easy_mode_answer));
    std::printf("hard mode : player %s, cost %d\n", (std::get<0>(hard_mode_answer) ? "won" : "lost"), std::get<1>(hard_mode_answer));
    return 0;
}
