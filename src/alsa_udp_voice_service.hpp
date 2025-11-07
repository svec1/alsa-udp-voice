#ifndef ALSA_UDP_VOICE_SERVICE_HPP
#define ALSA_UDP_VOICE_SERVICE_HPP

#include <ainput.hpp>
#include <aoutput.hpp>
#include <net.hpp>
#include <print>
#include <thread>

using namespace boost;

template <
    typename T = packet_native_t<>,
    typename _aprotocol = applied_native_protocol<T>,
    typename = std::void_t<
        typename _aprotocol::cfunction_t,
        std::enable_if_t<std::is_invocable_v<decltype(_aprotocol::prepare),
                                             decltype(std::declval<T&>())> &&
                         std::is_invocable_v<decltype(_aprotocol::was_accepted),
                                             decltype(std::declval<T&>())>>>>
class alsa_udp_voice_service {
    using aprotocol = _aprotocol;
    using package_t = packet<T, typename aprotocol::cfunction_t,
                             aprotocol::prepare, aprotocol::was_accepted>;

   public:
    alsa_udp_voice_service(asio::ip::address_v4 host, asio::ip::port_type port)
        : net(io, host, port) {
        run();
    }

   private:
    void run() {
        std::thread sender(this->send_samples, std::ref(net), std::ref(in));
        std::thread receiver(this->receive_samples, std::ref(net),
                             std::ref(out));

        sender.join();
        receiver.join();
    }

   private:
    static void send_samples(nstream& net, input& in) {
        try {
            static package_t pckg;
            while (true) {
                for (unsigned int i = 0;
                     i < package_t::buffer_size / audio::buffer_size; ++i) {
                    const auto arr = in.get_samples();
                    std::copy(arr.begin(), arr.end(),
                              pckg.buffer.begin() + i * audio::buffer_size);
                }
                net.send_to<package_t, aprotocol>(std::move(pckg));
            }
        } catch (std::runtime_error& excp) {
            std::println("{}", excp.what());
            exit(1);
        }
    }
    static void receive_samples(nstream& net, output& out) {
        try {
            static package_t pckg;
            while (true) {
                net.receive_last<package_t, aprotocol>(pckg);
                out.play_samples(
                    std::span<typename package_t::buffer_el_t,
                              package_t::buffer_size>{pckg.buffer});
            }
        } catch (std::runtime_error& excp) {
            std::println("{}", excp.what());
            exit(1);
        }
    }

   public:
    input in;
    output out;

    asio::io_context io;
    nstream net;
};

#endif
