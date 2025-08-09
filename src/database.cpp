//
// Created by blomq on 2025-07-10.
//

#include "database.hpp"

#include <ranges>

#include "game.hpp"
#include "room.hpp"
#include "user.hpp"

namespace worms_server
{
    std::shared_ptr<Database> Database::getInstance()
    {
        static auto instance = std::make_shared<Database>();
        return instance;
    }

    uint32_t Database::getNextId()
    {
        static constexpr uint32_t K_START = 0x1000U;
        const auto instance = getInstance();
        if (!instance)
        {
            throw std::runtime_error("database not initialized");
        }

        if (uint32_t recycledId; instance->recycledIds_.try_dequeue(recycledId))
        {
            return recycledId;
        }

        uint32_t id = instance->nextId_.fetch_add(1, std::memory_order::relaxed);
        if (id == 0 || id < K_START)
        {
            id = K_START;
            instance->nextId_.store(id + 1, std::memory_order::relaxed);
        }

        // ReSharper disable once CppDFAConstantConditions
        if (id == 0)
        {
            // wrap-around (rare but possible)
            // ReSharper disable once CppDFAUnreachableCode
            spdlog::critical("ID pool exhausted");
            throw std::overflow_error("ID pool exhausted");
        }
        return id;
    }

    void Database::recycleId(const uint32_t id)
    {
        const auto instance = getInstance();
        if (instance == nullptr)
        {
            return;
        }

        // quick sanity guard in debug builds
        assert(id >= 0x1000U);

        instance->recycledIds_.enqueue(id);
    }

    std::shared_ptr<User> Database::getUser(const uint32_t id) const
    {
        const std::shared_lock lock(usersMutex_);
        const auto it = users_.find(id);
        return it != users_.end() ? it->second : nullptr;
    }

    std::shared_ptr<Room> Database::getRoom(const uint32_t id) const
    {
        const std::shared_lock lock(roomsMutex_);
        const auto it = rooms_.find(id);
        return it != rooms_.end() ? it->second : nullptr;
    }

    std::shared_ptr<Game> Database::getGame(const uint32_t id) const
    {
        const std::shared_lock lock(gamesMutex_);
        const auto it = games_.find(id);
        return it != games_.end() ? it->second : nullptr;
    }

    std::vector<std::shared_ptr<User>> Database::getUsers() const
    {
        const std::shared_lock lock(usersMutex_);
        std::vector<std::shared_ptr<User>> users;

        users.reserve(users_.size());
        std::ranges::copy(users_ | std::views::values, std::back_inserter(users));

        return users;
    }

    std::vector<std::shared_ptr<Room>> Database::getRooms() const
    {
        const std::shared_lock lock(roomsMutex_);
        std::vector<std::shared_ptr<Room>> rooms;

        rooms.reserve(rooms_.size());
        std::ranges::copy(rooms_ | std::views::values, std::back_inserter(rooms));
        return rooms;
    }

    std::vector<std::shared_ptr<Game>> Database::getGames() const
    {
        const std::shared_lock lock(gamesMutex_);
        std::vector<std::shared_ptr<Game>> games;

        games.reserve(games_.size());
        std::ranges::copy(games_ | std::views::values, std::back_inserter(games));
        return games;
    }

    std::vector<std::shared_ptr<User>> Database::getUsersInRoom(const uint32_t roomId) const
    {
        const std::shared_lock usersLock(usersMutex_);

        std::vector<std::shared_ptr<User>> users;

        users.reserve(rooms_.size());
        for (const auto& user : users_ | std::views::values)
        {
            if (user->getRoomId() == roomId)
            {
                users.emplace_back(user);
            }
        }

        return users;
    }

    std::shared_ptr<Game> Database::getGameByName(std::string_view name) const
    {
        const std::shared_lock lock(gamesMutex_);

        auto view = games_ | std::views::values;
        const auto it = std::ranges::find_if(view, [name](const auto& game) { return game->getName() == name; });

        return (it != view.end()) ? *it : nullptr;
    }

    void Database::setUserRoomId(const uint32_t userId, const uint32_t roomId)
    {
        const std::unique_lock lock(usersMutex_);
        const auto it = std::ranges::find_if(
            users_, [userId](const auto& user) -> bool { return user.second->getId() == userId; });

        if (it != std::end(users_))
        {
            it->second->setRoomId(roomId);
            return;
        }

        spdlog::error("User {} not found", userId);
    }

    void Database::addUser(std::shared_ptr<User> user)
    {
        const std::scoped_lock lock(usersMutex_);
        const uint32_t id = user->getId();
        users_[id] = std::move(user);
    }

    void Database::removeUser(const uint32_t id)
    {
        const std::scoped_lock lock(usersMutex_);
        users_.erase(id);
        recycledIds_.enqueue(id);
    }

    void Database::addRoom(std::shared_ptr<Room> room)
    {
        const std::scoped_lock lock(roomsMutex_);
        const uint32_t id = room->getId();
        rooms_[id] = std::move(room);
    }

    void Database::removeRoom(const uint32_t id)
    {
        const std::scoped_lock lock(roomsMutex_);
        rooms_.erase(id);
        recycledIds_.enqueue(id);
    }

    void Database::addGame(std::shared_ptr<Game> game)
    {
        const std::scoped_lock lock(gamesMutex_);
        const uint32_t id = game->getId();
        games_[id] = std::move(game);
    }

    void Database::removeGame(const uint32_t id)
    {
        const std::scoped_lock lock(gamesMutex_);
        games_.erase(id);
        recycledIds_.enqueue(id);
    }
} // namespace worms_server
