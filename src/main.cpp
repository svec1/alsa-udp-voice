#include <ainput.hpp>
#include <aoutput.hpp>
#include <net.hpp>
#include <string_view>
#include <thread>

struct vcu_config {
    std::string_view device, host;
    boost::asio::ip::port_type port;
};

static vcu_config parse_options(int argc, char* argv[]) {
    vcu_config cfg;

    static constexpr auto throw_usage = [](std::string_view arg,
                                           int expected_argument = 0) {
        std::string throw_string;

        if (expected_argument)
            throw_string = std::format("Option requires an argument: {}", arg);
        else if (expected_argument == -1)
            throw_string = std::format("Invalid argument: {}", arg);
        else
            throw_string = std::format("Invalid option: {}", arg);

        throw std::runtime_error(std::format(
            "{}\n"
            "Usage: alsa_tcp_voice [-D sound device] [-h IP host] [-p port]",
            throw_string));
    };

    static constexpr auto get_argument = [](int argc, char* argv[], int& i) {
        if (std::strlen(argv[i]) > 2)
            return argv[i] + 2;
        else {
            if (argc < i + 1)
                throw_usage(argv[i], 1);
            else if (argv[i + 1][0] == '-')
                throw_usage(argv[i + 1], -1);

            return argv[i++];
        }
    };

    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] != '-') throw_usage(argv[i]);

        std::string_view option = argv[i];
        std::string_view current_value = get_argument(argc, argv, i);

        switch (option[1]) {
            case 'D': {
                cfg.device = current_value;
                break;
            }
            case 'h': {
                cfg.host = current_value;
                break;
            }
            case 'p': {
                try {
                    cfg.port = std::stoi(current_value.data());
                } catch (...) {
                    throw_usage(current_value, -1);
                }
                break;
            }
            default:
                throw_usage(std::string("-") + option[1]);
        }
    }

    return cfg;
}

template <typename T = package_native_t<>>
class voice_service_udp_user {
    using package_t = package<T>;

   public:
    voice_service_udp_user(vcu_config& _cfg)
        : cfg(_cfg),
          io_net(io, boost::asio::ip::make_address_v4(cfg.host), cfg.port) {
        run();
    }

   private:
    void run() {
        std::thread sender(this->send_samples, std::ref(io_net), std::ref(in));
        std::thread receiver(this->receive_samples, std::ref(io_net),
                             std::ref(out));

        sender.join();
        receiver.join();
    }

   private:
    static void send_samples(class socket& io_net, input& in) {
        static package_t pckg;
        while (true) {
            for (unsigned int i = 0; i < sizeof pckg / audio::buffer_size; ++i)
                std::memcpy(pckg.buffer.data() + i * audio::buffer_size,
                            in.get_samples().data(), audio::buffer_size);

            io_net.send_to(std::move(pckg));
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
    vcu_config cfg;

   public:
    input in;
    output out;

    boost::asio::io_context io;
    class socket io_net;
};

int main(int argc, char* argv[]) {
    try {
        vcu_config cfg = parse_options(argc, argv);
        audio::device = cfg.device.data();

        voice_service_udp_user vsc(cfg);
    } catch (std::exception& excp) {
        std::printf("%s\n", excp.what());
        return 1;
    }

    return 0;
}
