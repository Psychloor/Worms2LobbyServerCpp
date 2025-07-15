//
// Created by blomq on 2025-07-10.
//

#include "database.hpp"

#include <print>
#include <ranges>

#include "game.hpp"
#include "room.hpp"
#include "user.hpp"

namespace worms_server
{
    std::shared_ptr<database> database::get_instance()
    {
        static auto instance = std::make_shared<database>();
        return instance;
    }

    uint32_t database::get_next_id()
    {
        static constexpr uint32_t k_start = 0x1000U;
        const auto instance = get_instance();
        if (!instance) throw std::runtime_error("database not initialized");

        if (uint32_t recycled_id; instance->recycled_ids_.try_dequeue(recycled_id))
            return recycled_id;

        uint32_t id = instance->next_id_.fetch_add(1, std::memory_order::relaxed);
        if (id == 0 || id < k_start)
        {
            id = k_start;
            instance->next_id_.store(id + 1, std::memory_order::relaxed);
        }

        if (id == 0) // wrap-around (rare but possible)
            throw std::overflow_error("ID pool exhausted");

        return id;
    }

    void database::recycle_id(const uint32_t id)
    {
        const auto instance = get_instance();
        if (instance == nullptr) return;

        // quick sanity guard in debug builds
        assert(id >= 0x1000U);

        instance->recycled_ids_.enqueue(id);
    }

    std::shared_ptr<user> database::get_user(const uint32_t id) const
    {
        const std::shared_lock lock(users_mutex_);
        const auto it = users_.find(id);
        return it != users_.end() ? it->second : nullptr;
    }

    std::shared_ptr<room> database::get_room(const uint32_t id) const
    {
        const std::shared_lock lock(rooms_mutex_);
        const auto it = rooms_.find(id);
        return it != rooms_.end() ? it->second : nullptr;
    }

    std::shared_ptr<game> database::get_game(const uint32_t id) const
    {
        const std::shared_lock lock(games_mutex_);
        const auto it = games_.find(id);
        return it != games_.end() ? it->second : nullptr;
    }

    std::vector<std::shared_ptr<user>> database::get_users() const
    {
        const std::shared_lock lock(users_mutex_);
        std::vector<std::shared_ptr<user>> users;

        users.reserve(users_.size());
        std::ranges::copy(users_ | std::views::values,
            std::back_inserter(users));

        return users;
    }

    std::vector<std::shared_ptr<room>> database::get_rooms() const
    {
        const std::shared_lock lock(rooms_mutex_);
        std::vector<std::shared_ptr<room>> rooms;

        rooms.reserve(rooms_.size());
        std::ranges::copy(rooms_ | std::views::values,
            std::back_inserter(rooms));
        return rooms;
    }

    std::vector<std::shared_ptr<game>> database::get_games() const
    {
        const std::shared_lock lock(games_mutex_);
        std::vector<std::shared_ptr<game>> games;

        games.reserve(games_.size());
        std::ranges::copy(games_ | std::views::values, std::back_inserter(games));
        return games;
    }

    std::vector<std::shared_ptr<user>> database::get_users_in_room(
        const uint32_t room_id) const
    {
        const std::shared_lock users_lock(users_mutex_);

        std::vector<std::shared_ptr<user>> users;

        users.reserve(rooms_.size());
        for (const auto& user : users_ | std::views::values)
        {
            if (user->get_room_id() == room_id)
            {
                users.push_back(user);
            }
        }

        return users;
    }

    std::shared_ptr<game> database::get_game_by_name(
        std::string_view name) const
    {
        const std::shared_lock lock(games_mutex_);

        auto view = games_ | std::views::values;
        const auto it = std::ranges::find_if(view,
            [name](const auto& game)
            {
                return game->get_name() ==
                    name;
            });

        return (it != view.end()) ? *it : nullptr;
    }

    void database::set_user_room_id(const uint32_t user_id,
        const uint32_t room_id)
    {
        const std::unique_lock lock(users_mutex_);
        const auto it = std::ranges::find_if(users_,
            [user_id](const auto& user) -> bool
            {
                return user.second->get_id() ==
                    user_id;
            });

        if (it != std::end(users_))
        {
            it->second->set_room_id(room_id);
            return;
        }

        spdlog::error("User {} not found", user_id);
    }

    void database::add_user(std::shared_ptr<user> user)
    {
        const std::scoped_lock lock(users_mutex_);
        const uint32_t id = user->get_id();
        users_[id] = std::move(user);
    }

    void database::remove_user(const uint32_t id)
    {
        const std::scoped_lock lock(users_mutex_);
        users_.erase(id);
        recycled_ids_.enqueue(id);
    }

    void database::add_room(std::shared_ptr<room> room)
    {
        const std::scoped_lock lock(rooms_mutex_);
        const uint32_t id = room->get_id();
        rooms_[id] = std::move(room);
    }

    void database::remove_room(const uint32_t id)
    {
        const std::scoped_lock lock(rooms_mutex_);
        rooms_.erase(id);
        recycled_ids_.enqueue(id);
    }

    void database::add_game(std::shared_ptr<game> game)
    {
        const std::scoped_lock lock(games_mutex_);
        const uint32_t id = game->get_id();
        games_[id] = std::move(game);
    }

    void database::remove_game(const uint32_t id)
    {
        const std::scoped_lock lock(games_mutex_);
        games_.erase(id);
        recycled_ids_.enqueue(id);
    }
}
