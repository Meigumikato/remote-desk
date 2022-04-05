extern "C" {
#include <SDL2/SDL.h>
#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}
#include <iostream>
#include <thread>


class Player {

 public:
	Player() {

	}


	int Init() {
		if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_TIMER)) {
			return -1;
		}

		SDL_Window *window = SDL_CreateWindow("SDL2", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_RESIZABLE);
		if (!window) {
			return -1;
		}

		SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
		if (!renderer) {
			return -1;
		}
		return 0;
	}


	int Receive() {

		avformat_network_init();
		AVFormatContext *fmt_ctx = avformat_alloc_context();
		AVIOContext *avio_ctx = nullptr;
		AVIOContext *client_ctx = nullptr;

		AVDictionary *options = NULL;
		av_dict_set(&options, "listen", "2", 0);
		avio_open2(&avio_ctx, "tcp://localhost:7000", AVIO_FLAG_READ_WRITE, nullptr, &options);
		avio_accept(avio_ctx, &client_ctx);
		avio_handshake(client_ctx);

		AVPacket *packet = av_packet_alloc();

		unsigned char buffer[8192];

		while (1) {

			int size = avio_read(fmt_ctx->pb, buffer, 8192);
			
			if (size <= 0) continue;
			av_packet_from_data(packet, buffer, size);

			std::cout << "size: " << size << std::endl;
		}

		return 0;
	}

 private:
	// backend thread
	std::thread backend_thread_;


};

int main () {

	Player player;
	player.Receive();
	return 0;
}
