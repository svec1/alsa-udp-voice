#ifndef NET_HPP
#define NET_HPP

#include <array>
#include <boost/asio.hpp>

using namespace boost;

template <unsigned int _buffer_size = 4096>
struct packet_native_t {
    using buffer_el_t = char;
    template <unsigned int size_buffer>
    using buffer_t = std::array<buffer_el_t, size_buffer>;

   public:
    static constexpr unsigned int buffer_size = _buffer_size;

   public:
    packet_native_t() = default;

    virtual buffer_t<buffer_size> to_bytes() const {
        buffer_t<buffer_size> arr;
        const buffer_el_t* bytes_p =
            reinterpret_cast<const buffer_el_t*>(this) + 8;

        std::memcpy(arr.data(), bytes_p, buffer_size);
        return arr;
    }
    virtual buffer_el_t* get_buffer() {
        return reinterpret_cast<buffer_el_t*>(this) + 8;
    }

   public:
    buffer_t<buffer_size> buffer;
};

class nstream;

template <typename T, typename F, F _prepare, F _was_accepted>
class packet : public std::enable_if_t<
                   std::is_base_of_v<packet_native_t<T::buffer_size>, T> &&
                       std::is_invocable_v<F, decltype(std::declval<T&>())>,
                   T> {
    friend class nstream;

   public:
    using packet_t = T;
    using cfunction_t = F;

   public:
    static constexpr unsigned int size = sizeof(T) - 8;

   public:
    packet() = default;
    packet(T&& pckg) : T(pckg) {}

   public:
    static void prepare(packet_t& pckt) { _prepare(pckt); }
    static void was_accepted(packet_t& pckt) { _was_accepted(pckt); }
};

template <typename T>
struct applied_native_protocol {
    static constexpr void prepare(T& pckt) {}
    static constexpr void was_accepted(T& pckt) {}

    using cfunction_t = decltype(prepare);
};

class nstream final {
   public:
    nstream(asio::io_context& io, asio::ip::address_v4 addr,
            asio::ip::port_type port)
        : sock(io), p(addr, port) {
        sock.open(p.protocol(), ec);
        sock.set_option(asio::socket_base::broadcast(true));
        if (ec.value())
            throw std::runtime_error(
                std::format("Failed to open udp socket({}).", ec.message()));
    }

   public:
    template <typename T, typename aprotocol = applied_native_protocol<T>>
    std::enable_if_t<std::is_same_v<
        T, packet<typename T::packet_t, typename aprotocol::cfunction_t,
                  aprotocol::prepare, aprotocol::was_accepted>>>
    send_to(T&& pckt) {
        T::prepare(pckt);
        sock.send_to(boost::asio::buffer(pckt.to_bytes()), p, 0, ec);
        if (ec.value())
            throw std::runtime_error(
                std::format("Failed to send packet({}).", ec.message()));
    }

    template <typename T, typename aprotocol = applied_native_protocol<T>>
    std::enable_if_t<std::is_same_v<
        T, packet<typename T::packet_t, typename aprotocol::cfunction_t,
                  aprotocol::prepare, aprotocol::was_accepted>>>
    receive_last(T& pckt) {
        unsigned int len = sock.receive_from(
            boost::asio::buffer(pckt.get_buffer(), T::size), p, 0, ec);
        if (ec.value())
            throw std::runtime_error(
                std::format("Failed to process packet({}).", ec.message()));
        else if (len != T::size)
            throw std::runtime_error("Undefined package.");

        T::was_accepted(pckt);
    }

   private:
    asio::ip::udp::socket sock;
    asio::ip::udp::endpoint p;

    boost::system::error_code ec;
};

#endif
