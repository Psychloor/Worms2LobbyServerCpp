#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <atomic>
#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <moodycamel/concurrentqueue.h>

#include <asio/ip/address_v4.hpp>

namespace worms_server
{
    class User;
    class Room;
    class Game;

    class Database : public std::enable_shared_from_this<Database>
    {
    public:
        [[nodiscard]] static std::shared_ptr<Database> getInstance();
        [[nodiscard]] static uint32_t getNextId();
        static void recycleId(uint32_t id);

        [[nodiscard]] std::shared_ptr<User> getUser(uint32_t id) const;
        [[nodiscard]] std::shared_ptr<Room> getRoom(uint32_t id) const;
        [[nodiscard]] std::shared_ptr<Game> getGame(uint32_t id) const;

        [[nodiscard]] std::vector<std::shared_ptr<User>> getUsers() const;
        [[nodiscard]] std::vector<std::shared_ptr<Room>> getRooms() const;
        [[nodiscard]] std::vector<std::shared_ptr<Game>> getGames() const;

        [[nodiscard]] std::vector<std::shared_ptr<User>> getUsersInRoom(uint32_t roomId) const;
        [[nodiscard]] std::shared_ptr<Game> getGameByName(std::string_view name) const;

        void setUserRoomId(uint32_t userId, uint32_t roomId);

        void addUser(std::shared_ptr<User> user);
        void removeUser(uint32_t id);

        void addRoom(std::shared_ptr<Room> room);
        void removeRoom(uint32_t id);

        void addGame(std::shared_ptr<Game> game);
        void removeGame(uint32_t id);

    private:
        mutable std::shared_mutex usersMutex_;
        mutable std::shared_mutex roomsMutex_;
        mutable std::shared_mutex gamesMutex_;

        std::atomic_uint32_t nextId_ = 0x1000U;
        moodycamel::ConcurrentQueue<uint32_t> recycledIds_;

        std::unordered_map<uint32_t, std::shared_ptr<User>> users_;
        std::unordered_map<uint32_t, std::shared_ptr<Room>> rooms_;
        std::unordered_map<uint32_t, std::shared_ptr<Game>> games_;
    };
} // namespace worms_server

#endif // DATABASE_HPP
