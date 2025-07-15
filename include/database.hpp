#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <memory>
#include <unordered_map>

#include <boost/asio/ip/address_v4.hpp>
#include <concurrent_queue.hpp>

namespace worms_server
{
    class user;
    class room;
    class game;

    class database : std::enable_shared_from_this<database>
    {
    public:
        static std::shared_ptr<database> get_instance();
        static uint32_t get_next_id();
        static void recycle_id(uint32_t id);

        [[nodiscard]] std::shared_ptr<user> get_user(uint32_t id) const;
        [[nodiscard]] std::shared_ptr<room> get_room(uint32_t id) const;
        [[nodiscard]] std::shared_ptr<game> get_game(uint32_t id) const;

        [[nodiscard]] std::vector<std::shared_ptr<user>> get_users() const;
        [[nodiscard]] std::vector<std::shared_ptr<room>> get_rooms() const;
        [[nodiscard]] std::vector<std::shared_ptr<game>> get_games() const;

        std::vector<std::shared_ptr<user>> get_users_in_room(
            uint32_t room_id) const;
        std::shared_ptr<game> get_game_by_name(std::string_view name) const;

        void set_user_room_id(uint32_t user_id, uint32_t room_id);

        void add_user(std::shared_ptr<user> user);
        void remove_user(uint32_t id);

        void add_room(std::shared_ptr<room> room);
        void remove_room(uint32_t id);

        void add_game(std::shared_ptr<game> game);
        void remove_game(uint32_t id);

    private:
        mutable std::shared_mutex users_mutex_;
        mutable std::shared_mutex rooms_mutex_;
        mutable std::shared_mutex games_mutex_;

        std::atomic_uint32_t next_id_ = 0x1000U;
        moodycamel::ConcurrentQueue<uint32_t> recycled_ids_;

        std::unordered_map<uint32_t, std::shared_ptr<user>> users_;
        std::unordered_map<uint32_t, std::shared_ptr<room>> rooms_;
        std::unordered_map<uint32_t, std::shared_ptr<game>> games_;
    };
}

#endif //DATABASE_HPP
