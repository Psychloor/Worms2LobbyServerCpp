//
// Created by blomq on 2025-07-10.
//

#include "database.hpp"

#include <ranges>
#include <print>

#include "game.hpp"
#include "room.hpp"
#include "user.hpp"

std::weak_ptr<worms_server::database> worms_server::database::get_instance()
{
	static auto inst = std::make_shared<database>();
	return inst;
}

uint32_t worms_server::database::get_next_id()
{
	const auto instance = get_instance().lock();
	if (instance == nullptr) return 0;

	const uint32_t next_id = instance->_next_id.fetch_add(1, std::memory_order::relaxed);

	return next_id;
}

std::weak_ptr<worms_server::user> worms_server::database::get_user(const uint32_t id) const
{
	std::shared_lock lock(_users_mutex);
	const auto it = _users.find(id);
	return it != _users.end() ? it->second : nullptr;
}

std::weak_ptr<worms_server::room> worms_server::database::get_room(const uint32_t id) const
{
	std::shared_lock lock(_rooms_mutex);
	const auto it = _rooms.find(id);
	return it != _rooms.end() ? it->second : nullptr;
}

std::weak_ptr<worms_server::game> worms_server::database::get_game(const uint32_t id) const
{
	std::shared_lock lock(_games_mutex);
	const auto it = _games.find(id);
	return it != _games.end() ? it->second : nullptr;
}

std::vector<std::weak_ptr<worms_server::user>> worms_server::database::get_users() const
{
	std::shared_lock lock(_users_mutex);
	std::vector<std::weak_ptr<user>> users;

	users.reserve(_users.size());
	std::ranges::copy(_users | std::views::values, std::back_inserter(users));

	return users;
}

std::vector<std::weak_ptr<worms_server::room>> worms_server::database::get_rooms() const
{
	std::shared_lock lock(_rooms_mutex);
	std::vector<std::weak_ptr<room>> rooms;

	rooms.reserve(_rooms.size());
	std::ranges::copy(_rooms | std::views::values, std::back_inserter(rooms));
	return rooms;
}

std::vector<std::weak_ptr<worms_server::game>> worms_server::database::get_games() const
{
	std::shared_lock lock(_games_mutex);
	std::vector<std::weak_ptr<game>> games;

	games.reserve(_games.size());
	std::ranges::copy(_games | std::views::values, std::back_inserter(games));
	return games;
}

std::vector<std::weak_ptr<worms_server::user>> worms_server::database::get_users_in_room(const uint32_t room_id) const
{
	std::shared_lock users_lock(_users_mutex);
	std::shared_lock rooms_lock(_rooms_mutex);

	std::vector<std::weak_ptr<user>> users;

	users.reserve(_rooms.size());
	for (const auto& user : _users | std::views::values)
	{
		if (user->get_room_id() == room_id)
		{
			users.push_back(user);
		}
	}

	return users;
}

void worms_server::database::set_user_room_id(const uint32_t user_id, const uint32_t room_id)
{
	std::unique_lock lock(_users_mutex);

	if (const auto it = _users.find(user_id); it != _users.end()) {
		it->second->set_room_id(room_id);
		return;
	}

	std::println("User {} not found", user_id);
}

void worms_server::database::add_user(std::shared_ptr<user>&& user)
{
	std::unique_lock lock(_users_mutex);
	const uint32_t id = user->get_id();
	_users[id] = std::move(user);
}

void worms_server::database::remove_user(const uint32_t id)
{
	std::unique_lock lock(_users_mutex);
	_users.erase(id);
}

void worms_server::database::add_room(std::shared_ptr<room>&& room)
{
	std::unique_lock lock(_rooms_mutex);
	const uint32_t id = room->get_id();
	_rooms[id] = std::move(room);
}

void worms_server::database::remove_room(const uint32_t id)
{
	std::unique_lock lock(_rooms_mutex);
	_rooms.erase(id);
}

void worms_server::database::add_game(std::shared_ptr<game>&& game)
{
	std::unique_lock lock(_games_mutex);
	const uint32_t id = game->get_id();
	_games[id] = std::move(game);
}

void worms_server::database::remove_game(const uint32_t id)
{
	std::unique_lock lock(_games_mutex);
	_games.erase(id);
}
