#include <ainput.hpp>
#include <aoutput.hpp>
#include <net.hpp>
#include <thread>

template <typename T = package_native_t<>>
class voice_service_udp_user {
    using package_t = package<T>;

   public:
    voice_service_udp_user(std::span<char*> argv)
        : host(argv[2]), port(std::stoi(argv[3])) {
        run();
    }

   private:
    void run() {
        std::thread sender(this->send_samples, std::ref(io_net), std::ref(in),
                           host, port);
        std::thread receiver(this->receive_samples, std::ref(io_net),
                             std::ref(out));

        sender.join();
        receiver.join();
    }

   private:
    static void send_samples(class socket& io_net, input& in, const char* host,
                             unsigned int port) {
        static package_t pckg;
        while (true) {
            for (unsigned int i = 0; i < sizeof pckg / audio::buffer_size; ++i)
                std::memcpy(pckg.buffer.data() + i * audio::buffer_size,
                            in.get_samples().data(), audio::buffer_size);

            io_net.send_to(std::move(pckg),
                           boost::asio::ip::make_address_v4(host), port);
        }
    }
    static void receive_samples(class socket& io_net, output& out) {
        static package_t pckg(
            [](T&) { std::printf("New T package accepted.\n"); });
        while (true) {
            io_net.receive_last(pckg);
            out.play_samples({pckg.get_buffer(), package_t::size});
        }
    }

   public:
    const char* host;
    unsigned int port;

   public:
    input in;
    output out;

    boost::asio::io_context io;
    class socket io_net{io};
};

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::printf("Usage: alsa_tcp_voice <device> <host> <port>\n");
        return 1;
    }
    audio::device = argv[1];

    voice_service_udp_user vsc({argv, static_cast<unsigned long>(argc)});

    return 0;
}
