#ifndef NET_HPP
#define NET_HPP

#include <array>
#include <boost/asio.hpp>

using namespace boost::asio;

template <unsigned int _buffer_size = 4096>
struct package_native_t {
    using buffer_el_t = char;
    template <unsigned int size_buffer>
    using buffer_t = std::array<buffer_el_t, size_buffer>;

   public:
    static constexpr unsigned int buffer_size = _buffer_size;

   public:
    package_native_t() = default;

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

class socket;

template <typename T>
class package : public std::enable_if_t<
                    std::is_base_of_v<package_native_t<T::buffer_size>, T>, T> {
    friend class socket;

   public:
    using callback_func_t = std::function<void(T& package)>;
    using package_t = T;

   public:
    static constexpr unsigned int size = sizeof(T) - 8;

   public:
    package() = default;
    package(T&& pckg) : T(pckg) {}

    package(callback_func_t _c) : c(_c), expect(true) {}

   private:
    void operator()() {
        if (c) c(*dynamic_cast<T*>(this));
    }

   private:
    callback_func_t c;
    const bool expect = false;
};

class socket final {
   public:
    socket(io_context& io, ip::address_v4 addr, ip::port_type port)
        : sock(io), p(addr, port) {
        sock.open(p.protocol());
    }

   public:
    template <typename T>
    void send_to(package<T>&& pckg) {
        sock.send_to(boost::asio::buffer(pckg.to_bytes()), p);
    }

    template <typename T>
    void receive_last(package<T>& pckg) {
        if (!pckg.expect)
            throw std::runtime_error("The packet is being transmitted.");

        unsigned int len = sock.receive_from(
            boost::asio::buffer(pckg.get_buffer(), package<T>::size), p);

        if (len != package<T>::size)
            throw std::runtime_error("Undefined package.");

        pckg();
    }

   private:
    ip::udp::socket sock;
    ip::udp::endpoint p;
};

#endif
